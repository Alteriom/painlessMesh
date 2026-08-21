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
- Compile it out entirely with `-U PAINLESSMESH_ENABLE_OTA` on nodes that will
  never receive an update.
- Use distinct `role`/`hardware` strings so an announcement cannot fan out
  across an entire fleet.

Signed OTA is tracked as candidate v2.0 work. Until it ships, assume mesh OTA
is as trusted as mesh membership.

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

## Current Security Posture (as of v2.0)

This section documents the **actual** security properties of the library.  Any
gap listed here is a known limitation, not an oversight.  We believe an honest
documented gap is less harmful than an inaccurate assurance.

### Mesh authentication

- **Shared WPA2 password** (`MESH_PREFIX` / `MESH_PASSWORD`) is the only
  authentication mechanism.  Any device that knows the password can join the
  mesh and send or receive all messages.
- There is no per-node identity, certificate, or token.  Impersonating a
  specific node ID requires only sending packets with that `from` field.

### OTA firmware updates

- **Integrity check**: incoming firmware chunks are verified against an MD5
  hash supplied by the sender.  MD5 is weak against collision attacks and
  provides no authentication guarantee.
- **Authentication**: there is **no cryptographic signature** on OTA images.
  Any mesh node that has the mesh password can push firmware to any other node
  by supplying a matching `md5 + role + hardware` tuple
  (`ota.hpp`, `OTA_OP_CODES::DATA` handler).
- `PAINLESSMESH_ENABLE_OTA` is **on by default**.  Disable it at compile time
  (`#undef PAINLESSMESH_ENABLE_OTA`) if OTA is not required.
- **v2.0 preview**: a `PAINLESSMESH_OTA_REQUIRE_SIGNATURE` compile flag has
  been added (default **off**).  The signing verification back-end will be
  implemented behind this flag during the v2.0 cycle.  The flag will default
  **on** in v3.0.  Do not assume signature verification is active unless you
  explicitly define the macro.

### Gateway HTTPS (sendToInternet)

- **ESP8266**: `WiFiClientSecure::setInsecure()` is called before every HTTPS
  request.  SSL certificate validation is **disabled**.  All HTTPS connections
  from an ESP8266 gateway are vulnerable to MITM attacks.
- **ESP32**: `HTTPClient::begin(url)` is called with **no trust anchor
  configured**.  The underlying `WiFiClientSecure` accepts any certificate by
  default, so HTTPS connections from an ESP32 gateway are equally vulnerable
  to MITM attacks despite the absence of an explicit `setInsecure()` call.
- A `setCACert` / cert-bundle / fingerprint API for both platforms is planned
  for v2.1.  Until then, treat all gateway HTTP traffic as unencrypted from a
  trust perspective.

### Gateway HTTP timeout / mesh partition risk

- The gateway HTTP timeout (`GATEWAY_HTTP_TIMEOUT_MS`) was previously 30 s,
  longer than `NODE_TIMEOUT` (10 s).  A slow server response could block the
  cooperative `TaskScheduler` long enough for every peer's timeout task to
  fire, partitioning the mesh (reported in issues #318, #332).
- **Fixed in v2.0**: `GATEWAY_HTTP_TIMEOUT_MS` is now 8 s.  All direct peer
  connection timeouts are refreshed before the HTTP call so that a slow but
  sub-8 s response does not cause a partition.

## Security Update Process

1. Vulnerability is reported
2. Team confirms and assesses severity
3. Fix is developed and tested
4. Security advisory is published
5. Patch is released
6. Public disclosure after users have time to update

Thank you for helping keep painlessMesh secure!
