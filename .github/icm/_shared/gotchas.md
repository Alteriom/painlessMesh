---
verified_at: 2026-10-15
verified_commit: d46bfa244d39a3f5a3745754fc442ea56a51f540
---

# Gotchas

Traps that have already cost this repository a failed build, a wrong fix or a release. One line each, with the file that explains it — read that file before working near the trap.

## Toolchain and build

- ArduinoJson v7: `JsonDocument doc(size)` does not work on ESP32 — use `JsonDocument doc;`, which grows as needed; and never name an internal namespace such as `ArduinoJson::V742PB22::` — use the public types. — recorded in #362
- PlatformIO compiles only the `.cpp` files directly in the `srcDir` that `library.json` sets (`src`), not in its subfolders, which is why `src/connection.cpp` is not under `src/painlessmesh/`. — recorded in #362
- A catch test never sees the guards in `src/painlessmesh/configuration.hpp`: `test/catch/Arduino.h` defines its include guard and supplies its own macros. A build flag is proven only by a PlatformIO project in `test/ci/`. — `.github/workflows/ci.yml`, `build-platformio` step comment
- A macro that changes a class's members must reach every translation unit, so it is set per CMake target, not `#define`d in a test file (ODR). — `CMakeLists.txt`, comments above `catch_ota_disabled` and `catch_node_timeout_override`
- Defining both `PAINLESSMESH_ENABLE_OTA` and `PAINLESSMESH_DISABLE_OTA` is a compile error on purpose: OTA is on by default, so the disable flag alone is what a build wants. — `src/painlessmesh/configuration.hpp`, the `#error` and the comment above it
- Only a sketch whose folder name equals its `.ino` name is compiled by CI; an earlier loop compiled none at all for a while. — `.github/workflows/ci.yml`, `build-arduino`
- The ESP32 example build is pinned to the gnu++11 core on purpose; do not "upgrade" the pin to make a brace-initialiser compile. — `examples/basic/platformio.ini`

## Tests and evidence

- A desktop test that copies the logic under test mirrors the bug: `catch_http_status_codes.cpp` copied the `uint16_t` behind #446. — `RELEASE_GUIDE.md` "How the hardware result arrives"
- Gateway scenarios without `PAINLESSMESH_TESTPOINT` only warn, so a local green run may have checked nothing. — `.github/workflows/ci.yml`, "Start the HTTP test point"
- Real services answer badly (CallMeBot refuses with HTTP 201/203); a gateway bug starts by teaching `test/mock-http-server/server.py` the bad answer. — `CONTRIBUTING.md` "The HTTP test point"
- The unit tests mock the radio; #466 reached a user before a board. Radio, routing, gateway, OTA changes need the rig or a serial log. — `CONTRIBUTING.md` "Simulator and hardware tests"
- A green `Hardware validation on the farm` may have run nothing: without `FARM_DISPATCH_TOKEN` it posts "No rig run" and succeeds, and `Automated Release` then tags a version bump no board has seen. Check the run's log for a "Rig run" link before calling it evidence. — [release.md](release.md) "Is it proof that the rig ran?"; `.github/workflows/farm-hil.yml`, "Send the farm its repository_dispatch"
- A multi-node rig run that dies mid-transfer (OTA, gateway relay) with `scanComplete(): Mesh also on channel N with X nodes, Y here` in the log is a mesh split across channels, not a library regression: a bridge sits on its router's channel and the nodes that see it follow, while `init()` defaults to channel 1. Check that the rig's uplink AP is on the mesh's channel before debugging the library. — `src/painlessMeshSTA.cpp:412`; `src/arduino/wifi.hpp:63`, `:189`
- Every node registers a fallback `GATEWAY_DATA` handler first, in `Mesh::init()`: it answers "is not an Internet gateway" only while it is the only such handler, so a rebooted ex-bridge still in `knownBridges` refuses instead of swallowing the request, and the sender forgets it and redispatches without spending a retry. A gateway handler depends on that order and count; the regression test is `catch_stale_gateway` (`test/boost/stale_gateway.cpp`). — `src/painlessmesh/mesh.hpp:304`, `:2237`

## Workflows

- Workflow `name:` values are triggers: `farm-hil.yml` runs on `workflows: ["CI/CD Pipeline"]` and `release.yml` on `workflows: ["Hardware validation on the farm"]`. Renaming either workflow breaks the chain without an error anywhere; a CI change that renames one updates its consumer in the same pull request. — the `on: workflow_run` block of each
- A merge that changes `package.json` counts as a version bump to `release.yml`, so a dependency bump merged while an untagged version sits on `main` releases it. — [release.md](release.md) "From merge to registries"
- Do not hunt for a credential for the `alteriom` PlatformIO owner: this project cannot publish under it, which is why `platformio-publish.yml` defaults to another owner. Publish and repair under that default. — `.github/workflows/platformio-publish.yml:20`

## Duplicated files that must move together

- `examples/alteriom/mppt_example/` carries its own copies of `alteriom_sensor_package.hpp` and `alteriom_custom_package_template.hpp` (a sketch folder can include only its own files). Change both and `diff` them.

## Documents that can be out of date

Trust `CONTRIBUTING.md`, `RELEASE_GUIDE.md`, `.github/workflows/` and the code over `docsify-site/` and `docsify-site/wiki/`: they restate facts the code owns and have drifted before (a `develop` branch, `CommandPackage` as 201). A disagreement is a `05-docs` correction.
