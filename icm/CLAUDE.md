# painlessMesh work

The three jobs that recur on this repository — fixing a defect, adding a capability, releasing a version — each as a pipeline whose stages stop at a person's check. What leaves this workspace is a reviewed pull request or a published version.

Built on ICM: folders carry sequencing, hierarchy carries context, files carry state. The repository's own files hold the facts; this workspace points at them (one home per fact). A path in backticks that does not start with a job folder or `_shared/` is from the repository root. `AGENTS.md` beside each `CLAUDE.md` is a byte copy of it; edit `CLAUDE.md` and copy.

## Jobs

| Job | Pipeline | Starts from |
|---|---|---|
| Fix a defect | [01-fix](01-fix/CLAUDE.md) | an issue, a failing test, a serial log |
| Add a capability | [02-feature](02-feature/CLAUDE.md) | a request for new behaviour, API or package type |
| Release a version | [03-release](03-release/CLAUDE.md) | user-visible changes waiting under `## [Unreleased]` in `CHANGELOG.md` |

Reference every job loads, and which stage loads what: [_shared/CONTEXT.md](_shared/CONTEXT.md).

## Route by what just happened

| If | Go to | Then stop at |
|---|---|---|
| a defect is reported (issue, failing test, serial log) | `01-fix/stages/01_reproduce/CONTEXT.md` | a person has seen the reproduction fail |
| a new capability or package type is asked for | `02-feature/stages/01_spec/CONTEXT.md` | a person approves the spec |
| a version is to be released | `03-release/stages/01_plan/CONTEXT.md` | a person approves the version number |
| a stage's output was approved | next numbered stage of the same pipeline | a person reads that stage's output |
| asked for status | scan `*/stages/*/output/` | report what exists |
| asked which package type number to use | `_shared/package-types.md` | the answer, with the file and line that proves it |

## Rules

- Nothing moves to the next stage until a person has read the output of the last one.
- A pipeline reads only its own runs' output. A fix that turns out to need a wire or API change stops and starts over in `02-feature`.
- When this workspace and the repository disagree, the repository is right: correct the workspace in the same pull request.
