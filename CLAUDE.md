# painlessMesh

Alteriom's fork of painlessMesh: a C++ mesh networking library for ESP32 and ESP8266 (Arduino, PlatformIO). What leaves this repository is a reviewed pull request or a released version in every registry.

## Where things live

| Path | What it holds |
|---|---|
| `src/`, `examples/`, `test/` | the library, one sketch per folder, the desktop tests — mapped in [architecture.md](.github/icm/_shared/architecture.md) |
| `.github/workflows/` | CI, the hardware rig, release and publication |
| `CONTRIBUTING.md`, `RELEASE_GUIDE.md`, `SECURITY.md` | branches and versioning; the release procedure; the threat model and private reporting |
| [`.github/icm/_shared/`](.github/icm/_shared/CONTEXT.md) | what every job below draws on, one file per question |
| [`.github/icm/01-fix/`](.github/icm/01-fix/CLAUDE.md) | fix a defect |
| [`.github/icm/02-feature/`](.github/icm/02-feature/CLAUDE.md) | add a capability: an API, option, package type or example |
| [`.github/icm/03-release/`](.github/icm/03-release/CLAUDE.md) | release a version |
| [`.github/icm/04-deps/`](.github/icm/04-deps/CLAUDE.md) | review a dependency bump |
| [`.github/icm/05-docs/`](.github/icm/05-docs/CLAUDE.md) | correct a document that disagrees with the code |

## Route by what just happened

| Route | If | Go to | Then stop at |
|---|---|---|---|
| triage | an issue arrives and its kind is unclear | read it whole (`gh issue view <n> --comments`) and take the row below that fits | the row chosen, said to the person |
| fix | a defect is reported (issue, failing test, serial log) | [01_reproduce](.github/icm/01-fix/stages/01_reproduce/CONTEXT.md) | a person has seen the reproduction fail |
| ci-red | `CI/CD Pipeline` or `Hardware validation on the farm` is red on `main` | [01_reproduce](.github/icm/01-fix/stages/01_reproduce/CONTEXT.md), with the run log as the report | a person has seen the reproduction fail |
| security | a report may be a vulnerability | [SECURITY.md](SECURITY.md) "Reporting a Vulnerability": a private advisory, never a public issue, reproduction or pull request | the maintainer has the private report |
| feature | a new capability or package type is asked for | [01_spec](.github/icm/02-feature/stages/01_spec/CONTEXT.md) | a person approves the spec |
| fix-to-feature | a fix turns out to need a wire or public-API change | [01_spec](.github/icm/02-feature/stages/01_spec/CONTEXT.md), with the fix run's `reproduction.md` as the request | a person approves the spec |
| release | a version is to be released | [01_plan](.github/icm/03-release/stages/01_plan/CONTEXT.md) | a person approves the version number |
| registry-repair | a released version is missing from a registry | [03_publish](.github/icm/03-release/stages/03_publish/CONTEXT.md), from step 3, for that tag | a person has seen the version in that registry |
| deps | a dependency bump is proposed | [01_review](.github/icm/04-deps/stages/01_review/CONTEXT.md) | a person decides to merge it or not |
| docs | a document is wrong or missing and no code has to change | [01_locate](.github/icm/05-docs/stages/01_locate/CONTEXT.md) | a person confirms each owning file |
| next | a stage's output was approved | the next numbered stage of the same job | a person reads that stage's output |
| status | asked for status | scan `.github/icm/*/stages/*/output/` | report what exists |
| package-type | asked which package type number to use | [package-types.md](.github/icm/_shared/package-types.md) | the answer, with the file and line that proves it |

## Rules

- Nothing moves to the next stage until a person has read the output of the last one.
- A stage writes to its own `output/`, which is not committed: one run per worktree or branch, and clear that job's `stages/*/output/` when its pull request merges or is abandoned.
- A job reads only its own run's output; moving to another job is a route above, started by the person.
- `.github/icm/` points at the files that own each fact. When they disagree, the owning file is right: correct `.github/icm/` in the same pull request.
