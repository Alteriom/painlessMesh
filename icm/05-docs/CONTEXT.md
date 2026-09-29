# 05-docs — the pipeline

The flow in one line: find what the owning file says, then make the document say it or point at it.

| Stage | Job | Input | Output | Human check |
|---|---|---|---|---|
| [`01_locate`](stages/01_locate/CONTEXT.md) | find the owner of each claim | the report | `output/discrepancy.md` | A person opens each cited owner line and confirms it says what `discrepancy.md` claims, and agrees which copies become pointers. |
| [`02_edit`](stages/02_edit/CONTEXT.md) | correct the documents | 01's output | `output/pull-request.md` | A reviewer reads the diff beside the owner lines in `discrepancy.md`, then says whether to open the pull request; the reviewer merges it. |

Each row's Human check is its contract's, word for word, and so is that stage's `human_check` in `icm.source.json`: change the contract, then make both match it.

Factory (every run): `../_shared/` — each contract names the files it loads.
Product (each run): each stage's `output/`, which is not committed (`icm/.gitignore`).

Status is whatever exists: a stage is complete when its `output/` holds the artifact above.

One run per worktree or branch: `output/` belongs to the checkout, so two runs in one checkout overwrite each other. When the run's pull request merges or is abandoned, clear this pipeline's `stages/*/output/`; a finished run left in place still reads as in progress.
