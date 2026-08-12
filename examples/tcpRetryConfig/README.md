# tcpRetryConfig

Tuning painlessMesh's TCP connection retry behaviour with
`setTcpRetryConfig()` (issue
[#378](https://github.com/Alteriom/painlessMesh/issues/378)).

## What this controls

When a node acquires an IP and tries to open its TCP connection to the mesh,
that connection can fail — the parent's TCP server may not be ready yet, the
network stack may still be settling, or several nodes may be connecting at
once. painlessMesh retries with exponential backoff before giving up and
falling back to a full WiFi reconnect.

Five parameters describe that behaviour:

| Field | Default | Meaning |
|---|---|---|
| `maxRetries` | 5 | TCP connect attempts after the first before giving up |
| `retryDelayMs` | 1000 | Base delay between retries; scaled 1x, 2x, 4x, 8x, 8x |
| `stabilizationDelayMs` | 500 | Wait after IP acquisition before the first attempt |
| `exhaustionReconnectDelayMs` | 10000 | Wait before the WiFi reconnect that follows exhaustion |
| `failureBlockDurationMs` | 60000 | How long a failed peer is skipped during AP selection |

With the defaults, a node that cannot reach its parent spends
1 + 2 + 4 + 8 + 8 = **23 s** retrying, then waits another **10 s** before
reconnecting WiFi, and will not re-select that same peer for **60 s**.

## Profiles in this sketch

Switch with `#define ACTIVE_PROFILE`.

| | `PROFILE_REALTIME` | default | `PROFILE_RELIABLE` | `PROFILE_BATTERY` |
|---|---|---|---|---|
| `maxRetries` | 1 | 5 | 10 | 2 |
| `retryDelayMs` | 200 | 1000 | 2000 | 3000 |
| `stabilizationDelayMs` | 100 | 500 | 1000 | 500 |
| `exhaustionReconnectDelayMs` | 1000 | 10000 | 30000 | 60000 |
| `failureBlockDurationMs` | 5000 | 60000 | 180000 | 300000 |
| worst-case retry time | 0.2 s | 23 s | 126 s | 9 s |
| full failure cycle | 1.2 s | 33 s | 156 s | 69 s |

- **`PROFILE_REALTIME`** — real-time sensor and LED meshes, the use case from
  [discussion #368](https://github.com/Alteriom/painlessMesh/discussions/368).
  A node stuck in a 23 s backoff is worse than one that drops and re-scans, so
  fail fast and move on.
- **`PROFILE_RELIABLE`** — industrial meshes where getting connected matters
  more than how long it takes. `maxRetries = 10` is the maximum the library
  accepts.
- **`PROFILE_BATTERY`** — every retry is radio-on time. Few attempts, spaced
  widely, and a long blocklist so the node stops waking up for a peer that is
  known to be down.

## Clamping

`setTcpRetryConfig()` coerces the two values that can render a node unusable:

- `maxRetries` is capped at **10**. Each retry allocates an `AsyncClient` and
  schedules a task, so an unbounded value is a heap and recursion-depth hazard
  on ESP8266.
- `retryDelayMs` is held between **50 ms** and **60000 ms**. A zero delay would
  schedule retries with no spacing — a hot loop allocating an `AsyncClient`
  every scheduler tick. The ceiling keeps `retryDelayMs * 8` clear of `uint32_t`
  overflow.

Everything else passes through untouched, including zeros, which are
meaningful:

- `maxRetries = 0` — do not retry at all; fall straight back to a WiFi
  reconnect on the first TCP error.
- `stabilizationDelayMs = 0` — attempt the TCP connection immediately on IP
  acquisition.
- `exhaustionReconnectDelayMs = 0` — reconnect WiFi immediately after
  exhaustion.
- `failureBlockDurationMs = 0` — never blocklist a failed peer.

Call `getTcpRetryConfig()` after setting to see what actually took effect; the
sketch prints this at startup.

## The defaults exist for a reason

painlessMesh 1.9.x deliberately *raised* these values (retries 3 → 5, base
delay 500 ms → 1000 ms) to fix real-world mesh instability. Tuning them back
down reintroduces the problems that change fixed. Symptoms of an over-aggressive
profile:

- **Connection churn** — nodes repeatedly connect and drop. `maxRetries` is too
  low for how long your parent actually takes to be ready; raise it or raise
  `stabilizationDelayMs`.
- **Rapid reconnect loops / network congestion** — a node hammers a parent whose
  TCP server is down. `exhaustionReconnectDelayMs` is too short.
- **The same dead peer is picked over and over** — `failureBlockDurationMs` is
  shorter than one full retry-plus-reconnect cycle, so the peer comes off the
  blocklist before the node has finished failing over. Keep
  `failureBlockDurationMs` greater than
  `(sum of retry backoffs) + exhaustionReconnectDelayMs`. All three profiles
  above satisfy this; `catch_tcp_blocklist.cpp` pins it as a test.

Change one parameter at a time and watch the serial log with
`mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION)`.

## Building

```bash
# PlatformIO
pio run -e esp32     # or -e esp8266

# Arduino CLI
arduino-cli compile --fqbn esp32:esp32:esp32 examples/tcpRetryConfig/tcpRetryConfig.ino
```
