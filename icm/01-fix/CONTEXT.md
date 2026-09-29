# 01-fix — the pipeline

The flow in one line: make it fail, make it pass for the right reason, prove it everywhere CI will.

| Stage | Job | Input | Output | Human check |
|---|---|---|---|---|
| [`01_reproduce`](stages/01_reproduce/CONTEXT.md) | reproduce the reported defect | the report | `output/reproduction.md` | A person runs the command, or reads the log, and confirms it fails for the reported reason and not a neighbouring one. |
| [`02_fix`](stages/02_fix/CONTEXT.md) | change the cause, not the symptom | 01's output | `output/change.md` | A person reads the diff beside the reproduction: it removes the cause, not only the symptom the test sees, and no wire or API change slipped in. Edit `change.md` if the explanation is wrong; stage 03 reads it. |
| [`03_verify`](stages/03_verify/CONTEXT.md) | checks, changelog, pull request | 02's output | `output/pull-request.md` | A person reads `pull-request.md` and its evidence, then says whether to open it. For radio, routing, gateway or OTA the linked `farm/hil` run or a serial log is there, or the fix does not merge. The reviewer, not the author, merges. |

Each row's Human check is its contract's, word for word, and so is that stage's `human_check` in `icm.source.json`: change the contract, then make both match it.

Factory (every run): `../_shared/` — each contract names the files it loads.
Product (each run): each stage's `output/`, which is not committed (`icm/.gitignore`).

Status is whatever exists: a stage is complete when its `output/` holds the artifact above.

One run per worktree or branch: `output/` belongs to the checkout, so two runs in one checkout overwrite each other. When the run's pull request merges or is abandoned, clear this pipeline's `stages/*/output/`; a finished run left in place still reads as in progress.
