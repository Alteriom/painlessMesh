# 01_reproduce — reproduce the reported defect

One job: turn the report into something that fails on demand, for the reason reported.

## Inputs
- Working (this run): the report the person gave — an issue, a failing CI job, a serial log. Read it whole, including linked logs.
- Reference (every run): ../../../_shared/architecture.md
- Reference (every run): ../../../_shared/build-and-test.md
- Reference (every run): ../../../_shared/gotchas.md

Do NOT load: `02-feature/`, `03-release/`, other runs' output, `CHANGELOG.md` beyond the entries the report names.

## Process
1. Restate the report as observed versus expected, with the version, chip (ESP32 / ESP8266), role (node, bridge, gateway) and build flags it came from.
2. Find the code path with [architecture.md](../../../_shared/architecture.md) and name the suspected cause as `path:line`. A suspicion is not a finding until something fails.
3. Pick the level from "Choosing the level of a test" in [build-and-test.md](../../../_shared/build-and-test.md). A gateway defect starts by teaching `test/mock-http-server/server.py` the service's behaviour.
4. Write the failing test (or, for the rig, the exact steps and the serial log that shows it). It must call the library's code, never a copy of it.
5. Run it and keep the failing output. If it passes, the reproduction is wrong or the report is: say which, and stop.

## Outputs
- reproduction.md → output/ — the restated report; suspected cause (`path:line`); the level chosen and why; the test file and the command; the failing output; what this reproduction does not cover (for example: the real radio).

## Human check
Run the command, or read the log, and confirm it fails for the reported reason and not a neighbouring one. If the fix will need a new or changed package, message or public API, send the run to `02-feature` now.
