# painlessMesh work

The jobs that recur on this repository — fixing a defect, adding a capability, releasing a version, reviewing a dependency bump, correcting a document — each as a pipeline whose stages stop at a person's check. What leaves this workspace is a reviewed pull request or a published version.

Built on ICM: folders carry sequencing, hierarchy carries context, files carry state. The repository's own files hold the facts; this workspace points at them (one home per fact). A path in backticks that does not start with a job folder or `_shared/` is from the repository root. Each `AGENTS.md` here is a one-line pointer to the `CLAUDE.md` beside it.

## Jobs

| Job | Pipeline | Starts from |
|---|---|---|
| Fix a defect | [01-fix](01-fix/CLAUDE.md) | an issue, a failing test, a serial log, a red run on `main` |
| Add a capability | [02-feature](02-feature/CLAUDE.md) | a request for new behaviour, API or package type |
| Release a version | [03-release](03-release/CLAUDE.md) | user-visible changes waiting under `## [Unreleased]` in `CHANGELOG.md` |
| Review a dependency bump | [04-deps](04-deps/CLAUDE.md) | a Dependabot pull request, a submodule or toolchain pin |
| Correct a document | [05-docs](05-docs/CLAUDE.md) | a document that disagrees with the code, a workflow or a script |

Shared reference, one file per question: [_shared/CONTEXT.md](_shared/CONTEXT.md). Each stage's contract names the files it loads.

## Route by what just happened

The Route column is the id a client binds to; keep it when rewording a row.

| Route | If | Go to | Then stop at |
|---|---|---|---|
| triage | an issue arrives and its kind is unclear | read it whole (`gh issue view <n> --comments`) and take the row below that fits | the row chosen, said to the person |
| fix | a defect is reported (issue, failing test, serial log) | `01-fix/stages/01_reproduce/CONTEXT.md` | a person has seen the reproduction fail |
| ci-red | `CI/CD Pipeline` or `Hardware validation on the farm` is red on `main` | `01-fix/stages/01_reproduce/CONTEXT.md`, with the run log as the report | a person has seen the reproduction fail |
| security | a report may be a vulnerability | the repository root's SECURITY.md, "Reporting a Vulnerability": a private advisory, never a public issue, reproduction or pull request | the maintainer has the private report |
| feature | a new capability or package type is asked for | `02-feature/stages/01_spec/CONTEXT.md` | a person approves the spec |
| fix-to-feature | a fix turns out to need a wire or public-API change | `02-feature/stages/01_spec/CONTEXT.md`, with the fix run's `reproduction.md` as the request | a person approves the spec |
| release | a version is to be released | `03-release/stages/01_plan/CONTEXT.md` | a person approves the version number |
| registry-repair | a released version is missing from a registry | `03-release/stages/03_publish/CONTEXT.md`, from step 3, for that tag | a person has seen the version in that registry |
| deps | a dependency bump is proposed | `04-deps/stages/01_review/CONTEXT.md` | a person decides to merge it or not |
| docs | a document is wrong or missing and no code has to change | `05-docs/stages/01_locate/CONTEXT.md` | a person confirms each owning file |
| next | a stage's output was approved | next numbered stage of the same pipeline | a person reads that stage's output |
| status | asked for status | scan `*/stages/*/output/` | report what exists |
| package-type | asked which package type number to use | `_shared/package-types.md` | the answer, with the file and line that proves it |

## Rules

- Nothing moves to the next stage until a person has read the output of the last one.
- A pipeline reads only its own runs' output; a handoff to another pipeline is a route above, started by the person.
- When this workspace and the repository disagree, the repository is right: correct the workspace in the same pull request.
