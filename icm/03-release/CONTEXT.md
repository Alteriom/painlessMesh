# 03-release — the pipeline

The flow in one line: choose the number, prepare it in one reviewed pull request, watch it reach every registry.

| Stage | Job | Input | Output | Human check |
|---|---|---|---|---|
| `01_plan` | choose what ships and its number | `CHANGELOG.md` `[Unreleased]`, `main` | `output/release-plan.md` | the version number and wording approved |
| `02_prepare` | the release pull request | 01's output | `output/release-pr.md` | approving review; checks green on the final commit |
| `03_publish` | follow it to every registry | 02's output | `output/publication.md` | the version is visible everywhere before it is announced |

Factory (every run): `../_shared/release.md`, and `RELEASE_GUIDE.md` at the repository root.
Product (each run): each stage's `output/`, which is not committed (`icm/.gitignore`).

Status is whatever exists: a stage is complete when its `output/` holds the artifact above.
