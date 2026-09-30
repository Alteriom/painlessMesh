# 01-fix — fix a defect

One run is one defect: reproduced, fixed at its cause, and proposed as a pull request with the evidence. It is the job this repository does most: `fix(…)` is the most common commit subject on `main`.

Stages, in order; inputs, outputs and checks for each are in [CONTEXT.md](CONTEXT.md):

1. [Reproduce](stages/01_reproduce/CONTEXT.md) — make it fail on purpose.
2. [Fix](stages/02_fix/CONTEXT.md) — change the cause, watch the reproduction pass.
3. [Verify](stages/03_verify/CONTEXT.md) — the full checks, the changelog, the pull request.
