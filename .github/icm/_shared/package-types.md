# Package types — where the numbers live

A package's `type` is on the wire: two nodes that disagree about a number misread each other, and a number once shipped is never reused for something else. This file says where the truth is and how to pick a number; it deliberately holds no table of its own.

## Where the numbers are defined (the code is the authority)

| Range | Defined in |
|---|---|
| core protocol (3–9) | `enum Type` in `src/painlessmesh/protocol.hpp` |
| OTA (10–12) | the `Announce`, `DataRequest` and `Data` constructors in `src/painlessmesh/ota.hpp` |
| optional plugins (13, 14) | `PerformancePackage` in `src/plugin/performance.hpp`, `RemotePackage` in `src/plugin/remote.hpp` |
| an example's own type (21) | `NamePackage` in `examples/namedMesh/namedMesh.ino` |
| mesh-internal bridge, gateway and delivery-ack types (610–613, 620–622, 630) | `constexpr int` block in `src/painlessmesh/protocol.hpp`; `BridgeCoordinationPackage` in `src/painlessmesh/plugin.hpp` |
| application packages (200, 202, 204, 400, 600–605, 610–612, 614) | the constructors in `examples/alteriom/alteriom_sensor_package.hpp`; `MpptPackage` (205) in `examples/alteriom/alteriom_custom_package_template.hpp` |

Find every number in use before choosing one. A constructor may take a literal, a `protocol::` constant, or pass its number through a base class (`DataRequest` is `Announce(11, …)`), so search all three shapes:

```bash
grep -rnE '(Package|Announce|DataRequest)\((protocol::)?([0-9]+|[A-Z][A-Z_]+)[,)]' src examples
grep -rnE 'constexpr int [A-Z_]+ = [0-9]+' src
```

Ignore the hits that are calls rather than definitions (`onPackage(protocol::SINGLE, …)`) and the copies under `examples/alteriom/mppt_example/`.

The comments beside several application types ("per mqtt-schema v0.7.2+") tie them to the `@alteriom/mqtt-schema` package (`package.json` devDependencies): a new application type that has a schema counterpart takes that number, and a bump of that package that renumbers a type is a wire change.

## The human-readable registry

`README.md` "Message Types" (protocol-level and application-level tables) is the registry users read, and the one a change that adds or renumbers a type must update in the same pull request. It lists every number the first grep finds in `src/` and `examples/alteriom/`; the example-only 21 is not in it. When it and the grep disagree, the grep is right and the README is a `05-docs` correction.

Any other document that states a number — `.github/copilot-instructions.md`, `docsify-site/` — is checked against the grep the same way, never copied from.

## Choosing a number

1. Run both greps and read `README.md` "Message Types".
2. Stay in the range whose meaning fits (application data, mesh-internal, bridge, gateway); a new range is a design decision for the spec's human check.
3. `CONTRIBUTING.md` "Versioning" lists a new or changed protocol message as a major change. The spec says which class this change is, and why.
4. Add a round-trip test in `test/catch/` (the existing ones: `catch_alteriom_packages.cpp`, `catch_mesh_packages.cpp`).
