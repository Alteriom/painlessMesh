---
applies_to:
  - ".github/workflows/**/*.yml"
---

# GitHub Workflows Instructions

When working with CI/CD workflows in this repository:

## Workflow Overview

### CI Workflow (`ci.yml`)
Runs on every push and pull request:
- Builds the project
- Runs all tests
- Validates code quality

### Documentation Workflow (`docs.yml`)
Updates documentation site:
- Generates API documentation with Doxygen
- Deploys to GitHub Pages

### Release Workflows
- `release.yml` - Creates new releases
- `platformio-publish.yml` - Publishes to PlatformIO registry
- `manual-publish.yml` - Manual publishing when needed

## Modifying Workflows

### Adding Build Steps
When adding new build requirements:

```yaml
- name: Install dependencies
  run: |
    sudo apt-get update
    sudo apt-get install -y cmake ninja-build libboost-dev libboost-system-dev
    # Add new dependencies here
```

### Adding Test Steps
When adding new test suites:

```yaml
- name: Run tests
  run: |
    cmake -G Ninja .
    ninja
    ./bin/catch_alteriom_packages
    # Add new test executables here
```

### Modifying Documentation Generation
When updating docs:

```yaml
- name: Generate docs
  run: |
    doxygen Doxyfile
    # Additional doc generation steps
```

## Best Practices

### Workflow Security
- Never commit secrets directly
- Use GitHub Secrets for sensitive data
- Validate inputs from external sources
- Use pinned action versions when possible

### Efficiency
- Cache dependencies when possible
- Run tests in parallel when they're independent
- Skip unnecessary steps with conditions

### Debugging Workflows
- Add debug output: `echo "Debug: $VARIABLE"`
- Check workflow logs in GitHub Actions tab
- Test locally with `act` when possible

## Common Patterns

### Conditional Steps
```yaml
- name: Only on main branch
  if: github.ref == 'refs/heads/main'
  run: echo "Main branch only"
```

### Matrix Builds
```yaml
strategy:
  matrix:
    platform: [esp32, esp8266]
```

### Artifact Upload
```yaml
- uses: actions/upload-artifact@v3
  with:
    name: test-results
    path: bin/
```

## Important Notes

1. **Test dependencies**: Workflows must clone ArduinoJson and TaskScheduler
2. **Boost requirement**: Install libboost-dev for tests
3. **CMake + Ninja**: Standard build system for this repo
4. **Branch protection**: Main branch requires passing CI

## Workflow Triggers

- `push` - Any push to repository
- `pull_request` - Any PR opened or updated
- `workflow_dispatch` - Manual trigger
- `release` - When a release is created

## Troubleshooting

If workflows fail:
1. Check the logs in GitHub Actions tab
2. Verify all dependencies are installed
3. Ensure test dependencies are cloned correctly
4. Check for version compatibility issues
5. Test the commands locally first
