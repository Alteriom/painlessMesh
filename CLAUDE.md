# CLAUDE.md — painlessMesh

C++ mesh networking library for ESP32/ESP8266 (PlatformIO). Alteriom fork of painlessMesh v2.0.0.

## Work on this repository

Each recurring job is a pipeline under `.github/icm/`, whose stages stop at a person's check. Start at [.github/icm/CLAUDE.md](.github/icm/CLAUDE.md), which routes by what just happened; the rows below are its jobs.

| Job | Start at |
|---|---|
| Fix a defect (an issue, a failing test, CI or the rig red on `main`) | [.github/icm/01-fix](.github/icm/01-fix/CLAUDE.md) |
| Add a capability (API, option, package type, example) | [.github/icm/02-feature](.github/icm/02-feature/CLAUDE.md) |
| Release a version | [.github/icm/03-release](.github/icm/03-release/CLAUDE.md) |
| Review a dependency bump | [.github/icm/04-deps](.github/icm/04-deps/CLAUDE.md) |
| Correct a document that disagrees with the code | [.github/icm/05-docs](.github/icm/05-docs/CLAUDE.md) |
| Report a suspected vulnerability | [SECURITY.md](SECURITY.md) "Reporting a Vulnerability" — privately, never as a public issue or pull request |

Where the code lives: [.github/icm/_shared/architecture.md](.github/icm/_shared/architecture.md). Build and test commands: [.github/icm/_shared/build-and-test.md](.github/icm/_shared/build-and-test.md). Package type numbers: [.github/icm/_shared/package-types.md](.github/icm/_shared/package-types.md) — the code is the authority; do not copy numbers from any document. The version is whatever `library.properties` says; `scripts/bump-version.sh` changes it.

## Gotchas

**ArduinoJson v7:**
- `JsonDocument doc(size)` does NOT work on ESP32 — use `JsonDocument doc;` (default constructor, grows dynamically)
- Never use internal namespaces like `ArduinoJson::V742PB22::` — use public types directly

**PlatformIO:**
- Only compiles `.cpp` files in `srcDir` root, not subdirectories
- `connection.cpp` must live in `src/` not `src/painlessmesh/` — it was moved there intentionally

More, each with its source: [.github/icm/_shared/gotchas.md](.github/icm/_shared/gotchas.md).
