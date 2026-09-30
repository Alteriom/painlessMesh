# 04-deps — review a dependency bump

One run is one proposed bump — usually a Dependabot pull request — reviewed for what it changes on the wire, in the build and in the release chain, so a person can decide to merge it. Dependency bumps are the second most common change on `main` (`chore(deps)` commits), and most are routine; this pipeline exists for the few that are not.

Stages, in order; inputs, outputs and checks for each are in [CONTEXT.md](CONTEXT.md):

1. [Review](stages/01_review/CONTEXT.md) — what the bump changes, and what could go wrong.
