# Security Policy

## Supported Versions

We release patches for security vulnerabilities. Which versions are eligible for receiving such patches depends on the CVSS v3.0 Rating:

| Version | Supported          |
| ------- | ------------------ |
| 2.0.x   | :white_check_mark: |
| 1.x     | :white_check_mark: (critical fixes only) |
| < 1.0   | :x:                |

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
