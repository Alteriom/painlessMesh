# Shared reference

Factory content: stable across runs, loaded by the stages named here and by nothing else. Each file points at the repository file that owns a fact rather than restating it; follow the pointer when you need the detail.

| File | Answers | Loaded by |
|---|---|---|
| [architecture.md](architecture.md) | where each part of the library lives | fix 01, feature 01 |
| [build-and-test.md](build-and-test.md) | the commands, the test levels, and what counts as evidence | fix 01, fix 03, feature 03 |
| [package-types.md](package-types.md) | which type numbers exist, where they are defined, how to pick one | feature 01 (and fix 02 when a package is involved) |
| [coding-rules.md](coding-rules.md) | the rules CI enforces and the conventions it does not | fix 02, feature 02 |
| [gotchas.md](gotchas.md) | traps that have already cost a release, each with its source | fix 01, fix 02, feature 02 |
| [docs-and-changelog.md](docs-and-changelog.md) | which documents a change touches, and how a changelog entry reads | fix 03, feature 04 |
| [release.md](release.md) | how a version is chosen, prepared and published, and how it is repaired | every release stage |

Do not load this whole folder into a stage. A stage's contract names the files it needs.
