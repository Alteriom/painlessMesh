# 03_verify — checks, changelog, pull request

One job: prove the fix everywhere CI and the rig will look, and write the pull request a reviewer can merge.

## Inputs
- Working (this run): ../02_fix/output/change.md
- Reference (every run): ../../../_shared/build-and-test.md
- Reference (every run): ../../../_shared/docs-and-changelog.md

Do NOT load: `02-feature/`, `03-release/`, `_shared/release.md`, other runs' output.

## Process
1. Run what CI runs that the change can affect ([build-and-test.md](../../../_shared/build-and-test.md)): gcc, clang and ASan desktop builds; the test point for gateway code; the example sketches and PlatformIO projects that include the changed files; the `code-quality` checks.
2. Decide the hardware evidence: radio, routing, gateway, OTA or timing means a rig run (ask a maintainer for the `run-hil` label) or a serial log. Say which, or why none is needed.
3. Add the entry under `## [Unreleased]` in `CHANGELOG.md` (`Fixed`), in the house style of [docs-and-changelog.md](../../../_shared/docs-and-changelog.md). Update any document that described the old behaviour.
4. Write the pull request: what was wrong, how you know (the reproduction), what the change does; the checks run and their results; the hardware evidence or its request. Title: `fix(<area>): <the outcome> (#<issue>)`.

## Outputs
- pull-request.md → output/ — the title and body, the checks run with results, the changelog entry as written, and links to any rig run or log.

## Human check
A maintainer reads the pull request and its evidence. For radio, routing, gateway or OTA the `farm/hil` status or a serial log is linked, or the fix does not merge. The reviewer, not the author, merges.
