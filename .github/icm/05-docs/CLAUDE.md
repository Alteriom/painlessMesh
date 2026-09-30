# 05-docs — correct a document

One run is one set of documents that disagree with the code, a workflow or a script — or that restate a fact another file owns. The owning file is right; the document is made to say the same, or to point at the owner instead. No code changes here: a document that is right about behaviour the code gets wrong is a defect for `01-fix`.

Stages, in order — the table with inputs, outputs and checks is [CONTEXT.md](CONTEXT.md):

1. [Locate](stages/01_locate/CONTEXT.md) — each wrong claim beside the file that owns the fact.
2. [Edit](stages/02_edit/CONTEXT.md) — correct or replace with a pointer, and the pull request.

Stop after each stage until a person has read its output.
