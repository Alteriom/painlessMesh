# 01-fix — the pipeline

Make it fail, make it pass for the right reason, prove it everywhere CI will.

| Stage | Job | Input | Output | Human check |
|---|---|---|---|---|
| [`01_reproduce`](stages/01_reproduce/CONTEXT.md) | reproduce the reported defect | the report | `output/reproduction.md` | A person runs the command, or reads the log, and confirms it fails for the reported reason and not a neighbouring one. |
| [`02_fix`](stages/02_fix/CONTEXT.md) | change the cause, not the symptom | 01_reproduce's `reproduction.md` | `output/change.md` | A person reads the diff beside the reproduction: it removes the cause, not only the symptom the test sees, and no wire or API change slipped in. Edit `change.md` if the explanation is wrong; stage 03 reads it. |
| [`03_verify`](stages/03_verify/CONTEXT.md) | checks, changelog, pull request | 02_fix's `change.md` | `output/pull-request.md` | A person reads `pull-request.md` and its evidence, then says whether to open it. For radio, routing, gateway or OTA the linked `farm/hil` run or a serial log is there, or the fix does not merge. The reviewer, not the author, merges. |
