# Contributing

## Branches

| Branch | What it is | Lifetime |
|---|---|---|
| `main` | the released line. A push carrying a version-file change, or whose head commit starts with `release:`, tags and publishes (see [RELEASE_GUIDE.md](RELEASE_GUIDE.md)) | permanent |
| `release/<major>.x` | one major line — `release/3.x` while 3.0 is being built, and the same branch afterwards for patches to 2.x once `main` has moved on | as long as that major is being built or supported |
| `release/<version>` | preparing one release: the version files, the changelog date | short-lived |
| `fix/…` `feat/…` `docs/…` `ci/…` `test/…` `refactor/…` `chore/…` | one change | short-lived |

**Open pull requests against `main`** unless the change belongs to a major
line that has its own branch, in which case open them against that. This is
also what the repository does in practice: every fix since #448 was merged to
`main`.

`Feat/next-release` is retired. It was where v2 was built while `main` still
carried v1, which is exactly the job `release/<major>.x` now names; it holds
nothing `main` does not, and leaving it there sent contributors at a branch
27 commits behind.

### Naming

- **`type/short-slug`**, lowercase, hyphens — never underscores or capitals.
  The type is one of the [Conventional Commits](https://www.conventionalcommits.org/)
  types this repository already uses in commit subjects, so a branch and the
  commits on it agree about what they are.
- **An issue number goes at the end** when there is one: `fix/rejoin-459`.
  A slug that says only the area (`fix/critical-bugs`) tells a later reader
  nothing about what was wrong.
- **No `v` in a branch name.** Tags carry it (`v2.0.3`); branches do not
  (`release/2.0.4`, `release/3.x`). 2.0.2 shipped from `release/2.0.2` and
  2.0.3 from `release/v2.0.3` — pick the one without.
- **Tool-generated prefixes** (`copilot/…`, `claude/…`, `dependabot/…`) are
  left as the tool makes them. They are ordinary short-lived work branches
  and are deleted on merge like any other.

### Status

A branch is deleted when its pull request merges — by whoever merges it, or
by GitHub's automatic branch deletion. Anything still on the remote is
either live work or something that was forgotten, and the two look identical
from a branch list, so:

```bash
./scripts/branch-status.sh          # every remote branch: ahead, behind, age, unmerged work
./scripts/branch-status.sh --stale  # only the ones with nothing of their own
```

`ahead` counts commits `main` does not have; `unmerged` counts those whose
change is not in `main` under any commit, which is the number that matters
after a squash merge. A branch with `unmerged=0` can be deleted without
losing anything.

Maintainers merge quickly, often as a squash. Push every commit you describe
before you describe it, and cut follow-up work from the merged base rather
than from a stale branch.

## Submit a pull request

- Point the pull request at `main`, or at the major line's own branch when
  the change belongs to one (see [Branches](#branches)).
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
and OTA behaviour is validated on the Alteriom hardware-in-the-loop farm.
`.github/workflows/farm-hil.yml` sends the farm a run for **every merge to
`main` whose CI passed**, and for a pull request when a maintainer adds the
`run-hil` label, then **waits for the rig's verdict and carries it**. The
Automated Release workflow runs only once that job has succeeded on
`main`, so a version bump is tagged after the boards have passed it, never
merely after they were asked; a merge that bumps nothing still runs on the
rig and releases nothing. Both need the `FARM_DISPATCH_TOKEN` secret (a
fine-grained token for the farm repository with Contents read/write and
Actions read); without it the job says so and passes, and, having sent
nothing, releases nothing. A rig run is never a release build from here --
one that may spend a real provider message is started by a person from the
farm's own workflow -- and the release gate is three consecutive clean
runs of the whole suite.

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
