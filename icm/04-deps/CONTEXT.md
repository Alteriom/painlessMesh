# 04-deps — the pipeline

The flow in one line: say what the bump changes, then let a person decide.

| Stage | Job | Input | Output | Human check |
|---|---|---|---|---|
| [`01_review`](stages/01_review/CONTEXT.md) | what the bump changes | the bump (a pull request or a pin) | `output/review.md` | A person reads `review.md` and decides: merge, wait, or close the bump. A wire change found in step 2 or an untagged version found in step 5 is never merged from here. |

Each row's Human check is its contract's, word for word, and so is that stage's `human_check` in `icm.source.json`: change the contract, then make both match it.

Factory (every run): `../_shared/` — the contract names the files it loads.
Product (each run): each stage's `output/`, which is not committed (`icm/.gitignore`).

Status is whatever exists: a stage is complete when its `output/` holds the artifact above.

One run per worktree or branch: `output/` belongs to the checkout, so two runs in one checkout overwrite each other. When the run's pull request merges or is abandoned, clear this pipeline's `stages/*/output/`; a finished run left in place still reads as in progress.
