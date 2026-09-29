# 03-release — release a version

One run is one version, from the changes under `## [Unreleased]` to the same number visible in every registry. The steps that are the person's decision (the number, the merge) are stage boundaries; the rest follows `RELEASE_GUIDE.md`, which this pipeline never restates.

Stages, in order — the table with inputs, outputs and checks is [CONTEXT.md](CONTEXT.md):

1. [Plan](stages/01_plan/CONTEXT.md) — what ships, and which number.
2. [Prepare](stages/02_prepare/CONTEXT.md) — the release pull request.
3. [Publish](stages/03_publish/CONTEXT.md) — follow the workflows to every registry.

Stop after each stage until a person has read its output. A published version is never reused.
