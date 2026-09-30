# Releasing — pointers and the parts that bite

The procedure is `RELEASE_GUIDE.md`; read it whole before any release stage. This file adds the chain of workflows, how to tell whether the rig really ran, and what the history shows goes wrong.

## Semver and the version files

The version classes are `CONTRIBUTING.md` "Versioning"; the files that carry the version, and why each matters, are `RELEASE_GUIDE.md` "Version metadata". Change the version only with `./scripts/bump-version.sh <patch|minor|major> [X.Y.Z] [--yes]`, which updates and cross-checks every one of them (`--help` lists them). No document carries a "current version" by hand; if `git grep -n "<previous version>"` finds one outside the changelog and migration notes, it is a `05-docs` correction, not something to keep in step. A published version is never reused (`RELEASE_GUIDE.md` "Recovery").

## From merge to registries

The chain runs on `main` only (`workflow_run` with `branches: [main]` in `farm-hil.yml` and `release.yml`).

1. The release PR merges to `main`. `release.yml` ("Capture commit message", "Set release decision") releases when the tag for `library.properties`' version does not exist yet and either the head commit title starts with `release:` or the merge changed `library.properties`, `library.json` or `package.json` — a dependency bump in `package.json` included. Title the merge `release: vX.Y.Z — <one-line outcome>`, the shape every recent release used.
2. `CI/CD Pipeline` (`.github/workflows/ci.yml`) passes on `main`.
3. `Hardware validation on the farm` (`farm-hil.yml`) dispatches the rig and waits for its verdict — when `FARM_DISPATCH_TOKEN` is set. Without it the job posts a "No rig run" notice, exits 0 and succeeds having sent nothing.
4. `Automated Release` (`release.yml`) runs when step 3 concluded `success` on `main` — which it also does after a "No rig run" — checks out the revision step 3 ran on, tags `vX.Y.Z`, creates the GitHub release with the Arduino archive, publishes npm and GitHub Packages, and dispatches `platformio-publish.yml`.

Validation before merging: `./scripts/validate-release.sh` and `./scripts/release-agent.sh` (they check version agreement, the tag not existing yet, and a dated changelog section).

## Is it proof that the rig ran?

A green `Hardware validation on the farm` is not, by itself. It is proof only when its run carries a "Rig run" link to a run of `hil-painlessmesh.yml` in the farm repository:

```bash
gh run list --workflow farm-hil.yml --branch main --commit <sha>
gh run view <run-id> --log | grep -E 'No rig run|Rig run'
```

"No rig run" means nothing was sent, and a version bump in that merge was tagged anyway. For a pull request the rig's verdict is the `farm/hil` commit status with a link to the run (`RELEASE_GUIDE.md` "How the hardware result arrives"): `gh api repos/Alteriom/painlessMesh/commits/<sha>/status --jq '.statuses[] | select(.context=="farm/hil")'`. Record the link, not the colour.

## Releasing from a line other than `main`

`CONTRIBUTING.md` "Branches" allows `release/<major>.x` lines, but the chain above does not run on them. A release from such a line is not covered by this file: stop and ask the maintainer how it is tagged and published.

## Repairing a publication

Releases are immutable once created. A registry that failed is re-published for the same tag with `manual-publish.yml` (npm, GitHub Packages; input `ref`) or `platformio-publish.yml` (input `version`) — never by re-running `release.yml`, never by moving the tag. Defective code gets a new patch version. The PlatformIO package is `sparck75/AlteriomPainlessMesh` (#440, #444); Arduino Library Manager indexing lags the GitHub release. Suspected credential exposure: `SECURITY.md`.
