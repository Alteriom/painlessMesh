# Contributing to painlessMesh

Thank you for your interest in contributing to the Alteriom painlessMesh library!

## Quick Links

- **[Main Contributing Guide](../../CONTRIBUTING.md)** - Complete contribution guidelines
- **[Documentation Guide](documentation.md)** - How to contribute to documentation
- **[Code Style Guide](ARDUINO_COMPLIANCE_SUMMARY.md)** - Arduino library standards
- **[Testing Guide](TESTING_SUMMARY.md)** - Testing infrastructure and practices
- **[Docker Testing](DOCKER_TESTING.md)** - Containerized testing environment

## Overview

There are many ways to contribute to painlessMesh:

### 🐛 Report Bugs

Found a bug? [Open an issue](https://github.com/Alteriom/painlessMesh/issues/new) with:

- Hardware (ESP32/ESP8266 model)
- Library version
- Minimal code to reproduce
- Expected vs actual behavior
- Debug output

### 💡 Suggest Features

Have an idea? [Start a discussion](https://github.com/Alteriom/painlessMesh/discussions/new) or open an issue with:

- Clear use case
- Proposed API/interface
- Implementation considerations
- Compatibility impact

### 📝 Improve Documentation

Documentation improvements are always welcome:

- Fix typos or unclear explanations
- Add examples and tutorials
- Improve API documentation
- Translate documentation

See the [Documentation Guide](documentation.md) for details.

### 🔧 Submit Code

Contributing code:

1. **Fork** the repository
2. **Create a branch**: `git checkout -b feature/my-feature`
3. **Make changes**: Follow our code style
4. **Test**: Run the test suite
5. **Commit**: Use descriptive commit messages
6. **Push**: `git push origin feature/my-feature`
7. **Pull Request**: Describe your changes

### ✅ Code Quality Standards

All code contributions should:

- Follow Arduino library guidelines
- Include tests where applicable
- Compile without warnings on ESP32 and ESP8266
- Include comments for complex logic
- Update documentation as needed

### 🧪 Testing

Before submitting:

```bash
# Build tests
cmake -G Ninja .
ninja

# Run tests
run-parts --regex catch_ bin/

# Test Arduino examples
# Use Arduino IDE or PlatformIO to verify examples compile
```

See [Testing Guide](TESTING_SUMMARY.md) for comprehensive testing instructions.

## Development Workflow

### Setting Up Development Environment

**Required Tools:**

- Git
- CMake 3.10+
- Ninja build system
- GCC/Clang compiler
- Arduino IDE or PlatformIO

**Optional Tools:**

- Docker (for containerized testing)
- Python 3.6+ (for scripts)
- Node.js (for npm package development)

**Clone and Build:**

```bash
git clone https://github.com/Alteriom/painlessMesh.git
cd painlessMesh

# Initialize submodules
git submodule update --init

# Build
cmake -G Ninja .
ninja

# Run tests
run-parts --regex catch_ bin/
```

### Branch Naming

Use descriptive branch names:

- `feature/broadcast-ota` - New features
- `fix/memory-leak` - Bug fixes
- `docs/mqtt-guide` - Documentation
- `refactor/clean-api` - Code improvements
- `test/coverage-boost` - Test additions

### Commit Messages

Follow conventional commits:

```
type(scope): brief description

Longer explanation if needed.

Fixes #123
```

**Types:**

- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation
- `style`: Code style (formatting)
- `refactor`: Code refactoring
- `test`: Tests
- `chore`: Build/tooling

**Examples:**

```
feat(ota): add broadcast OTA distribution

Implements chunk-based OTA distribution across mesh network
with 98% traffic reduction for large meshes.

Fixes #42
```

```
fix(mqtt): correct topic sanitization

Node IDs with special characters were causing MQTT
subscription failures. Now properly escapes characters.

Fixes #56
```

## Platform-Specific Considerations

### ESP32

- Memory: ~320KB RAM available
- Use PSRAM where available
- Test with WiFi library v2.0.0+
- Consider dual-core implications

### ESP8266

- Memory: ~80KB RAM (limited!)
- Be extra careful with memory allocation
- Test with Arduino Core 3.0.0+
- Avoid large stack allocations

## Code Style

### C++ Style

- Use 2-space indentation
- Opening braces on same line
- Use `camelCase` for functions
- Use `PascalCase` for classes
- Use `UPPER_CASE` for constants

### Arduino Conventions

- Provide `.ino` examples
- Use `setup()` and `loop()` pattern
- Include helpful Serial output
- Add comments explaining hardware setup

### Header Guards

```cpp
#ifndef PAINLESSMESH_FEATURE_HPP
#define PAINLESSMESH_FEATURE_HPP

// Code here

#endif // PAINLESSMESH_FEATURE_HPP
```

## Pull Request Process

### Before Submitting

- [ ] Code compiles on ESP32 and ESP8266
- [ ] Tests pass
- [ ] Documentation updated
- [ ] Examples updated (if API changed)
- [ ] CHANGELOG.md updated
- [ ] No merge conflicts with main

### PR Template

```markdown
## Description
What does this PR do?

## Motivation
Why is this change needed?

## Changes
- Changed X to Y
- Added Z feature
- Fixed W bug

## Testing
How was this tested?

## Checklist
- [ ] Code compiles without warnings
- [ ] Tests pass
- [ ] Documentation updated
- [ ] Examples work
- [ ] CHANGELOG updated
```

### Review Process

1. **Automated checks** run (build, tests, linting)
2. **Maintainer review** (usually within 1 week)
3. **Feedback** addressed
4. **Approval** and merge

## Community Guidelines

### Be Respectful

- Be kind and courteous
- Accept constructive criticism
- Focus on the issue, not the person
- Help others learn

### Communication Channels

- **GitHub Issues** - Bug reports and feature requests
- **GitHub Discussions** - Questions and ideas
- **Pull Requests** - Code contributions

## Getting Help

Stuck? Need help contributing?

- Check the [FAQ](../troubleshooting/faq.md)
- Ask in [Discussions](https://github.com/Alteriom/painlessMesh/discussions)
- Review [existing PRs](https://github.com/Alteriom/painlessMesh/pulls)
- Read the [complete contributing guide](../../CONTRIBUTING.md)

## License

By contributing, you agree that your contributions will be licensed under the LGPL-3.0 License.

See [LICENSE](../../LICENSE) for details.

## Thank You! 🎉

Your contributions make painlessMesh better for everyone. We appreciate your time and effort!

## See Also

- [Main Contributing Guide](../../CONTRIBUTING.md) - Complete guidelines
- [Documentation Guide](documentation.md) - Documentation standards
- [Testing Guide](TESTING_SUMMARY.md) - Testing practices
- [Release Guide](../../RELEASE_GUIDE.md) - Release process
- [Code of Conduct](../../CODE_OF_CONDUCT.md) - Community standards (if exists)
