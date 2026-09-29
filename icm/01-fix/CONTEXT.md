# 01-fix — the pipeline

The flow in one line: make it fail, make it pass for the right reason, prove it everywhere CI will.

| Stage | Job | Input | Output | Human check |
|---|---|---|---|---|
| `01_reproduce` | reproduce the reported defect | the report | `output/reproduction.md` | saw it fail for the reported reason |
| `02_fix` | change the cause, not the symptom | 01's output | `output/change.md` | the diff addresses the cause; no wire change |
| `03_verify` | checks, changelog, pull request | 02's output | `output/pull-request.md` | evidence matches the risk; approve and merge |

Factory (every run): `../_shared/` — each contract names the files it loads.
Product (each run): each stage's `output/`, which is not committed (`icm/.gitignore`).

Status is whatever exists: a stage is complete when its `output/` holds the artifact above.
