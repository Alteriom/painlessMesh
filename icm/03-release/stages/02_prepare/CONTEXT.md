# 02_prepare — the release pull request

One job: one pull request that changes the version everywhere, dates the changelog, and passes every check.

## Inputs
- Working (this run): ../01_plan/output/release-plan.md
- Reference (every run): ../../../_shared/release.md
- Reference (every run): `RELEASE_GUIDE.md` "Release checklist", "Version metadata", "Validation", "Review gate"

Do NOT load: `01-fix/`, `02-feature/`, other runs' output.

## Process
1. Branch `release/X.Y.Z` from current `origin/main`.
2. `./scripts/bump-version.sh <kind> X.Y.Z --yes`; then `git grep -n "<previous version>"` and update the hand-edited places [release.md](../../../_shared/release.md) names ("Where the version lives"). Leave historical mentions (changelog sections, migration notes) alone.
3. Replace `## [Unreleased]` content with the approved dated section; leave an empty `## [Unreleased]` above it.
4. Run `./scripts/validate-release.sh` and `./scripts/release-agent.sh`, and the desktop suite. Fix what they report; do not skip a check.
5. Open the pull request against `main` with the plan's title; the body is the changelog section plus the validation output.

## Outputs
- release-pr.md → output/ — the pull request link, the files changed, both scripts' output, the check state of the final commit.

## Human check
An approving review of the final commit, every conversation resolved, every required check green (`RELEASE_GUIDE.md` "Review gate" is the list). The person merges with the `release: vX.Y.Z — …` title; any code change after approval needs approval again.
