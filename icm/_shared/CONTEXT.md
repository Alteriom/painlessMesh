# Shared reference

Factory content: stable across runs. Each file points at the repository file that owns a fact rather than restating it; follow the pointer when you need the detail.

| File | Answers |
|---|---|
| [architecture.md](architecture.md) | where each part of the library lives |
| [build-and-test.md](build-and-test.md) | how to build and test, which level a test belongs at, and what counts as evidence |
| [package-types.md](package-types.md) | which type numbers exist, where they are defined, how to pick one |
| [coding-rules.md](coding-rules.md) | the rules CI enforces and the conventions it does not |
| [gotchas.md](gotchas.md) | traps that have already cost a release, each with its source |
| [docs-and-changelog.md](docs-and-changelog.md) | which documents a change touches, and how a changelog entry reads |
| [release.md](release.md) | how a version is chosen, prepared and published, and how it is repaired |

Which stage loads which file is said once, in that stage's contract (its "Inputs"), and nowhere else. Do not load this whole folder into a stage.
