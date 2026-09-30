# 01_reproduce — reproduce the reported defect

One job: turn the report into something that fails on demand, for the reason reported.

## Inputs
- Working (this run): the report — read whole with what it links: an issue (`gh issue view <n> --comments`), a red run on `main` (`gh run view <run-id> --log-failed`), a serial log.
- Reference (every run): `.github/icm/_shared/architecture.md`
- Reference (every run): `.github/icm/_shared/build-and-test.md`
- Reference (every run): `.github/icm/_shared/gotchas.md`

Do NOT load: the other pipelines, `.github/icm/_shared/release.md`, other runs' output, released sections of CHANGELOG.md beyond the entries the report names.

## Process
1. If the report could be a vulnerability (`SECURITY.md` "What is in scope for a vulnerability report"), stop before writing anything: this pipeline ends in a public pull request, and `SECURITY.md` "Reporting a Vulnerability" is the route.
2. Restate the report as observed versus expected, with the version, chip (ESP32 / ESP8266), role (node, bridge, gateway) and build flags it came from.
3. Find the code path with [architecture.md](../../../_shared/architecture.md) and name the suspected cause as `path:line`. A suspicion is not a finding until something fails.
4. Pick the level from "Choosing the level of a test" in [build-and-test.md](../../../_shared/build-and-test.md). A gateway defect starts by teaching `test/mock-http-server/server.py` the service's behaviour.
5. Write the failing test (or, for the rig, the exact steps and the serial log that shows it). It must call the library's code, never a copy of it.
6. Run it and keep the failing output. If it passes, the reproduction is wrong or the report is: say which, and stop.

## Outputs
- reproduction.md → output/ — the restated report; suspected cause (`path:line`); the level chosen and why; the test file and the command; the failing output; what this reproduction does not cover (for example: the real radio).

## Human check
A person runs the command, or reads the log, and confirms it fails for the reported reason and not a neighbouring one.
