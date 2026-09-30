# 03-release — the pipeline

Choose the number, prepare it in one reviewed pull request, watch it reach every registry.

| Stage | Job | Input | Output | Human check |
|---|---|---|---|---|
| [`01_plan`](stages/01_plan/CONTEXT.md) | choose what ships and its number | the unreleased changes | `output/release-plan.md` | A maintainer approves the version number and the changelog wording. The number is permanent once tagged: a mistake later costs a new version, never a retag. |
| [`02_prepare`](stages/02_prepare/CONTEXT.md) | the release pull request | 01_plan's `release-plan.md` | `output/release-pr.md` | An approving review of the final commit, every conversation resolved, every required check green (`RELEASE_GUIDE.md` "Review gate" is the list). The person merges with the `release: vX.Y.Z — …` title; any code change after approval needs approval again. |
| [`03_publish`](stages/03_publish/CONTEXT.md) | follow it to every registry | 02_prepare's `release-pr.md` | `output/publication.md` | A person confirms the version is visible in each registry before it is announced, and reads any repair that was needed. |
