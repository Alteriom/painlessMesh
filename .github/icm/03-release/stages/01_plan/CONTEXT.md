# 01_plan — choose what ships and its number

One job: a release plan that names the version, justifies its class, and drafts the dated changelog section.

## Inputs
- Working (this run): the unreleased changes — `## [Unreleased]` in `CHANGELOG.md`, and `main` since the previous `v*` tag (`git log --oneline <previous tag>..origin/main`).
- Reference (every run): `.github/icm/_shared/release.md`
- Reference (every run): `RELEASE_GUIDE.md` — read it whole.

Do NOT load: the other pipelines, other runs' output, released sections of CHANGELOG.md beyond the previous one.

## Process
1. This pipeline releases from `main`, the only branch the release chain runs on. A release from a `release/<major>.x` line is outside it: stop and ask the maintainer ([release.md](../../../_shared/release.md) "Releasing from a line other than `main`").
2. List every merged change since the previous tag and match each to an `[Unreleased]` entry. A user-visible change with no entry is a gap to fill now; an entry with no change is a mistake to remove.
3. Classify against `CONTRIBUTING.md` "Versioning" (pointer in [release.md](../../../_shared/release.md)): any wire or public-API break makes it major; any addition, minor; else patch. Say which entry decides it.
4. Confirm the tag does not exist (`git ls-remote --tags origin "v<version>"`) and that `main`'s head has passed CI and the rig, or name what is still pending. A green `Hardware validation on the farm` is proof only with its "Rig run" link; check as [release.md](../../../_shared/release.md) "Is it proof that the rig ran?" says, and record the link.
5. Draft the dated section `## [X.Y.Z] - YYYY-MM-DD`: a lead paragraph (who should upgrade and why), then `Added` / `Changed` / `Fixed`, keeping the entries' wording unless it is wrong.
6. Name the branch (`release/X.Y.Z`, no `v`) and the merge title (`release: vX.Y.Z — <one-line outcome>`).

## Outputs
- release-plan.md → output/ — the version and the entry that decides its class; the changes included; CI and rig state of `main`, with the rig run's link; the drafted changelog section; branch and merge title.

## Human check
A maintainer approves the version number and the changelog wording. The number is permanent once tagged: a mistake later costs a new version, never a retag.
