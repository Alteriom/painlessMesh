# 02_fix — change the cause, not the symptom

One job: the smallest change at the defect's cause that turns the reproduction green without weakening it.

## Inputs
- Working (this run): `.github/icm/01-fix/stages/01_reproduce/output/reproduction.md`
- Reference (every run): `.github/icm/_shared/coding-rules.md`
- Reference (every run): `.github/icm/_shared/gotchas.md`
- Reference (only when a package's fields or type are involved): `.github/icm/_shared/package-types.md`

Do NOT load: the other pipelines, `.github/icm/_shared/release.md`, other runs' output.

## Process
1. Read the reproduction and its suspected cause. Confirm the cause in the code before changing anything; if it moved, record where it really is.
2. Change the cause. Do not edit the reproduction to make it pass; if it was wrong, go back to stage 01.
3. Keep the wire format and the public API as they are (a 2.x patch stays wire-compatible with 2.0, `CONTRIBUTING.md` "Versioning"). If the fix cannot, stop: the person starts `02-feature` with `reproduction.md` as the request (route `fix-to-feature` in the root `CLAUDE.md`).
4. Check the change against [coding-rules.md](../../../_shared/coding-rules.md): member initializers, gnu++11, warnings as errors under both compilers, memory on ESP8266, the `mppt_example/` copies if a package header changed.
5. Run the reproduction (green now) and the whole desktop suite: `cmake -G Ninja . && ninja && run-parts --regex catch_ bin/`.

## Outputs
- change.md → output/ — the cause in one paragraph; each file changed and why; the reproduction's passing output; the full-suite result; anything the fix deliberately does not address.

## Human check
A person reads the diff beside the reproduction: it removes the cause, not only the symptom the test sees, and no wire or API change slipped in. Edit `change.md` if the explanation is wrong; stage 03 reads it.
