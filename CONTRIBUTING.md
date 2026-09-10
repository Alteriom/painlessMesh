# Contributing

## Branches

- `main` holds released code. A push to `main` that carries a version bump,
  or whose head commit message starts with `release:`, is what tags and
  publishes a release (see [RELEASE_GUIDE.md](RELEASE_GUIDE.md)).
- `Feat/next-release` is the integration branch for the next version. Open
  pull requests against it.
- Work happens on short-lived feature branches (`fix/…`, `feat/…`, `docs/…`)
  cut from `Feat/next-release`.

Maintainers merge quickly, often as a squash. Push every commit you describe
before you describe it, and cut follow-up work from the merged base rather
than from a stale branch.

## Submit a pull request

- Point the pull request at `Feat/next-release`, not `main`.
- Say what was wrong, how you know (a log, a test, a measurement), and what
  the change does about it. For anything that touches the radio, routing,
  the gateway or OTA, the evidence is a serial log or a run on the
  hardware-in-the-loop rig; a unit test alone is not enough there, because
  the unit tests mock the radio.
- Get your code reviewed by another contributor, and let the reviewer merge.

Tests must pass for the code to be merged. Add a changelog entry under
`## [Unreleased]` in [CHANGELOG.md](CHANGELOG.md) for any user-visible change.

## Testing requirements

### Running the desktop tests

Before submitting a pull request, build and run the Catch2 and Boost suites:

```bash
git submodule update --init
cmake -G Ninja .
ninja
run-parts --regex catch_ bin/
```

The same suites run in CI under gcc, clang and AddressSanitizer, and CI also
compiles every example for esp32 and esp8266 with `arduino-cli`, builds the
PlatformIO projects under `test/ci/`, and checks formatting and library
metadata.

### Simulator and hardware tests

Multi-node behaviour is tested with the external
[painlessMesh-simulator](https://github.com/Alteriom/painlessMesh-simulator);
scenarios for an example live under `examples/<example>/test/simulator/`
(see `examples/basic/test/simulator/`). Radio, routing, gateway, failover
and OTA behaviour is validated on the Alteriom hardware-in-the-loop farm; a
maintainer runs it on a pull request by adding the `run-hil` label, and the
release gate is three consecutive clean runs of the whole suite.

### The HTTP test point

`test/mock-http-server/server.py` is the controlled Internet destination for
`sendToInternet()` tests at every level. It answers the way real services do,
including the ways they get it wrong (its CallMeBot emulation returns refusals
as HTTP 201 and 203, as the real API does), and it keeps a delivery ledger:
`GET /requests/{tag}` says whether the service actually accepted a request.
The desktop CI job starts one and exports `PAINLESSMESH_TESTPOINT`, and
`catch_issue450_testpoint_semantics` makes real HTTP requests to it and
requires the library's verdict to match the ledger. The farm's gateway probe
serves the same routes with the same record shape, so a hardware row and a
desktop scenario are measured against one source of truth. When a gateway bug
comes in, add the service behaviour that exposed it to the server first, then
the test that fails against it.

### Adding tests for new features

1. **Unit tests**: add to `test/catch/` for new components.
2. **Integration tests**: add to `test/boost/tcp_integration.cpp` for core
   behaviour that spans connections.
3. **Simulator scenarios**: add YAML scenarios under
   `examples/<example>/test/simulator/` for new examples.
4. **Documentation**: keep `USER_GUIDE.md`, `README.md` and the example's
   own README in step with the change.

## Versioning

This project follows [semver](https://semver.org/). Version 2.0 patch
releases must remain wire-compatible with the 2.0 protocol; anything that
changes what a node puts on the wire, or what an existing sketch has to do to
keep working, is a major version.

- **Major**: a backwards-incompatible change (a new or changed protocol
  message, a removed API).
- **Minor**: a backwards-compatible addition.
- **Patch**: a backwards-compatible fix.

Documentation does not require a version bump. The three version files
(`library.properties`, `library.json`, `package.json`) are changed together
with `./scripts/bump-version.sh`.
