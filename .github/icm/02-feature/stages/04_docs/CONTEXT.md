# 04_docs — documents, changelog, pull request

One job: every document a user reads says what the code now does, and the pull request carries the evidence.

## Inputs
- Working (this run): `.github/icm/02-feature/stages/01_spec/output/spec.md` — as amended at stage 02's human check, so it describes what was built, deviations included.
- Working (this run): `.github/icm/02-feature/stages/03_test/output/test-report.md`
- Reference (every run): `.github/icm/_shared/docs-and-changelog.md`

Do NOT load: the other pipelines, `.github/icm/_shared/release.md`, other runs' output, the source files beyond the public declarations.

## Process
1. Use the table in [docs-and-changelog.md](../../../_shared/docs-and-changelog.md) to list every document this capability touches; update each (API pages, `USER_GUIDE.md`, `README.md` "Message Types" for a new type, `keywords.txt`, Doxygen comments on the new declarations).
2. Add the `CHANGELOG.md` entry under `## [Unreleased]` (`Added`, or `Changed`), naming the feature-test macro.
3. Write the pull request: the need, the approved spec (summarised, linked), what was built, the tests and their evidence, the rig request. Title: `feat(<area>): <the capability>`.
4. After the human check, the agent opens it against `main` from this file (`gh pr create --base main --title "<title>" --body-file <this stage's output/pull-request.md>`). The agent never merges.

## Outputs
- pull-request.md → output/ — title and body, the list of documents changed, the changelog entry as written.

## Human check
A maintainer reads the changed documents against the behaviour in `test-report.md`, then `pull-request.md`, and says whether to open it. The reviewer merges; a code change after approval needs approval again.
