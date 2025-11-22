# Contributing

We try to follow the [git flow](https://www.atlassian.com/git/tutorials/comparing-workflows/gitflow-workflow) development model. Which means that we have a `develop` branch and `master` branch. All development is done under feature branches, which are (when finished) merged into the development branch. When a new version is released we merge the `develop` branch into the `master` branch.

## Git flow

If you would like to use [git flow tools](http://danielkummer.github.io/git-flow-cheatsheet/) you are more than welcome to. We use it and it's pretty nifty. If you see a `feature\` prefix on a comment then that is git flow automating branch creation. It does need more typing than just plain git so I suggest creating shell aliases for the commands.

## Submit a pull request:

* If your push triggered a 'you just pushed...' message from GitLab then click on the button provided by that pop up to create a pull request.
* If not, then create a pull request and point it to your branch.
* Make sure that you're attempting to merge into `develop` and not `master`.
* Get your code reviewed by another contributor. If there are no contributors who possess the same set of skills then get them to review it anyway but explain what the code does beforehand and why. Use it as an opportunity for discussion around the feature set, to transfer knowledge, and to possibly [rubber duck](https://en.wikipedia.org/wiki/Rubber_duck_debugging) your code.
* Once the code is reviewed then have your reviewer merge your code.

NOTE: Tests *must* pass in order for the code to be merged.

NOTE: Always do a `git pull` on `develop` before you start working to capture the latest changes.

## Testing Requirements

### Running Tests

Before submitting a pull request, ensure all tests pass:

```bash
# Build tests
cmake -G Ninja .
ninja

# Run all tests
run-parts --regex catch_ bin/
```

### Adding Tests for New Features

When adding new features or examples:

1. **Unit Tests**: Add tests in `test/catch/` for new components
2. **Example Tests**: Add simulator tests in `test/boost/` for new example sketches
3. **Documentation**: Update test documentation in `test/README.md`

See [Simulator Testing Guide](docs/development/SIMULATOR_TESTING.md) for detailed instructions on writing tests.

### Example Validation

All example sketches should have corresponding simulator tests that validate:
- Basic functionality works as documented
- Messages are delivered correctly
- Callbacks fire as expected
- Edge cases are handled properly

This helps prevent regressions and ensures examples remain working references for users.

## Versioning

This project will try its best to adhere to [semver](http://semver.org/) i.e, a codified guide to versioning software. When a new feature is developed or a bug is fixed the version will need to be bumped to signify the change. 

The semver string is built like this:

Major.Minor.Patch

A major version bump means that a massive change took place and that application will probably have to be redeployed because a *backwards incompatible* version was released. Example: A library => model relationship change which requires previous configuration options to become invalid.

A minor version is a *backwards compatible* addition or change to the core software. Most development activity will be this type of version bump. Example: A new feature or model.

A patch version is a *backwards compatible* bug fix or application configuration change.

Documentation doesn't require a version bump.
