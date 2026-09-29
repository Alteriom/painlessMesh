# Coding rules

What a change to `src/` or `examples/` has to respect. Rules CI enforces are named with the job that enforces them; the job is the authority.

## Enforced by CI (`.github/workflows/ci.yml`)

- **Warnings are errors** under gcc and clang (`build-test-desktop`). clang is the stricter of the two: #469's missing virtual destructor on `PackageInterface` failed only there.
- **Every pointer and arithmetic member under `src/` has a default member initializer** (`code-quality`, `test/ci/check_member_init.py`). #466 was an `AsyncServer*` with none, read before it was set, crashing every node at boot.
- **It still builds as gnu++11** on the ESP32 Arduino 2.x core that `examples/basic/platformio.ini` pins: a struct with default member initializers is not an aggregate there, so brace-initialising one does not compile (`build-platformio`).
- **Every example compiles for esp32 and esp8266** (`build-arduino`), so a public API change updates the examples that use it.
- **`examples/alteriom/` is clang-formatted** against `.clang-format` and carries no TODO/FIXME (`code-quality`). Core files under `src/` have their own historical formatting; do not reformat them in passing.

## Conventions the history keeps (not enforced)

- C++14 (`CMakeLists.txt`), 2-space indent, `TSTRING` rather than `String` in code that also builds on the desktop.
- Memory is tight — ESP8266 far more than ESP32. Prefer fixed-size fields and bounded containers; say in the spec or PR what a new buffer or queue costs.
- A new public capability gets a feature-test macro so sketches built against older releases can test for it: `PAINLESSMESH_HAS_TCP_LISTENING` (`src/arduino/wifi.hpp`), `PAINLESSMESH_HAS_INTERNET_RESULT` (`src/painlessmesh/mesh.hpp`). New public names also go in `keywords.txt`.
- A tunable is a build flag read in `src/painlessmesh/configuration.hpp`, documented as a flag, and tested per target — never `#define`d in one source file (the reason is in `CMakeLists.txt`, above `catch_ota_disabled`).
- A 2.x patch release stays wire-compatible with 2.0 (`CONTRIBUTING.md` "Versioning").
- Input from the mesh is untrusted: validate JSON before use, bound every length. Threat model and what the library does not protect: `SECURITY.md`.
- A comment says why the code is shaped this way and, where one exists, which issue proved it (see the comments on `PackageInterface` and in `ci.yml`). Do not narrate what the next line does.

## Commits and branches

Conventional-commit subjects whose text states the outcome ("fix(gateway): a bridge serves its first sends on its own uplink"), with the issue number. Branch naming, the target branch, and who merges: `CONTRIBUTING.md` "Branches". `.github/copilot-instructions.md` still describes a `develop` branch and git flow; that is retired.
