# 03-release — the pipeline

The flow in one line: choose the number, prepare it in one reviewed pull request, watch it reach every registry.

| Stage | Job | Input | Output | Human check |
|---|---|---|---|---|
| [`01_plan`](stages/01_plan/CONTEXT.md) | choose what ships and its number | `CHANGELOG.md` `[Unreleased]`, `main` | `output/release-plan.md` | A maintainer approves the version number and the changelog wording. The number is permanent once tagged: a mistake later costs a new version, never a retag. |
| [`02_prepare`](stages/02_prepare/CONTEXT.md) | the release pull request | 01's output | `output/release-pr.md` | An approving review of the final commit, every conversation resolved, every required check green (`RELEASE_GUIDE.md` "Review gate" is the list). The person merges with the `release: vX.Y.Z — …` title; any code change after approval needs approval again. |
| [`03_publish`](stages/03_publish/CONTEXT.md) | follow it to every registry | 02's output, or a tag to repair | `output/publication.md` | A person confirms the version is visible in each registry before it is announced, and reads any repair that was needed. |

Each row's Human check is its contract's, word for word, and so is that stage's `human_check` in `icm.source.json`: change the contract, then make both match it.

Factory (every run): `../_shared/release.md`, and `RELEASE_GUIDE.md` at the repository root.
Product (each run): each stage's `output/`, which is not committed (`.github/icm/.gitignore`).

Status is whatever exists: a stage is complete when its `output/` holds the artifact above.

One run per worktree or branch: `output/` belongs to the checkout, so two runs in one checkout overwrite each other. When the run's pull request merges or is abandoned, clear this pipeline's `stages/*/output/`; a finished run left in place still reads as in progress.
