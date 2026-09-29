# Gotchas

Traps that have already cost this repository a failed build, a wrong fix or a release. One line each, with the file that explains it — read that file before working near the trap.

## Toolchain and build

- ArduinoJson v7: `JsonDocument doc(size)` breaks on ESP32, and internal `ArduinoJson::V7…` namespaces must not be named. — `CLAUDE.md` "Gotchas"
- PlatformIO compiles only the `.cpp` files directly in `src/`, which is why `src/connection.cpp` is not under `src/painlessmesh/`. — `CLAUDE.md` "Gotchas"
- A catch test never sees the guards in `src/painlessmesh/configuration.hpp`: `test/catch/Arduino.h` defines its include guard and supplies its own macros. A build flag is proven only by a PlatformIO project in `test/ci/`. — `.github/workflows/ci.yml`, `build-platformio` step comment
- A macro that changes a class's members must reach every translation unit, so it is set per CMake target, not `#define`d in a test file (ODR). — `CMakeLists.txt`, comments above `catch_ota_disabled` and `catch_node_timeout_override`
- Defining both `PAINLESSMESH_ENABLE_OTA` and `PAINLESSMESH_DISABLE_OTA` is a compile error on purpose. — `SECURITY.md` "OTA"
- Only a sketch whose folder name equals its `.ino` name is compiled by CI; an earlier loop compiled none at all for a while. — `.github/workflows/ci.yml`, `build-arduino`
- The ESP32 example build is pinned to the gnu++11 core on purpose; do not "upgrade" the pin to make a brace-initialiser compile. — `examples/basic/platformio.ini`

## Tests and evidence

- A desktop test that copies the logic under test mirrors the bug: `catch_http_status_codes.cpp` copied the `uint16_t` behind #446. — `RELEASE_GUIDE.md` "How the hardware result arrives"
- Gateway scenarios without `PAINLESSMESH_TESTPOINT` only warn, so a local green run may have checked nothing. — `.github/workflows/ci.yml`, "Start the HTTP test point"
- Real services answer badly (CallMeBot refuses with HTTP 201/203); a gateway bug starts by teaching `test/mock-http-server/server.py` the bad answer. — `CONTRIBUTING.md` "The HTTP test point"
- The unit tests mock the radio; #466 reached a user before a board. Radio, routing, gateway, OTA changes need the rig or a serial log. — `CONTRIBUTING.md` "Simulator and hardware tests"

## Duplicated files that must move together

- `examples/alteriom/mppt_example/` carries its own copies of `alteriom_sensor_package.hpp` and `alteriom_custom_package_template.hpp` (a sketch folder can include only its own files). Change both and `diff` them.
- The version lives in six files the script updates and two it does not (`CLAUDE.md` "Version", `README.md` "Latest Release"). — [release.md](release.md)

## Documents that are out of date

Trust `CONTRIBUTING.md`, `RELEASE_GUIDE.md`, `.github/workflows/` and the code over these:

- `.github/copilot-instructions.md`: `develop` branch and git flow (retired), package type numbers (wrong).
- `CLAUDE.md` "Package types": numbers disagree with the code. — [package-types.md](package-types.md)
- `CONTRIBUTING.md` "Versioning" says three version files; `RELEASE_GUIDE.md` and `scripts/bump-version.sh` say six.
