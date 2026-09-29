# 01_plan — choose what ships and its number

One job: a release plan that names the version, justifies its class, and drafts the dated changelog section.

## Inputs
- Working (this run): `CHANGELOG.md` `## [Unreleased]`, and `main` since the previous `v*` tag (`git log --oneline <previous tag>..origin/main`).
- Reference (every run): ../../../_shared/release.md
- Reference (every run): `RELEASE_GUIDE.md` at the repository root — read it whole.

Do NOT load: `01-fix/`, `02-feature/`, other runs' output, released sections of `CHANGELOG.md` beyond the previous one.

## Process
1. List every merged change since the previous tag and match each to an `[Unreleased]` entry. A user-visible change with no entry is a gap to fill now; an entry with no change is a mistake to remove.
2. Classify with [release.md](../../../_shared/release.md) "Semver": any wire or public-API break makes it major; any addition, minor; else patch. Say which entry decides it.
3. Confirm the tag does not exist (`git ls-remote --tags origin "v<version>"`) and that `main`'s head has passed CI and the rig (`farm/hil`), or name what is still pending.
4. Draft the dated section `## [X.Y.Z] - YYYY-MM-DD`: a lead paragraph (who should upgrade and why), then `Added` / `Changed` / `Fixed`, keeping the entries' wording unless it is wrong.
5. Name the branch (`release/X.Y.Z`, no `v`) and the merge title (`release: vX.Y.Z — <one-line outcome>`).

## Outputs
- release-plan.md → output/ — the version and the entry that decides its class; the changes included; CI and rig state of `main`; the drafted changelog section; branch and merge title.

## Human check
A maintainer approves the version number and the changelog wording. The number is permanent once tagged: a mistake later costs a new version, never a retag.
