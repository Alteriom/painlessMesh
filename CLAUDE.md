# CLAUDE.md — painlessMesh

C++ mesh networking library for ESP32/ESP8266 (PlatformIO). Alteriom fork of painlessMesh v2.0.0.

## Work on this repository

Each recurring job is a pipeline under `icm/`, whose stages stop at a person's check. Start at [icm/CLAUDE.md](icm/CLAUDE.md), which routes by what just happened; the rows below are its jobs.

| Job | Start at |
|---|---|
| Fix a defect (an issue, a failing test, CI or the rig red on `main`) | [icm/01-fix](icm/01-fix/CLAUDE.md) |
| Add a capability (API, option, package type, example) | [icm/02-feature](icm/02-feature/CLAUDE.md) |
| Release a version | [icm/03-release](icm/03-release/CLAUDE.md) |
| Review a dependency bump | [icm/04-deps](icm/04-deps/CLAUDE.md) |
| Correct a document that disagrees with the code | [icm/05-docs](icm/05-docs/CLAUDE.md) |
| Report a suspected vulnerability | [SECURITY.md](SECURITY.md) "Reporting a Vulnerability" — privately, never as a public issue or pull request |

Where the code lives: [icm/_shared/architecture.md](icm/_shared/architecture.md). Build and test commands: [icm/_shared/build-and-test.md](icm/_shared/build-and-test.md). Package type numbers: [icm/_shared/package-types.md](icm/_shared/package-types.md) — the code is the authority; do not copy numbers from any document. The version is whatever `library.properties` says; `scripts/bump-version.sh` changes it.

## Gotchas

**ArduinoJson v7:**
- `JsonDocument doc(size)` does NOT work on ESP32 — use `JsonDocument doc;` (default constructor, grows dynamically)
- Never use internal namespaces like `ArduinoJson::V742PB22::` — use public types directly

**PlatformIO:**
- Only compiles `.cpp` files in `srcDir` root, not subdirectories
- `connection.cpp` must live in `src/` not `src/painlessmesh/` — it was moved there intentionally

More, each with its source: [icm/_shared/gotchas.md](icm/_shared/gotchas.md).
