# 03_test — prove it at the named level

One job: tests that fail without the capability and pass with it, at the level the spec named.

## Inputs
- Working (this run): `.github/icm/02-feature/stages/01_spec/output/spec.md`
- Working (this run): `.github/icm/02-feature/stages/02_implement/output/implementation.md`
- Reference (every run): `.github/icm/_shared/build-and-test.md`

Do NOT load: the other pipelines, `.github/icm/_shared/release.md`, other runs' output.

## Process
1. Write the tests the spec's test plan names: `test/catch/catch_<topic>.cpp` scenarios (Given / When / Then), `test/boost/` for multi-node behaviour, the test point for gateway behaviour, a `test/ci/` PlatformIO project for a build flag.
2. A new package type gets a round-trip test (to `Variant` and back, every field, the type number).
3. Each test calls the library. Check it fails with the implementation reverted (`git stash` the `src/` change and run it), then passes.
4. Add the example the spec names, if it names one: folder name equals the `.ino` name, listed in `library.json` `examples`, simulator scenarios under `examples/<example>/test/simulator/` when it is multi-node.
5. Run the checks in [build-and-test.md](../../../_shared/build-and-test.md) that the change reaches: gcc, clang and ASan; the Arduino compile of the new or changed examples; `code-quality`. Name what needs the rig.

## Outputs
- test-report.md → output/ — each test and what it proves; the fails-without / passes-with evidence; the check results; what only the rig can show, and whether it was requested.

## Human check
A person reads the tests, not only their results: they call the library's code and would have caught the capability missing. The rig was asked for where the spec said it must be.
