# Scripts Directory

This directory contains utility scripts for repository maintenance and releases.

## Release Management Scripts

### `create-missing-releases.sh` ⭐ NEW

**Purpose:** Create GitHub releases for versions that were missed (v1.7.8 and v1.7.9).

**Usage:**
```bash
./scripts/create-missing-releases.sh
```

**What it does:**
1. Extracts release notes from CHANGELOG.md for v1.7.8 and v1.7.9
2. Creates git tags if they don't exist
3. Pushes tags to GitHub
4. Creates GitHub releases with proper release notes

**Requirements:**
- GitHub CLI (`gh`) installed
- Authenticated with GitHub (`gh auth login`)
- Write access to the repository

**See also:** `docs/CREATE_MISSING_RELEASES.md` for detailed documentation

### `bump-version.sh`

Updates version numbers across all library files with consistency checks.

**Usage:**
```bash
./scripts/bump-version.sh patch        # Increment patch version (1.7.9 → 1.7.10)
./scripts/bump-version.sh minor        # Increment minor version (1.7.9 → 1.8.0)
./scripts/bump-version.sh major        # Increment major version (1.7.9 → 2.0.0)
./scripts/bump-version.sh patch 1.8.0  # Set specific version
```

**What it updates:**
- `library.properties` - Arduino library version
- `library.json` - PlatformIO library version
- `package.json` - NPM package version

### `release-agent.sh`

Comprehensive release validation and quality assurance tool.

**Usage:**
```bash
./scripts/release-agent.sh           # Full validation
./scripts/release-agent.sh --help    # Show help
./scripts/release-agent.sh --version # Show version
```

**What it checks:**
- ✅ Version consistency across all package files
- ✅ CHANGELOG completeness and format validation
- ✅ Build system configuration
- ✅ Dependency validation
- ✅ Git tag existence check
- ✅ Release workflow configuration
- ✅ Documentation link validation
- ✅ Test suite status (when available)

**When to use:**
- Before every release commit
- After making version changes
- When troubleshooting release issues
- As part of your local release workflow

### `validate-release.sh`

Quick pre-release validation script.

**Usage:**
```bash
./scripts/validate-release.sh
```

**What it checks:**
- Version consistency between files
- Changelog entries for current version
- Git working tree status
- Tag existence validation

## Complete Release Workflow

```bash
# 1. Update version
./scripts/bump-version.sh patch

# 2. Update CHANGELOG.md manually with release notes

# 3. Validate everything
./scripts/release-agent.sh

# 4. If all checks pass, commit and push
git add library.properties library.json package.json CHANGELOG.md
git commit -m "release: v1.8.0 - Brief description"
git push origin main

# 5. Verify release was created
gh release list --repo Alteriom/painlessMesh

# 6. If release is missing, use the create-missing-releases script
./scripts/create-missing-releases.sh
```

## Other Scripts

(Add documentation for other scripts as they are created)

## Getting Help

For issues with any script:
1. Check the script's `--help` option (if available)
2. Read related documentation in `docs/`
3. Open an issue with the `ci/cd` label

## Related Documentation

- **Release Guide**: `RELEASE_GUIDE.md` - Complete release process documentation
- **Create Missing Releases**: `docs/CREATE_MISSING_RELEASES.md` - How to create missed releases
- **Release Agent**: `.github/agents/release-agent.md` - Release agent detailed documentation
