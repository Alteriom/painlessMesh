# Architecture — where things live

A map for finding code, not a description of how the mesh works. How it works: `docsify-site/architecture/mesh-architecture.md` and `docsify-site/architecture/plugin-system.md`. Every document and where it is published: `.github/DOCUMENTATION.md`.

## The library (`src/`)

| Path | Holds |
|---|---|
| `src/painlessMesh.h`, `src/AlteriomPainlessMesh.h` | what a sketch includes; the second also carries the version string and MAJOR/MINOR/PATCH defines |
| `src/painlessmesh/*.hpp` | the header-only core: `protocol.hpp` (wire message types, `PackageInterface`), `plugin.hpp` (`SinglePackage`/`BroadcastPackage`), `mesh.hpp` (send options, `sendToInternet()` results), `router.hpp`, `tcp.hpp`, `connection.hpp`, `buffer.hpp`, `layout.hpp`, `gateway.hpp`, `ack.hpp` (delivery confirmation), `message_queue.hpp`, `ota.hpp`, `ntp.hpp`, `configuration.hpp` (build-flag macros) |
| `src/arduino/wifi.hpp` | the Arduino-side `Mesh`: Wi-Fi, the TCP listener, bridge and gateway roles |
| `src/painlessMeshSTA.*` | station scanning and joining |
| `src/boost/` | the desktop transport the tests build against |
| `src/plugin/` | optional plugins (`performance.hpp`, `remote.hpp`) |
| `src/*.cpp` | the only translation units PlatformIO compiles — see [gotchas.md](gotchas.md) |

Application packages built on the plugin system are not in `src/`: they are in `examples/alteriom/alteriom_sensor_package.hpp`, with the custom-package pattern in `examples/alteriom/alteriom_custom_package_template.hpp`.

## Tests (`test/`) — layout and purpose in `test/README.md`

| Path | Level |
|---|---|
| `test/catch/catch_*.cpp` | one Catch2 binary per file, globbed by `CMakeLists.txt`; fakes in `test/include/` and `test/catch/Arduino.h` |
| `test/boost/*.cpp` | several meshes over loopback TCP (`catch_tcp_integration`, `catch_connection`, `catch_stale_gateway`) |
| `test/mock-http-server/server.py` | the HTTP test point for `sendToInternet()`, with a delivery ledger |
| `test/ci/` | CI's own checks and the PlatformIO projects that prove build flags |
| `test/ArduinoJson`, `test/TaskScheduler` | submodules — the only two gitlinks in the tree (`git ls-tree HEAD test/`). `.gitmodules` also names `test/simulator`, but no commit is recorded for it, so `git submodule update --init` does not fetch it |

## Examples (`examples/`)

One folder per sketch; the folder name equals the `.ino` name, which is what CI compiles. Each is also listed in `library.json` `examples`. Simulator scenarios for an example live in `examples/<example>/test/simulator/`.

## Outside this repository

- The hardware rig (Alteriom ESP32 farm) is a separate private repository; `.github/workflows/farm-hil.yml` dispatches it and carries its verdict.
- Multi-node simulation is `Alteriom/painlessMesh-simulator`, a separate repository (`CONTRIBUTING.md` "Simulator and hardware tests"); it is not checked out here.
