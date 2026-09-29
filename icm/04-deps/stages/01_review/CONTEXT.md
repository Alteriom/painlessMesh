# 01_review — what the bump changes

One job: a review of one dependency bump that says what changes, what it can break, and whether it is safe to merge now.

## Inputs
- Working (this run): the bump the person names, read whole — the pull request (`gh pr view <n> --comments` and `gh pr diff <n>`) with the upstream release notes it links, or the pin a person wants to move.
- Reference (every run): ../../../_shared/gotchas.md
- Reference (every run): ../../../_shared/release.md
- Reference (only when `@alteriom/mqtt-schema` moves): ../../../_shared/package-types.md

Do NOT load: `01-fix/`, `02-feature/`, `03-release/`, `05-docs/`, other runs' output, the source tree beyond what the bump touches.

## Process
1. Say which pin moves and who moves it. Dependabot covers npm (`package.json` devDependencies), the `Dockerfile` base image and GitHub Actions, weekly, grouping minor and patch updates (`.github/dependabot.yml`). It does not cover the `test/ArduinoJson` and `test/TaskScheduler` submodules, the libraries `ci.yml` job `build-arduino` installs, `library.json` `dependencies`, `library.properties` `depends`, or the platform pinned in `examples/basic/platformio.ini`; a bump there is by hand.
2. `@alteriom/mqtt-schema`: compare the type numbers of the old and new versions with the code ([package-types.md](../../../_shared/package-types.md)). A number that moved is a wire change and goes to `02-feature`, not a merge.
3. The ESP32 platform pin: [gotchas.md](../../../_shared/gotchas.md) says why it stays on the gnu++11 core; a bump that moves it is not routine.
4. GitHub Actions: a bump must not rename a workflow or change the `workflows:` a `workflow_run` listens to ([gotchas.md](../../../_shared/gotchas.md) "Workflows").
5. The release chain: a merge that changes `package.json` counts as a version bump ([release.md](../../../_shared/release.md) "From merge to registries"). Check whether `library.properties`' version is already tagged (`git ls-remote --tags origin "v<version>"`); if not, merging this releases that version, and it waits for the release instead.
6. Read the pull request's checks. Say which ones exercise the bump (a devDependency used only by `npm run format` is exercised by nothing in CI) and what is still unexercised.

## Outputs
- review.md → output/ — the pin that moves, from and to; what upstream changed that matters here; the wire, build and release-chain findings from steps 2–5; the checks that exercised it; merge now, merge after something, or decline, with the reason.

## Human check
A person reads `review.md` and decides: merge, wait, or close the bump. A wire change found in step 2 or an untagged version found in step 5 is never merged from here.
