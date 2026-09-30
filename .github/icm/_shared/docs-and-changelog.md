---
verified_at: 2026-09-30
verified_commit: 6d8e0454edf9d2f2765b1ffff42a70b7741e999f
---

# Documentation and the changelog

## Which documents a change touches

The map of every document, what it is for and where it is published is `.github/DOCUMENTATION.md`; start there. The places a code change most often has to follow:

| The change | Update |
|---|---|
| any user-visible behaviour | `CHANGELOG.md` under `## [Unreleased]` |
| public API (a method, callback, option, macro) | `USER_GUIDE.md`, the matching page under `docsify-site/api/`, the Doxygen comment on the declaration, `keywords.txt` |
| a package type added or renumbered | `README.md` "Message Types"; `docsify-site/alteriom/packages.md` when it is an application package |
| bridge, gateway, failover, shared gateway, offline queue | `BRIDGE_TO_INTERNET.md` |
| a new example | its own `README.md`, `library.json` `examples`, the table in `.github/DOCUMENTATION.md`; simulator scenarios under `examples/<example>/test/simulator/` |
| a threat, an OTA or credential behaviour | `SECURITY.md` |
| a build or test command | `CONTRIBUTING.md`, and `.github/workflows/ci.yml` if CI should run it |

The site is rebuilt from `docsify-site/` and `src/` by `.github/workflows/docs.yml` on a push to `main`; nothing is edited on the published site.

## A changelog entry

Format and rules: `RELEASE_GUIDE.md` "Changelog" (Keep a Changelog; `Added` / `Changed` / `Fixed`). Read the `2.1.1` section of `CHANGELOG.md` for the house style:

- Lead with the observed defect or the new ability in the user's terms, then its impact, then what changed. Name the issue (#466).
- Say who has to act ("**Upgrade if you run 2.1.0**") and which macro or flag lets code test for the change.
- Operational evidence (logs, rig runs) goes in the pull request; the changelog keeps the user-facing impact.
- Released sections are never edited.

## The pull request

`CONTRIBUTING.md` "Submit a pull request": what was wrong, how you know (a log, a test, a measurement), what the change does about it. `.github/PULL_REQUEST_TEMPLATE.md` supplies the checklist. The reviewer merges.
