# Releasing — pointers and the parts that bite

The procedure is `RELEASE_GUIDE.md`; read it whole before any release stage. This file adds the chain of workflows, the places the version lives, and what the history shows goes wrong.

## Semver, in this repository's terms

`CONTRIBUTING.md` "Versioning": major for a changed or new protocol message or a removed API; minor for a backwards-compatible addition; patch for a fix. Every 2.x patch stays wire-compatible with 2.0. Documentation alone needs no version. A published version is never reused (`RELEASE_GUIDE.md` "Recovery").

## Where the version lives

- `./scripts/bump-version.sh <patch|minor|major> [X.Y.Z] [--yes]` updates and cross-checks six files: `library.properties`, `library.json`, `package.json`, `package-lock.json`, `src/AlteriomPainlessMesh.h`, `doxygen/Doxyfile`. It changes only the package's own version, never a dependency range (`test/ci/test_bump_version.sh` holds that).
- Two more are edited by hand: `CLAUDE.md` "Version" (the 2.1.0 and 2.1.1 release commits did) and `README.md` "Latest Release" (last updated for 2.0.3). Before opening the release PR: `git grep -n "<previous version>"` and decide each hit.

## From merge to registries

1. The release PR merges to `main`. It releases only if the push changes a version file or its head commit title starts with `release:` — so title the merge `release: vX.Y.Z — <one-line outcome>`, the shape every recent release used.
2. `CI/CD Pipeline` (`.github/workflows/ci.yml`) passes on `main`.
3. `Hardware validation on the farm` (`farm-hil.yml`) dispatches the rig and waits for its verdict; without `FARM_DISPATCH_TOKEN` it passes having sent nothing, and nothing releases.
4. `Automated Release` (`release.yml`) runs only after step 3 succeeded on `main`: it checks out the revision the rig passed, tags `vX.Y.Z`, creates the GitHub release with the Arduino archive, publishes npm and GitHub Packages, and dispatches `platformio-publish.yml`.

Validation before merging: `./scripts/validate-release.sh` and `./scripts/release-agent.sh` (they check version agreement, the tag not existing yet, and a dated changelog section).

## Repairing a publication

Releases are immutable once created. A registry that failed is re-published for the same tag with `manual-publish.yml` (npm, GitHub Packages) or `platformio-publish.yml`, both dispatched with the tag — never by re-running `release.yml`, never by moving the tag. Defective code gets a new patch version. The PlatformIO package is `sparck75/AlteriomPainlessMesh` (#440, #444); Arduino Library Manager indexing lags the GitHub release. Suspected credential exposure: `SECURITY.md`.
