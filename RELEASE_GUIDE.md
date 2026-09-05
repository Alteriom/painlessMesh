# AlteriomPainlessMesh Release Guide

This repository publishes one version to the Arduino Library Manager,
PlatformIO, npm, GitHub Packages, and GitHub Releases. A release starts only
after the release pull request is reviewed, all required CI checks pass, and
the version commit reaches `main`.

## Release checklist

1. Start from a clean branch based on the current release branch.
2. Confirm the intended changes are documented under `Unreleased` in
   [CHANGELOG.md](CHANGELOG.md).
3. Run the complete desktop, Arduino, PlatformIO, simulator, and hardware test
   suites required by the pull request.
4. Obtain approval and resolve every review conversation.
5. After approval, select the next semantic version and update the version
   metadata.
6. Move the `Unreleased` changes to a dated version section.
7. Run the release validation scripts.
8. Merge the release pull request. Do not create a tag by hand; the release
   workflow owns tags and publication.

## Version metadata

These files must always contain the same semantic version:

- `library.properties`
- `library.json`
- `package.json`

Use the repository script to change them together:

```bash
./scripts/bump-version.sh patch
```

You may use `minor`, `major`, or an explicit version when the release plan
requires it. Version 2.0 patch releases must remain wire-compatible with the
2.0 protocol.

## Changelog

During development, add user-visible changes to `## [Unreleased]`. At release
time, create a dated section:

```markdown
## [X.Y.Z] - YYYY-MM-DD

### Fixed

- Describe the observed defect, its impact, and the verified correction.
```

Keep operational evidence in the pull request and concise user-facing impact
in the changelog. Historical release entries remain immutable.

## Validation

Run both validation scripts from the repository root:

```bash
./scripts/validate-release.sh
./scripts/release-agent.sh
```

The pull request must also pass all required GitHub checks, including:

- gcc, clang, and AddressSanitizer desktop builds
- Arduino ESP32 and ESP8266 compilation
- PlatformIO builds and build-flag validation
- formatting and documentation checks
- CodeQL
- simulator scenarios
- ESP32, ESP32-C3, and ESP32-S3 hardware-in-the-loop coverage when the change
  touches radio, routing, gateway, OTA, or platform-specific behavior

For a hardware defect, attach the HIL run identifier and report link to the
pull request. A unit test alone is not sufficient evidence for a radio or
multi-device timing fix.

## Review gate

A release pull request is ready only when:

- it has an approving review;
- all review comments are resolved;
- required checks are green on the final commit;
- the branch is current with its target branch;
- version metadata and changelog validation pass;
- there are no unexplained skipped hardware scenarios.

Any code change after approval invalidates the approval and requires another
review of the final commit.

## Automated publication

When a qualifying version commit reaches `main`, `.github/workflows/release.yml`
validates the metadata, creates the `vX.Y.Z` tag and GitHub release, uploads the
Arduino archive, publishes npm and GitHub Packages, and dispatches the
PlatformIO publication workflow.

Monitor every publication job to completion. Verify the version is visible in
each registry before announcing the release. Arduino Library Manager indexing
can lag behind the GitHub release.

## Recovery

Never reuse a published semantic version. If publication succeeds in one
registry and fails in another, fix the credential or workflow problem and
rerun only the failed publication path for the same tag. If shipped code is
defective, prepare a new patch release rather than replacing the existing tag.

Security-sensitive release failures or suspected credential exposure must be
handled according to [SECURITY.md](SECURITY.md).
