# 03_publish — follow it to every registry

One job: see the merged version through CI, the rig and the release workflow, and confirm it is visible in every registry.

## Inputs
- Working (this run): ../02_prepare/output/release-pr.md — or, for a repair of an earlier release (route `registry-repair`), only the tag `vX.Y.Z` the person names; start at step 3.
- Reference (every run): ../../../_shared/release.md

Do NOT load: `01-fix/`, `02-feature/`, `04-deps/`, `05-docs/`, other runs' output, the source tree.

## Process
1. Follow the chain in [release.md](../../../_shared/release.md) "From merge to registries" on the merge commit: `CI/CD Pipeline`, then `Hardware validation on the farm`, then `Automated Release` (`gh run list --workflow <file> --branch main`). Record each run's link and result, and the farm job's "Rig run" link. A "No rig run" notice means the version was tagged without the rig: tell the person at once, before anything is announced.
2. If the rig or CI fails, stop: nothing was released, and the fix is a new `01-fix` run followed by a new release plan — never a re-run that skips the gate.
3. Check each destination for `X.Y.Z`: the `vX.Y.Z` tag and GitHub release with the Arduino archive; npm; GitHub Packages; the PlatformIO registry; Arduino Library Manager (which lags).
4. A destination that failed is repaired as "Repairing a publication" says, for the same tag. Record what was re-run and why.

## Outputs
- publication.md → output/ — each workflow run with its result; each destination with the version seen and a link; anything repaired.

## Human check
A person confirms the version is visible in each registry before it is announced, and reads any repair that was needed.
