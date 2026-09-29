# Build, test, and what counts as evidence

What CI runs is `.github/workflows/ci.yml`, one job per concern; read the job for its exact flags, targets and installed libraries rather than copying them anywhere. The contributor's version is `CONTRIBUTING.md` "Testing requirements".

## Desktop (Catch2 and Boost), from the repository root

The commands of `CONTRIBUTING.md` "Running the desktop tests":

```bash
git submodule update --init          # test/ArduinoJson and test/TaskScheduler
cmake -G Ninja . && ninja            # binaries land in bin/
run-parts --regex catch_ bin/        # every suite; or run one: ./bin/catch_routing
```

CI builds this under more than one compiler and a sanitizer (job `build-test-desktop`); a change that builds under one compiler only is not done. Gateway tests compare the library's verdict with a real HTTP server's ledger: start `test/mock-http-server/server.py` and export `PAINLESSMESH_TESTPOINT` as that job's "Start the HTTP test point" step does, or those scenarios only warn.

## Device builds and code quality

| What | Where it is defined |
|---|---|
| every example, esp32 and esp8266 | job `build-arduino` |
| PlatformIO builds, and the projects under `test/ci/` that prove build flags | job `build-platformio`; `test/ci/test_platformio.sh` |
| formatting, member initializers, the version script's own test, TODO/FIXME | job `code-quality` |
| library metadata | job `arduino-library-validation` (`scripts/validate-arduino-compliance.sh`, `scripts/validate_library_structure.py`) |

## Choosing the level of a test

| The behaviour is | Test it with |
|---|---|
| one component's logic (a buffer, a package's fields, a router decision) | a `test/catch/catch_<topic>.cpp` scenario |
| several nodes exchanging messages | `test/boost/` over loopback TCP |
| what an Internet service answered through a gateway | the test point: add the service's behaviour to `test/mock-http-server/server.py` first, then the scenario that fails against it |
| a build flag an operator sets | a PlatformIO project under `test/ci/` (the catch shim hides these — [gotchas.md](gotchas.md)) |
| radio, routing between real nodes, gateway uplink, failover, OTA, timing | the hardware rig, plus a serial log |

## Evidence

The rules are in `CONTRIBUTING.md` ("Submit a pull request") and `RELEASE_GUIDE.md` ("How the hardware result arrives"). In short: the unit tests mock the radio, so for radio, routing, gateway or OTA the evidence is a serial log or a rig run (a maintainer adds the `run-hil` label). A rig run counts only when it links to a run on the farm: a green `Hardware validation on the farm` job without that link may have run nothing — [release.md](release.md) "Is it proof that the rig ran?". A test that re-implements the logic it checks, instead of calling the library, is not evidence.
