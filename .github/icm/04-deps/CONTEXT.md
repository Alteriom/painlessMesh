# 04-deps — the pipeline

Say what the bump changes, then let a person decide.

| Stage | Job | Input | Output | Human check |
|---|---|---|---|---|
| [`01_review`](stages/01_review/CONTEXT.md) | what the bump changes | the bump | `output/review.md` | A person reads `review.md` and decides: merge, wait, or close the bump. A wire change found in step 2 or an untagged version found in step 5 is never merged from here. |
