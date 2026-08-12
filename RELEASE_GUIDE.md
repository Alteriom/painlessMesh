# painlessMesh Release Guide

This document provides comprehensive instructions for releasing new versions of the Alteriom painlessMesh library across all distribution channels.

## ⚠️ Important: Agent Requirements for Releases

**Releases MUST be performed by Alteriom AI Agent (`@alteriom-ai-agent`) with full tool access.**

### Why This Matters

A release requires updating 7 files consistently:
1. library.properties
2. library.json  
3. package.json
4. src/painlessMesh.h
5. src/AlteriomPainlessMesh.h
6. README.md
7. CHANGELOG.md

**✅ Correct Agent:** `@alteriom-ai-agent`
- Has file editing tools (`replace_string_in_file`, `multi_replace_string_in_file`)
- Can run terminal commands (`run_in_terminal`)
- Can execute git operations
- **Use this for release preparation**

**❌ Wrong Agent:** `@painlessmesh-coordinator` or specialized agents without tools
- Lack file editing capabilities
- Can only provide documentation/checklists
- Cannot actually perform releases
- Will result in manual work

### Agent-Assisted Release (Recommended)

```bash
# Ask Alteriom AI Agent to prepare release
@alteriom-ai-agent Prepare release v1.9.9 with these changes:
- Fixed ESP8266 WiFiClientSecure scope issue  
- Fixed TCP retry crash with AsyncClient cleanup

# Agent will:
# ✅ Update all 7 version files
# ✅ Restructure CHANGELOG.md
# ✅ Run validation: ./scripts/release-agent.sh
# ✅ Commit: "release: v1.9.9 - Description"
# ✅ Push to trigger automation
```

## 🚀 Quick Release Process

### Standard Release (Manual)

```bash
# 1. Update version using the bump script
./scripts/bump-version.sh patch  # or minor, major

# 2. Update CHANGELOG.md with your changes
# Add your changes under the new version section

# 3. Update header file version comments (recommended)
# Edit src/painlessMesh.h and src/AlteriomPainlessMesh.h
# Update @version comments to match the new library version

# 4. Run the Release Agent to validate readiness
./scripts/release-agent.sh

# 5. If all checks pass, commit and trigger release
git add library.properties library.json package.json CHANGELOG.md src/*.h README.md
git commit -m "release: v1.7.9 - Brief description"
git push origin main
```

**That's it!** GitHub Actions will automatically handle:
- ✅ Comprehensive testing across platforms
- ✅ Git tag creation and GitHub release
- ✅ NPM publishing (public + GitHub Packages)
- ✅ **PlatformIO Library Registry publishing**
- ✅ GitHub Wiki synchronization
- ✅ Arduino Library Manager package preparation
- ✅ Release notes generation from changelog

## 📋 Distribution Channels

### Automatic (Zero Manual Work Required)

1. **GitHub Releases** - Created with changelog and downloadable packages
2. **NPM Public Registry** - Published to <https://www.npmjs.com/package/@alteriom/painlessmesh>
3. **GitHub Packages** - Published to GitHub's NPM registry (@alteriom/painlessmesh)
4. **PlatformIO Registry** - Automatically published via GitHub Actions workflow
5. **GitHub Wiki** - Documentation synchronized from repository

### Semi-Automatic (One-Time Manual Submission)

1. **Arduino Library Manager** - Submit once, then automatically indexed

## 🎯 Detailed Process

### Version Management

> **📖 For detailed information about version management, see [VERSION_MANAGEMENT.md](docs/VERSION_MANAGEMENT.md)**

**File Consistency**: All three files must have matching versions:

```properties
# library.properties
version=1.6.1
```

```json
// library.json  
{
  "version": "1.6.1"
}
```

```json
// package.json
{
  "version": "1.6.1"
}
```

**Semantic Versioning**: Follow [semver.org](https://semver.org/):
- **MAJOR**: Breaking changes (e.g., 1.6.0 → 2.0.0)
- **MINOR**: New features, backward compatible (e.g., 1.6.0 → 1.7.0)  
- **PATCH**: Bug fixes, backward compatible (e.g., 1.6.0 → 1.6.1)

### Automation Triggers

The release workflow triggers on commits to `main` that:
1. Modify `library.properties`, `library.json`, `package.json`, or `CHANGELOG.md`
2. Have version files modified OR commit message starting with `release:`

### What Gets Automated

#### Testing Pipeline
- **Desktop builds**: gcc & clang with strict warnings
- **Arduino CLI**: ESP32 & ESP8266 compilation
- **PlatformIO**: Cross-platform build validation
- **Code quality**: Formatting and lint checks

#### Release Artifacts
- **Git tag**: `v1.6.1` format
- **GitHub Release**: With changelog excerpt
- **Library package**: `painlessMesh-v1.6.1.zip`
- **Documentation**: Auto-deployed to GitHub Pages

#### NPM Publishing
- **Public NPM**: Available to anyone via `npm install @alteriom/painlessmesh`
- **GitHub Packages**: Scoped package for authenticated users
- **Version consistency**: Verified across all package files

#### Wiki Updates
- **Home Page**: Generated from README.md
- **API Reference**: Auto-generated documentation
- **Examples**: Links to repository examples
- **Installation Guide**: Multi-platform instructions

## 📦 NPM Publishing Details

### Dual Publishing Strategy

Each release publishes to **two NPM registries**:

1. **Public NPM** (npmjs.com)
   - Package: `@alteriom/painlessmesh`
   - Installation: `npm install @alteriom/painlessmesh`
   - No authentication required

2. **GitHub Packages** (npm.pkg.github.com)
   - Package: `@alteriom/painlessmesh`
   - Installation: Requires `.npmrc` configuration
   - Authentication required for installation

### NPM Package Contents

The NPM package includes:
- `src/` - Complete library source code
- `examples/` - All Arduino examples
- `docs/` - Documentation files
- `library.properties` - Arduino metadata
- `library.json` - PlatformIO metadata
- Core documentation files (README, LICENSE, CHANGELOG)

Excluded from NPM package:

- Development files (`.github/`, `test/`, `scripts/`)
- Build artifacts (`bin/`, `build/`)
- IDE files and OS-specific files

## 🔧 PlatformIO Library Registry

### Automatic Publishing

Each release triggers the **PlatformIO Library Publishing** workflow:

1. **Validation**: Comprehensive library.json validation
2. **Dependencies**: Verification that all dependencies exist in PlatformIO Registry
3. **Authentication**: Uses `PLATFORMIO_AUTH_TOKEN` secret
4. **Publication**: Direct publishing via PlatformIO CLI
5. **Verification**: Post-publication registry verification

### Setup Requirements

#### One-Time Setup: PlatformIO Account & Token

1. **Create Account**: <https://platformio.org/account/register>
2. **Generate Token**: <https://platformio.org/account/token>
3. **Add Secret**: Repository Settings → Secrets → `PLATFORMIO_AUTH_TOKEN`

#### Automatic Workflow Trigger

The PlatformIO workflow is started by:

- The `platformio-dispatch` job in **Automated Release**, which calls
  `gh workflow run platformio-publish.yml --ref v<version> -f version=<version>`
  right after the release is created
- A GitHub release published by a human (via the UI or a PAT)
- Manual workflow dispatch for testing

> **Why the explicit dispatch?** `platformio-publish.yml` also listens for
> `release: published`, but that event never fires for releases created by
> `release.yml`: GitHub suppresses events raised by the built-in `GITHUB_TOKEN`.
> `workflow_dispatch` is one of the two documented exceptions to that rule, so
> the release workflow dispatches the publish explicitly and then verifies a run
> actually appeared. Before this was added, PlatformIO publication silently did
> not happen and had to be dispatched by hand (v1.9.21).

### PlatformIO Package Contents

Published package includes:

- Complete source code (`src/`)
- All examples (`examples/`)
- PlatformIO metadata (`library.json`)
- Arduino compatibility (`library.properties`)
- Documentation files

### Installation

Users can install via PlatformIO:

```ini
# platformio.ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps = alteriom/AlteriomPainlessMesh@^1.6.1
```

Or via CLI:

```bash
pio pkg install --library "alteriom/AlteriomPainlessMesh@^1.6.1"
```

### Manual Publication (Fallback)

If automatic publishing fails:

```bash
# Install PlatformIO CLI
pip install platformio

# Authenticate
pio account token --set YOUR_TOKEN

# Publish from repository root
pio pkg publish .
```

### Monitoring

- **Registry**: <https://registry.platformio.org/libraries>
- **Search**: <https://registry.platformio.org/search?q=AlteriomPainlessMesh>
- **Workflow**: GitHub Actions → PlatformIO Library Publishing

## 🛠️ Arduino Library Manager

### ✅ IMPORTANT: Library Name Fixed

**Current Status**: ✅ **REGISTERED AND FIXED**

The library IS registered in the Arduino Library Manager. The issue was a library name change that prevented new versions from being indexed.

**Issue Resolved**: Library name in `library.properties` has been restored to match the original registration.

**Result**: New releases will now be automatically indexed by Arduino Library Manager within 24-48 hours.

### One-Time Submission Process

**This must be done once** to enable Arduino IDE installation:

1. **Go to**: https://github.com/arduino/library-registry
2. **Click**: "Issues" → "New Issue"
3. **Create issue** with this template:

```markdown
Title: Add AlteriomPainlessMesh library

Repository URL: https://github.com/Alteriom/painlessMesh
Library Name: AlteriomPainlessMesh
Current Version: 1.8.2
Release Tag: v1.8.2

Description:
AlteriomPainlessMesh is a user-friendly library for creating mesh networks 
with ESP8266 and ESP32 devices. Enhanced fork of painlessMesh with:

- SensorPackage (Type 200): Environmental data collection
- StatusPackage (Type 202): Device health monitoring
- CommandPackage (Type 400): Remote device control
- MetricsPackage (Type 204): Performance metrics
- HealthCheckPackage (Type 605): Proactive monitoring
- Bridge Coordination: Multi-bridge high availability
- Message Queue: Offline message queueing

Category: Communication
Architectures: esp8266, esp32
Dependencies: ArduinoJson (^7.4.2), TaskScheduler (^4.0.0)
License: LGPL-3.0
Documentation: https://alteriom.github.io/painlessMesh/

All Arduino requirements met. Ready for indexing.
```

4. **Monitor** the issue for Arduino team approval (1-2 weeks typical)
5. **Verify** registration via Arduino IDE Library Manager search
6. **Future releases** automatically indexed (24-48 hour delay)

### Detailed Submission Guide

For complete instructions, see: [docs/ARDUINO_LIBRARY_MANAGER_SUBMISSION.md](docs/ARDUINO_LIBRARY_MANAGER_SUBMISSION.md)

The guide includes:
- Pre-submission checklist
- Detailed submission template
- Testing procedures
- Troubleshooting common issues
- Post-registration maintenance

### Arduino Library Compliance

The library meets all Arduino Library Manager requirements:
- ✅ Correct directory structure
- ✅ Valid `library.properties` file (version=1.8.2)
- ✅ Source files in `src/` directory
- ✅ Examples compile successfully (19+ examples)
- ✅ Consistent version numbering across files
- ✅ Open source license (LGPL-3.0)
- ✅ Git tags match library versions
- ✅ Comprehensive documentation

## 📚 GitHub Wiki Management

### Automatic Synchronization

Wiki pages are automatically updated on each release:

- **Home** - From README.md
- **Release-Guide** - From RELEASE_GUIDE.md  
- **Changelog** - From CHANGELOG.md
- **API-Reference** - Generated documentation
- **Examples** - Auto-generated from examples directory
- **Installation** - Multi-platform installation guide
- **Contributing** - From CONTRIBUTING.md

### Manual Wiki Updates

If you need to update the wiki manually:

```bash
# Clone wiki repository
git clone https://github.com/Alteriom/painlessMesh.wiki.git

# Edit markdown files directly
# Commit and push changes
```

Note: Manual changes may be overwritten by automatic synchronization.

## 🔧 Scripts Reference

### `./scripts/release-agent.sh` ⭐ NEW

**Comprehensive release validation and quality assurance.**

The Release Agent performs 21+ automated checks to ensure release readiness:

- ✅ Version consistency across all package files
- ✅ CHANGELOG completeness and format validation
- ✅ Build system configuration
- ✅ Dependency validation
- ✅ Git tag existence check
- ✅ Release workflow configuration
- ✅ Documentation link validation
- ✅ Test suite status (when available)

**Usage:**
```bash
./scripts/release-agent.sh           # Full validation
./scripts/release-agent.sh --help    # Show help
./scripts/release-agent.sh --version # Show version
```

**Benefits:**
- 🎯 Catches issues before they reach CI/CD
- 📊 Clear, color-coded output for easy scanning
- 🔧 Specific solutions for each type of issue
- 🚀 Comprehensive validation in under 5 seconds
- ✨ Professional release summary with next steps

**When to Use:**
- Before every release commit
- After making version changes
- When troubleshooting release issues
- As part of your local release workflow

**See Also:** `.github/agents/release-agent.md` for complete documentation

### `./scripts/bump-version.sh`
Updates version in all library files with consistency checks.

**Usage:**
```bash
./scripts/bump-version.sh patch        # Increment patch version
./scripts/bump-version.sh minor        # Increment minor version  
./scripts/bump-version.sh major        # Increment major version
./scripts/bump-version.sh patch 1.6.2  # Set specific version
```

### `./scripts/validate-release.sh`
Comprehensive pre-release validation.

**Checks:**
- Version consistency between all files
- Changelog entries for current version
- Git working tree status
- Tag existence validation
- Dependency declarations
- Build file presence
- Quick compilation test

## 🚨 Troubleshooting

### Common Issues

**Version Mismatch Error**
```bash
# Fix version inconsistencies
./scripts/bump-version.sh patch 1.6.2  # Force set version
```

**Missing Changelog Entry**
```bash
# Add changelog entry for current version
vim CHANGELOG.md
# Add section: ## [1.6.2] - YYYY-MM-DD
```

**Build Failures**
```bash
# Test locally before release
npm run build
npm run test
```

**NPM Token Expired / Invalid (`E401 Unauthorized`)**

Symptom: the `npm-publish` job fails at *Verify NPM authentication* with
`401 Unauthorized - GET https://registry.npmjs.org/-/whoami`. npm automation
tokens expire; everything else in the release (tag, GitHub Release, zip asset,
GitHub Packages, PlatformIO) succeeds independently, so **the release can look
green-ish while npmjs.org is missing the version**. Always confirm with
`npm view @alteriom/painlessmesh version`.

Rotating the token is operator-only — it cannot be automated from CI:

```bash
# 1. Mint a fresh *Automation* token (Granular, scoped to @alteriom/painlessmesh)
#    https://www.npmjs.com/settings/tokens
# 2. Update the NPM_TOKEN repository secret
#    https://github.com/Alteriom/painlessMesh/settings/secrets/actions
# 3. Verify locally before saving (optional but recommended)
NPM_TOKEN=<new-token> npm whoami --registry=https://registry.npmjs.org

# 4a. Re-run the failed release job (keeps the original run's context)
gh run rerun <run-id> --failed --repo Alteriom/painlessMesh

# 4b. …or republish the current version out-of-band
gh workflow run manual-publish.yml --repo Alteriom/painlessMesh \
  -f publish_npm=true -f publish_github=false

# 5. Confirm the version actually landed
npm view @alteriom/painlessmesh version
```

While rotating `NPM_TOKEN`, check `PLATFORMIO_AUTH_TOKEN` too — it expires the
same way and `platformio-publish.yml` hard-fails on an invalid one.

**GitHub Packages Authentication**
```bash
# Check if GITHUB_TOKEN has packages:write permission
# Repository Settings → Actions → General → Permissions
```

**Wiki Update Failure**
```bash
# Wiki may need manual initialization
# Go to: https://github.com/Alteriom/painlessMesh/wiki
# Create any page to initialize, then re-run release
```

**NPM/GitHub Packages Not Published Automatically**

The automated workflow triggers a release in two ways:

1. **Automatic (Recommended)**: When version files are updated in a commit
   - The workflow detects changes to `library.properties`, `library.json`, or `package.json`
   - Automatically creates tag, release, and publishes packages when these files are modified
   - Works seamlessly with PR merges and direct commits

2. **Manual trigger**: Commit message starts with `release:` (lowercase with colon):

```bash
# ✅ Correct - Will trigger full release
git commit -m "release: v1.7.7 - Complete mqtt-schema implementation"

# ✅ Also works - Version file changes detected automatically
# (No special commit message needed when library.properties/json/package.json are modified)
git commit -m "Bump version to 1.7.8"
```

**Note**: If version files weren't modified and commit message doesn't start with "release:", the workflow will skip publishing.

**Solution: Use Manual Publishing Workflow**

If this happens, you can manually publish packages:

1. Go to **Actions** → **Manual Package Publishing**
2. Click **Run workflow**
3. Select options:
   - ✅ Publish to NPM Registry
   - ✅ Publish to GitHub Packages
4. Click **Run workflow**

The manual workflow will:
- Read the current version from `library.properties` and refuse to run if it
  disagrees with `package.json` (npm publishes the `package.json` version)
- Validate `NPM_TOKEN` against the registry before attempting to publish, so an
  expired token fails immediately with rotation instructions
- Publish to NPM (if selected)
- Publish to GitHub Packages (if selected)
- Show success/failure status for each

It does **not** publish to the PlatformIO registry — use
`gh workflow run platformio-publish.yml --ref v<version> -f version=<version>`
for that.

Alternatively, from command line:
```bash
# Trigger via GitHub CLI
gh workflow run manual-publish.yml
```

### Manual Override

If automation fails, you can manually perform any step:

```bash
# Manual NPM publish
npm publish --access public

# Manual GitHub release  
gh release create v1.6.2 --title "painlessMesh v1.6.2" --notes-file CHANGELOG.md

# Manual tag creation
git tag v1.6.2
git push origin v1.6.2
```

## 🔍 Validation Commands

### Pre-Release Checks
```bash
# Comprehensive validation
./scripts/validate-release.sh

# Version consistency check
./scripts/bump-version.sh --verify || echo "Use proper arguments"

# NPM package validation
npm run validate-library

# Build test
npm run build && npm run test
```

### Post-Release Verification
```bash
# Check NPM publication
npm view @alteriom/painlessmesh

# Check GitHub Packages
npm view @alteriom/painlessmesh --registry=https://npm.pkg.github.com

# Check PlatformIO Registry
pio pkg search "AlteriomPainlessMesh"

# Verify GitHub release
gh release view

# Check wiki update
curl -s https://github.com/Alteriom/painlessMesh/wiki | grep -q "v1.6.2"
```

## 🎉 Success Indicators

### Successful Release Shows:
- ✅ GitHub release created with changelog
- ✅ Git tag pushed to repository  
- ✅ NPM package published (check npmjs.com)
- ✅ GitHub Packages updated
- ✅ Wiki pages synchronized
- ✅ All GitHub Actions workflows completed successfully

### Distribution Verification:
```bash
# Public NPM
npm view @alteriom/painlessmesh

# GitHub Packages
npm view @alteriom/painlessmesh --registry=https://npm.pkg.github.com

# PlatformIO (updated within minutes via GitHub Actions)
# Check: https://registry.platformio.org/libraries/alteriom/AlteriomPainlessMesh

# Arduino Library Manager (after manual submission)
# Search in Arduino IDE Library Manager
```

## 🌟 Advanced Topics

### Custom Release Notes

To customize release notes beyond the changelog:

1. Edit the generated `release_notes.txt` in the workflow
2. Or create a custom release notes file in `.github/release-template.md`

### Environment-Specific Releases

For testing releases:

```bash
# Use pre-release tags
git tag v1.6.2-beta
git push origin v1.6.2-beta

# This creates a pre-release without full publication
```

### Rollback Procedure

If a release has issues:

```bash
# Delete remote tag
git push origin :refs/tags/v1.6.2

# Delete local tag  
git tag -d v1.6.2

# Delete GitHub release
gh release delete v1.6.2

# Unpublish NPM package (contact npm support)
# GitHub Packages: Delete from package settings
```

## 📊 Release Metrics

Monitor your releases:
- **GitHub**: https://github.com/Alteriom/painlessMesh/releases
- **NPM**: https://www.npmjs.com/package/@alteriom/painlessmesh
- **PlatformIO**: https://registry.platformio.org/libraries/alteriom/painlessMesh
- **Wiki**: https://github.com/Alteriom/painlessMesh/wiki

## 🤝 Security & Permissions

### Required GitHub Secrets

- `GITHUB_TOKEN`: Automatically provided by GitHub Actions
- `NPM_TOKEN`: Required for NPM publishing (add in repository secrets)
- `PLATFORMIO_AUTH_TOKEN`: Required for PlatformIO Library Registry publishing

Both `NPM_TOKEN` and `PLATFORMIO_AUTH_TOKEN` are user-minted tokens that
**expire**. Their expiry is invisible until a release fails, so rotate them
together and re-check after any expiry date you set. See
[NPM Token Expired / Invalid](#-troubleshooting) for the rotation runbook.

### Repository Settings
- **Actions**: Enabled with write permissions
- **Packages**: Enabled for GitHub Packages publication
- **Wiki**: Enabled for documentation deployment
- **Releases**: Public releases enabled

---

## Quick Reference

**Release a patch version:**
```bash
./scripts/bump-version.sh patch
# Edit CHANGELOG.md
git add . && git commit -m "release: v1.6.2" && git push
```

**Check release status:**
```bash
./scripts/validate-release.sh
```

**Monitor release:**
- GitHub Actions: https://github.com/Alteriom/painlessMesh/actions
- Releases: https://github.com/Alteriom/painlessMesh/releases
- Documentation: https://github.com/Alteriom/painlessMesh/wiki

For questions or issues with the release process, create an issue with the `ci/cd` label.