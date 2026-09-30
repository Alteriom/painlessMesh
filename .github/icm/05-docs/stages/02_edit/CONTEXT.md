# 02_edit — correct the documents

One job: every document in the approved list says what its owner says, or points at it, in one pull request.

## Inputs
- Working (this run): `.github/icm/05-docs/stages/01_locate/output/discrepancy.md`
- Reference (every run): `.github/icm/_shared/docs-and-changelog.md`

Do NOT load: the other pipelines, `.github/icm/_shared/release.md`, other runs' output, the source tree beyond the owner lines `discrepancy.md` cites.

## Process
1. Make each change `discrepancy.md` lists, and nothing else. Leave released `CHANGELOG.md` sections alone.
2. Where an anchor or heading changed, find its links (`git grep -n "<anchor>"`) and fix them.
3. Documentation needs no version bump (`CONTRIBUTING.md` "Versioning").
4. Write the pull request: each claim, what it said, what owns the fact (`path:line`), what it says now. Title: `docs(<area>): <what is now correct>`.
5. After the human check, the agent opens it against `main` from this file (`gh pr create --base main --title "<title>" --body-file <this stage's output/pull-request.md>`). The agent never merges.

## Outputs
- pull-request.md → output/ — the title and body, and the list of files changed.

## Human check
A reviewer reads the diff beside the owner lines in `discrepancy.md`, then says whether to open the pull request; the reviewer merges it.
