# 01_locate — find the owner of each claim

One job: list every claim the correction touches, beside the file and line that owns the fact.

## Inputs
- Working (this run): the report — read whole: an issue (`gh issue view <n> --comments`), a review comment, a finding from another run.
- Reference (every run): `.github/icm/_shared/docs-and-changelog.md`
- Reference (every run): `.github/icm/_shared/architecture.md`
- Reference (only when the claim is a package type number): `.github/icm/_shared/package-types.md`

Do NOT load: the other pipelines, `.github/icm/_shared/release.md`, other runs' output.

## Process
1. For each claim, find the owner: the code for behaviour and numbers, `.github/workflows/` for what CI and the release chain do, `scripts/` for what a script changes, `CONTRIBUTING.md` and `RELEASE_GUIDE.md` for policy. Cite it as `path:line`.
2. Search for the same claim elsewhere (`git grep -n`), across `README.md`, `USER_GUIDE.md`, `docsify-site/` (including `wiki/`), `.github/copilot-instructions.md`, `CLAUDE.md` and `.github/icm/`; every copy is in this run.
3. For each copy decide: correct it, or delete it and point at the owner. A document that keeps a number, a version or a file list by hand drifts again; prefer the pointer.
4. If the owner itself looks wrong (the code does not do what every document says it should), stop: that is a defect for `01-fix`, not a correction.

## Outputs
- discrepancy.md → output/ — one row per claim: document and line, what it says, the owner and line, what it should say or where it should point.

## Human check
A person opens each cited owner line and confirms it says what `discrepancy.md` claims, and agrees which copies become pointers.
