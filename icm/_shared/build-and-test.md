# Build, test, and what counts as evidence

The commands CI actually runs are in `.github/workflows/ci.yml`; when this file and that one disagree, that one is right. The contributor's version is `CONTRIBUTING.md` ("Testing requirements").

## Desktop (Catch2 and Boost), from the repository root

```bash
git submodule update --init          # ArduinoJson, TaskScheduler, simulator
cmake -G Ninja . && ninja            # binaries land in bin/
run-parts --regex catch_ bin/        # every suite; or run one: ./bin/catch_routing
```

CI builds this three ways (job `build-test-desktop`): gcc with `-Wall -Werror`, clang with `-Wall -Werror -Wno-vla-cxx-extension`, and gcc with AddressSanitizer (`ASAN_OPTIONS=detect_leaks=0`: the signal wanted is use-after-free, not the fakes' deliberate leaks). A change that only builds under gcc is not done. Needs `libboost-system-dev`.

Gateway tests compare the library's verdict with a real HTTP server's ledger. Start the test point and export its address, or those scenarios only warn:

```bash
python3 test/mock-http-server/server.py --host 127.0.0.1 --port 8080 &
export PAINLESSMESH_TESTPOINT=http://127.0.0.1:8080
```

## Device builds

- Every example, both chips: job `build-arduino` (`arduino-cli`, esp32 and esp8266 `nodemcuv2`). Library dependencies it installs are listed there.
- PlatformIO: `bash test/ci/test_platformio.sh --example basic` (and `--example alteriom`); build-flag projects: `cd test/ci/no-ota && pio run -e esp32 && pio run -e esp8266`, same for `test/ci/tuned-timeouts`.
- Library metadata: `bash scripts/validate-arduino-compliance.sh` and `python3 scripts/validate_library_structure.py`.

## Code-quality checks (job `code-quality`)

`clang-format --dry-run --Werror` on `examples/alteriom/`; `bash test/ci/test_check_member_init.sh && python3 test/ci/check_member_init.py src`; `bash test/ci/test_bump_version.sh`; no TODO/FIXME in `examples/alteriom/`.

## Choosing the level of a test

| The behaviour is | Test it with |
|---|---|
| one component's logic (a buffer, a package's fields, a router decision) | a `test/catch/catch_<topic>.cpp` scenario |
| several nodes exchanging messages | `test/boost/` over loopback TCP |
| what an Internet service answered through a gateway | the test point: add the service's behaviour to `test/mock-http-server/server.py` first, then the scenario that fails against it |
| a build flag an operator sets | a PlatformIO project under `test/ci/` (the catch shim hides these — [gotchas.md](gotchas.md)) |
| radio, routing between real nodes, gateway uplink, failover, OTA, timing | the hardware rig, plus a serial log |

## Evidence

The rules are in `CONTRIBUTING.md` ("Submit a pull request") and `RELEASE_GUIDE.md` ("How the hardware result arrives"). In short: the unit tests mock the radio, so for radio, routing, gateway or OTA the evidence is a serial log or a rig run (a maintainer adds the `run-hil` label; the verdict arrives as the `farm/hil` status). A test that re-implements the logic it checks, instead of calling the library, is not evidence.
