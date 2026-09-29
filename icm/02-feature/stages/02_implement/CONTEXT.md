# 02_implement — build what the spec says

One job: implement the approved spec, and record every place the code had to differ from it.

## Inputs
- Working (this run): ../01_spec/output/spec.md
- Reference (every run): ../../../_shared/coding-rules.md
- Reference (every run): ../../../_shared/gotchas.md
- Reference (only to place a new file): ../../../_shared/architecture.md

Do NOT load: `01-fix/`, `03-release/`, `04-deps/`, `05-docs/`, `_shared/release.md`, other runs' output.

## Process
1. Implement the public surface exactly as named in the spec, in the files [architecture.md](../../../_shared/architecture.md) would put it (header-only core under `src/painlessmesh/`, Arduino side in `src/arduino/wifi.hpp`, application packages in `examples/alteriom/` and their `mppt_example/` copies).
2. Follow [coding-rules.md](../../../_shared/coding-rules.md): default member initializers, gnu++11-safe initialisation, both compilers at `-Werror`, a feature-test macro, a build flag read in `configuration.hpp` for any tunable.
3. Update every example that the change breaks. A new example is the one the spec names, and stage 03 adds it; do not add others here.
4. Build the desktop suite and one ESP8266 and one ESP32 PlatformIO build of an example that uses the change.
5. Where the code had to differ from the spec, write down what and why. Do not silently widen the API.

## Outputs
- implementation.md → output/ — files changed and what each holds; the public surface as built; deviations from the spec with reasons; the build results.

## Human check
A person reads `implementation.md` beside `spec.md`. Every deviation is either approved, and `spec.md` amended to match, or sent back. Nothing on the wire beyond what the spec approved.
