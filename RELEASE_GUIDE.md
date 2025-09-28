# painlessMesh Release Guide

This guide describes the automated release process for painlessMesh using the state-of-the-art CI/CD pipeline.

## Overview

The release process is fully automated via GitHub Actions and includes:

- ✅ Automated version validation
- ✅ Comprehensive testing (Desktop, Arduino, PlatformIO)
- ✅ Documentation generation and deployment
- ✅ GitHub Releases with changelog
- ✅ Library package distribution
- ✅ Arduino Library Manager compatibility
- ✅ PlatformIO Library Registry integration

## Quick Release Process

### 1. Prepare the Release

```bash
# Update version (interactive)
./scripts/bump-version.sh patch  # or minor, major

# Or set specific version
./scripts/bump-version.sh patch 1.5.7

# Validate release readiness
./scripts/validate-release.sh
```

### 2. Update Changelog

Edit `CHANGELOG.md` and add your changes under the new version:

```markdown
## [1.5.7] - 2024-09-28

### Added
- New feature descriptions

### Changed
- What changed from previous version

### Fixed
- Bug fixes in this release
```

### 3. Commit and Release

```bash
# Commit version bump and changelog
git add library.properties library.json CHANGELOG.md
git commit -m "release: v1.5.7"

# Push to trigger automated release
git push origin main
```

That's it! GitHub Actions will automatically:
1. Validate the release
2. Run all tests
3. Create a git tag
4. Generate GitHub release with changelog
5. Package the library
6. Deploy documentation

## Detailed Process

### Version Management

**File Consistency**: Both `library.properties` and `library.json` must have matching versions:

```properties
# library.properties
version=1.5.7
```

```json
// library.json  
{
  "version": "1.5.7"
}
```

**Semantic Versioning**: Follow [semver.org](https://semver.org/):
- **MAJOR**: Breaking changes (e.g., 1.5.6 → 2.0.0)
- **MINOR**: New features, backward compatible (e.g., 1.5.6 → 1.6.0)  
- **PATCH**: Bug fixes, backward compatible (e.g., 1.5.6 → 1.5.7)

### Automation Triggers

The release workflow triggers on commits to `main` that:
1. Modify `library.properties`, `library.json`, or `CHANGELOG.md`
2. Have a commit message starting with `release:`

### What Gets Automated

#### Testing Pipeline
- **Desktop builds**: gcc & clang with strict warnings
- **Arduino CLI**: ESP32 & ESP8266 compilation
- **PlatformIO**: Cross-platform build validation
- **Code quality**: Formatting and lint checks

#### Release Artifacts
- **Git tag**: `v1.5.7` format
- **GitHub Release**: With changelog excerpt
- **Library package**: `painlessMesh-v1.5.7.zip`
- **Documentation**: Auto-deployed to GitHub Pages

#### Library Distribution  
- **Arduino Library Manager**: Automatic via GitHub releases
- **PlatformIO Registry**: Automatic via `library.json` + tags

## Troubleshooting

### Common Issues

**Version Mismatch**
```bash
# Fix version inconsistencies
./scripts/bump-version.sh patch 1.5.7  # Force set version
```

**Missing Changelog Entry**
```bash
# Add changelog entry for current version
vim CHANGELOG.md
```

**Build Failures**
```bash
# Test locally before release
cmake -G Ninja .
ninja
run-parts --regex catch_ bin/
```

**Tag Already Exists**
```bash
# Delete existing tag if needed (use carefully!)
git tag -d v1.5.7
git push origin :refs/tags/v1.5.7
```

### Manual Override

If automation fails, you can manually:

```bash
# Create tag manually
git tag -a v1.5.7 -m "painlessMesh v1.5.7"
git push origin v1.5.7

# Create GitHub release manually via web UI or gh CLI
gh release create v1.5.7 --title "painlessMesh v1.5.7" --notes-file CHANGELOG.md
```

## Development Workflow

### Feature Development
```bash
# Work on feature branches
git checkout -b feature/my-new-feature
# ... make changes ...
git commit -m "feat: add new feature"
git push origin feature/my-new-feature
# Create PR to main
```

### Release Candidates
```bash
# For pre-releases
./scripts/bump-version.sh minor 1.6.0-rc.1
git commit -m "release: v1.6.0-rc.1"
git push origin main
# GitHub Actions will create pre-release
```

## Integration Points

### Arduino Library Manager
- Monitors GitHub releases
- Uses `library.properties` for metadata
- Automatic inclusion upon tagged release

### PlatformIO Library Registry  
- Monitors GitHub repositories
- Uses `library.json` for metadata
- Updates automatically on new tags
- Registry: https://registry.platformio.org/libraries/alteriom/painlessMesh

### Documentation Sites
- **GitHub Pages**: https://alteriom.github.io/painlessMesh/
- **Doxygen API**: Auto-generated from source comments
- **User Guides**: Markdown docs in `/docs` directory

## Scripts Reference

### `./scripts/bump-version.sh`
Updates version in both library files with consistency checks.

**Usage:**
```bash
./scripts/bump-version.sh patch        # Increment patch version
./scripts/bump-version.sh minor        # Increment minor version  
./scripts/bump-version.sh major        # Increment major version
./scripts/bump-version.sh patch 1.5.7  # Set specific version
```

### `./scripts/validate-release.sh`
Comprehensive pre-release validation.

**Checks:**
- Version consistency between files
- Changelog entries
- Git working tree status
- Tag existence
- Dependency declarations
- Build file presence
- Quick build test

## Security & Permissions

### GitHub Secrets Required
- `GITHUB_TOKEN`: Automatically provided
- Additional secrets may be needed for external registries

### Repository Settings
- **Actions**: Enabled with write permissions
- **Pages**: Enabled for documentation deployment  
- **Releases**: Public releases enabled

---

## Quick Reference

**Release a patch version:**
```bash
./scripts/bump-version.sh patch
# Edit CHANGELOG.md
git add . && git commit -m "release: v1.5.7" && git push
```

**Check release status:**
```bash
./scripts/validate-release.sh
```

**Monitor release:**
- GitHub Actions: https://github.com/Alteriom/painlessMesh/actions
- Releases: https://github.com/Alteriom/painlessMesh/releases
- Documentation: https://alteriom.github.io/painlessMesh/

For questions or issues with the release process, create an issue with the `ci/cd` label.