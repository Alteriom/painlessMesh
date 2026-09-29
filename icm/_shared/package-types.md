# Package types — where the numbers live

A package's `type` is on the wire: two nodes that disagree about a number misread each other, and a number once shipped is never reused for something else. This file says where the truth is and how to pick a number; it deliberately holds no table of its own.

## Where the numbers are defined (the code is the authority)

| Range | Defined in |
|---|---|
| core protocol (3–9) | `enum Type` in `src/painlessmesh/protocol.hpp` |
| mesh-internal bridge, gateway and delivery-ack types (610–613, 620–622, 630) | `constexpr int` block in `src/painlessmesh/protocol.hpp`; `BridgeCoordinationPackage` in `src/painlessmesh/plugin.hpp` |
| application packages (200-series, 400, 600–605, 610–614) | the constructors in `examples/alteriom/alteriom_sensor_package.hpp`; `MpptPackage` (205) in `examples/alteriom/alteriom_custom_package_template.hpp` |

Find every number in use before choosing one:

```bash
grep -rnE '(Single|Broadcast)Package\([0-9]+\)|constexpr int [A-Z_]+ = [0-9]+' src examples
```

The comments beside several application types ("per mqtt-schema v0.7.2+") tie them to the `@alteriom/mqtt-schema` package (`package.json` devDependencies): a new application type that has a schema counterpart takes that number.

## The human-readable registry

`README.md` "Message Types" (protocol-level and application-level tables) is the registry users read, and the one a change that adds or renumbers a type must update in the same pull request. It matches the code as of this workspace's writing, except that `MpptPackage` (205) is in the code and not the table.

## Documents that disagree with the code — do not copy numbers from them

- `CLAUDE.md` "Package types" names 201 device announcement, 202 sensor data and 204 gateway data; the code has `SensorPackage` 200, `StatusPackage` 202, `MetricsPackage` 204.
- `.github/copilot-instructions.md`, `docsify-site/alteriom/overview.md` and `docsify-site/architecture/plugin-system.md` give `CommandPackage` 201; the code has 400.

Correcting them is a documentation fix of its own (`01-fix`), not something to do inside an unrelated change.

## Choosing a number

1. Run the grep above and read `README.md` "Message Types".
2. Stay in the range whose meaning fits (application data, mesh-internal, bridge, gateway); a new range is a design decision for the spec's human check.
3. `CONTRIBUTING.md` "Versioning" lists a new or changed protocol message as a major change. The spec says which class this change is, and why.
4. Add a round-trip test in `test/catch/` (the existing ones: `catch_alteriom_packages.cpp`, `catch_mesh_packages.cpp`).
