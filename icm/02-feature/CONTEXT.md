# 02-feature — the pipeline

The flow in one line: agree the shape, build it, prove it, document it.

| Stage | Job | Input | Output | Human check |
|---|---|---|---|---|
| `01_spec` | agree API, wire and version | the request | `output/spec.md` | names, wire impact and semver class approved |
| `02_implement` | build what the spec says | 01's output | `output/implementation.md` | every deviation from the spec approved |
| `03_test` | prove it at the named level | 01's and 02's output | `output/test-report.md` | the tests call the library; results read |
| `04_docs` | documents, changelog, pull request | 01's and 03's output | `output/pull-request.md` | documents match the behaviour; approve and merge |

Factory (every run): `../_shared/` — each contract names the files it loads.
Product (each run): each stage's `output/`, which is not committed (`icm/.gitignore`).

Status is whatever exists: a stage is complete when its `output/` holds the artifact above.
