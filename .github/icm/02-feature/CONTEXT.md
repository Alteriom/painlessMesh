# 02-feature — the pipeline

Agree the shape, build it, prove it, document it.

| Stage | Job | Input | Output | Human check |
|---|---|---|---|---|
| [`01_spec`](stages/01_spec/CONTEXT.md) | agree API, wire and version | the request | `output/spec.md` | A maintainer approves the public names, the wire impact, the type number, the version class and the example, or sends the spec back. Nothing is implemented against an unapproved spec. Edit `spec.md` in place; the next stages build what it says. |
| [`02_implement`](stages/02_implement/CONTEXT.md) | build what the spec says | 01_spec's `spec.md` | `output/implementation.md` | A person reads `implementation.md` beside `spec.md`. Every deviation is either approved, and `spec.md` amended to match, or sent back. Nothing on the wire beyond what the spec approved. |
| [`03_test`](stages/03_test/CONTEXT.md) | prove it at the named level | 01_spec's `spec.md` and 02_implement's `implementation.md` | `output/test-report.md` | A person reads the tests, not only their results: they call the library's code and would have caught the capability missing. The rig was asked for where the spec said it must be. |
| [`04_docs`](stages/04_docs/CONTEXT.md) | documents, changelog, pull request | 01_spec's `spec.md` and 03_test's `test-report.md` | `output/pull-request.md` | A maintainer reads the changed documents against the behaviour in `test-report.md`, then `pull-request.md`, and says whether to open it. The reviewer merges; a code change after approval needs approval again. |
