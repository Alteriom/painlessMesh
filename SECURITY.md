# Security Policy

## Supported Versions

We release patches for security vulnerabilities. Which versions are eligible for receiving such patches depends on the CVSS v3.0 Rating:

| Version | Supported          |
| ------- | ------------------ |
| 2.0.x   | :white_check_mark: |
| 1.x     | :white_check_mark: (critical fixes only) |
| < 1.0   | :x:                |

## Threat model — what painlessMesh does and does not protect

This section states the current security posture plainly. It is written down
because the alternative — leaving it implied — has already produced source
comments that claimed protection the code does not implement. If you are
evaluating this library for a deployment that needs a security review, read
this first.

### Mesh membership: one shared secret, no per-node identity

Every node joins with the same SSID and password passed to `mesh.init()`. That
password is the entire access control model. There is no per-node identity, no
key exchange, no message signing and no replay protection.

Consequences:

- Anyone who extracts the password from any single node's firmware — flash
  readout, a lost device, a shared build artifact — can join the mesh as a
  full peer and inject packages indistinguishable from legitimate ones.
- Rotating the password means reflashing every node. There is no revocation.
- The mesh is only as trustworthy as its least-physically-secure node.

Treat mesh membership as a perimeter, not as authentication.

### OTA: integrity checked, not authenticated

A node that calls `mesh.initOTAReceive(role)` opts into firmware updates over
the mesh. It accepts an `Announce` when the announced `role` and `hardware`
match its own, requests the firmware, and verifies the received image against
the MD5 **from that same announcement**.

That is an integrity check against transmission corruption. It is not a
signature. The announcer is not authenticated, so MD5 confirms only that the
firmware you received is the firmware the announcer meant to send — the
announcer could be anyone on the mesh. Combined with the shared-secret
membership model above, any party who can join the mesh can push firmware to
every node matching a role/hardware pair.

Mitigations available today:

- Do not call `initOTAReceive()` on nodes that do not need it. Mesh OTA is
  opt-in per sketch; `PAINLESSMESH_ENABLE_OTA` only compiles the code in.
- Compile it out entirely on nodes that will never receive an update:

  ```ini
  ; platformio.ini
  build_flags = -DPAINLESSMESH_DISABLE_OTA
  ```

  A `#define PAINLESSMESH_DISABLE_OTA` above `#include <painlessMesh.h>` works
  for that one translation unit; the build flag is preferred because it applies
  to all of them.
- Use distinct `role`/`hardware` strings so an announcement cannot fan out
  across an entire fleet.

Until v2.0 this document recommended `-U PAINLESSMESH_ENABLE_OTA` (and, in
some revisions, `#undef PAINLESSMESH_ENABLE_OTA`). **Neither ever worked.**
`configuration.hpp` defined that macro unconditionally, during the compile and
long after a `-U` or a sketch-level `#undef` had been applied, so operators
who followed that advice shipped OTA code believing they had removed it. v2.0
guards the default definition, which is what makes
`-DPAINLESSMESH_DISABLE_OTA` above effective. An OTA-free build is compiled
and run in CI on every commit (`test/catch/catch_ota_disabled.cpp`) so the
mitigation cannot quietly rot a second time.

Signed OTA is roadmap work and is **not** in v2.0. There is deliberately no
flag for it: `PAINLESSMESH_OTA_REQUIRE_SIGNATURE` is rejected with a compile
error rather than accepted as a no-op, so no build can claim an enforcement
the library does not perform. Until signed OTA ships, assume mesh OTA is as
trusted as mesh membership.

### Gateway HTTPS: encrypted, not authenticated

The gateway relays mesh messages to Internet destinations over HTTPClient.
**No trust anchor is configured on either target.** There is no `setCACert`,
no certificate bundle and no fingerprint API anywhere in `src/`:

- **ESP8266** calls `setInsecure()`, which disables certificate validation
  outright.
- **ESP32** calls `http.begin(url)` with nothing supplied. What that yields
  depends entirely on the Arduino-ESP32 core in use; do not read it as
  verified. (A source comment previously asserted ESP32 "uses default SSL
  settings with certificate validation". That was never backed by anything in
  this repository and has been corrected.)

So an `https://` gateway destination gets you transport encryption, and it does
not get you assurance that you are talking to the host you named. Do not send
long-lived credentials, API tokens or anything else that an active man in the
middle must not learn, over a gateway destination on an untrusted network.

A configurable TLS trust API on the gateway config is tracked as v2.1 work.

### Gateway blocking: the mesh partition risk

`sendToInternet()` runs its HTTP request synchronously on the cooperative
`TaskScheduler`. Nothing else on the scheduler runs during the call, but
wall-clock time keeps passing, so any peer whose `NODE_TIMEOUT` deadline falls
inside the stall is overdue the instant the scheduler resumes and gets dropped
despite never having gone missing.

Until v2.0 the gateway timeout was a hardcoded 30 s against a 10 s
`NODE_TIMEOUT`, so a single slow server could partition the mesh around its own
gateway (issues #318, #332).

Fixed in v2.0 (`painlessmesh/gateway.hpp`, `arduino/wifi.hpp`):

- `GATEWAY_HTTP_TIMEOUT_MS` is derived from `NODE_TIMEOUT` rather than
  hardcoded — `NODE_TIMEOUT / 2`, so 5 s at the default.
- A `static_assert` fails the build if the two *socket* timeouts — the request
  and the captive-portal probe — reach `NODE_TIMEOUT` between them. Raising
  one without raising `NODE_TIMEOUT` is a compile error, not a field outage.
- Every direct peer's timeout task is restarted **after** the blocking call
  returns, not before it. Refreshing beforehand is the half that cannot work:
  the deadline is wall-clock, so pushing it out ahead of a stall longer than
  the window changes nothing.

This bounds the damage; it does not make the call asynchronous. A gateway
still stops servicing mesh traffic for up to `NODE_TIMEOUT` per request.

**Known gap — DNS is not inside that budget (issue #416).** On the first
request, and again whenever the 60 s connectivity cache expires,
`hasActualInternetAccess()` resolves a well-known hostname with
`WiFi.hostByName()` before either timed call runs. That overload takes no
timeout argument, so the stall is whatever the platform's resolver does, and
`gatewayBlockingBudgetMs()` does not count it — caching lowers how often the
unbounded call happens, not how long it can take. A gateway on a network with
slow or blackholed DNS can therefore still overrun `NODE_TIMEOUT` and
partition the mesh, at roughly one request per minute rather than every
request. **Do not read the `static_assert` as a guarantee that a gateway
cannot outlast the watchdog.** Bounding DNS needs a per-platform timeout API
and is tracked in #416.

### Denial of service

There is no rate limiting, no per-peer quota and no admission control on any
package type. A joined node can saturate the mesh. Bounding the outbound send
buffer is tracked in issue #388.

### What is in scope for a vulnerability report

Given the above, the following are **known and documented**, not
vulnerabilities to report: unauthenticated OTA, absent TLS trust anchors,
shared-password membership, and lack of rate limiting. Please do report memory
corruption, crashes reachable from a malformed packet, secrets leaking into
logs, and anything that defeats a protection this document claims exists.

## Reporting a Vulnerability

If you discover a security vulnerability in painlessMesh, please report it by:

1. **DO NOT** open a public issue
2. Use GitHub's [Security Advisory](https://github.com/Alteriom/painlessMesh/security/advisories/new) feature, or email the maintainer directly with:
   - Description of the vulnerability
   - Steps to reproduce
   - Potential impact
   - Suggested fix (if any)

We will respond within 48 hours and provide regular updates on the progress of the fix.

## Security Update Process

1. Vulnerability is reported
2. Team confirms and assesses severity
3. Fix is developed and tested
4. Security advisory is published
5. Patch is released
6. Public disclosure after users have time to update

Thank you for helping keep painlessMesh secure!
