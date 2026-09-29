# 01_spec — agree API, wire and version

One job: a short spec a maintainer can approve or refuse before any code exists.

## Inputs
- Working (this run): the request the person gave — an issue, a discussion, a need from a downstream project.
- Reference (every run): ../../../_shared/architecture.md
- Reference (every run): ../../../_shared/package-types.md
- Reference (every run): ../../../_shared/coding-rules.md

Do NOT load: `01-fix/`, `03-release/`, other runs' output, the full `CHANGELOG.md`.

## Process
1. State the need in the user's terms and what they do today without it. Check it does not exist already (`USER_GUIDE.md`, `src/`, the examples).
2. Name the public surface exactly: methods and signatures, callbacks, build flags, the `PAINLESSMESH_HAS_…` macro, new names for `keywords.txt`.
3. State the wire impact: none, a new package type (pick the number with [package-types.md](../../../_shared/package-types.md) and show the grep), or a changed message. Say what an older node does when it receives the new traffic.
4. Classify the version (major, minor, patch) against `CONTRIBUTING.md` "Versioning", with the reason.
5. Cost on the device: RAM and flash on ESP8266 and ESP32, new tasks or buffers, security relevance (`SECURITY.md`).
6. Test plan: which level from `_shared/build-and-test.md` "Choosing the level of a test", and whether the rig must see it. A new example is part of the plan if users need one.
7. Out of scope: what this deliberately does not do.

## Outputs
- spec.md → output/ — sections 1–7 above, in that order, under two pages.

## Human check
A maintainer approves the public names, the wire impact, the type number and the version class, or sends the spec back. Nothing is implemented against an unapproved spec. Edit `spec.md` in place; the next stages build what it says.
