# 05-docs — the pipeline

Find what the owning file says, then make the document say it or point at it.

| Stage | Job | Input | Output | Human check |
|---|---|---|---|---|
| [`01_locate`](stages/01_locate/CONTEXT.md) | find the owner of each claim | the report | `output/discrepancy.md` | A person opens each cited owner line and confirms it says what `discrepancy.md` claims, and agrees which copies become pointers. |
| [`02_edit`](stages/02_edit/CONTEXT.md) | correct the documents | 01_locate's `discrepancy.md` | `output/pull-request.md` | A reviewer reads the diff beside the owner lines in `discrepancy.md`, then says whether to open the pull request; the reviewer merges it. |
