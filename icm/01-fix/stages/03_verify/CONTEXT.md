# 03_verify — checks, changelog, pull request

One job: the pull request a reviewer can merge, with the evidence CI and the rig will look for. Running the checks and writing the changelog entry are its steps, not separate jobs.

## Inputs
- Working (this run): ../02_fix/output/change.md
- Reference (every run): ../../../_shared/build-and-test.md
- Reference (every run): ../../../_shared/docs-and-changelog.md

Do NOT load: `02-feature/`, `03-release/`, `04-deps/`, `05-docs/`, `_shared/release.md`, other runs' output.

## Process
1. Run what CI runs that the change can affect ([build-and-test.md](../../../_shared/build-and-test.md)): the desktop builds of `build-test-desktop`; the test point for gateway code; the example sketches and PlatformIO projects that include the changed files; the `code-quality` checks.
2. Decide the hardware evidence: radio, routing, gateway, OTA or timing means a rig run (ask a maintainer for the `run-hil` label) or a serial log. Say which, or why none is needed. A rig result counts only with its link to the farm run.
3. Add the entry under `## [Unreleased]` in `CHANGELOG.md` (`Fixed`), in the house style of [docs-and-changelog.md](../../../_shared/docs-and-changelog.md). Update any document that described the old behaviour.
4. Write the pull request: what was wrong, how you know (the reproduction), what the change does; the checks run and their results; the hardware evidence or its request. Title: `fix(<area>): <the outcome> (#<issue>)`.
5. After the human check, the agent opens it against `main` from this file (`gh pr create --base main --title "<title>" --body-file <this stage's output/pull-request.md>`). The agent never merges.

## Outputs
- pull-request.md → output/ — the title and body, the checks run with results, the changelog entry as written, and links to any rig run or log.

## Human check
A person reads `pull-request.md` and its evidence, then says whether to open it. For radio, routing, gateway or OTA the linked `farm/hil` run or a serial log is there, or the fix does not merge. The reviewer, not the author, merges.
