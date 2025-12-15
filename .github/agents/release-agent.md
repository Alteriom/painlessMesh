# Release Agent for AlteriomPainlessMesh

## Purpose

This agent ensures consistency and quality in all releases of the AlteriomPainlessMesh library. It automates validation, verification, and documentation tasks to maintain high standards across all release channels.

## Important: Agent Tool Requirements

**This release agent REQUIRES full tool access to perform its duties.**

### Required Tools for Release Management

The agent performing release tasks MUST have access to:

1. **File Editing Tools** (CRITICAL for implementation)
   - `replace_string_in_file` - Update version numbers, dates, and content in single files
   - `multi_replace_string_in_file` - Batch update multiple files efficiently (preferred)
   - `create_file` - Generate release documentation if needed
   - `read_file` - Verify file contents before and after changes
   - `file_search` - Locate files by glob pattern
   - `grep_search` - Search file contents by regex or text
   - `semantic_search` - Search codebase by semantic meaning

2. **Git and GitHub Tools** (CRITICAL for release workflow)
   - `get_changed_files` - Review pending changes and diffs
   - `run_in_terminal` - Execute git commands (add, commit, push, tag, status)
   - `mcp_github_*` tools - Create releases, manage issues/PRs, create branches
   - GitHub API read access - Fetch PR details, release info

3. **Build and Validation Tools** (CRITICAL for quality assurance)
   - `run_in_terminal` - Execute validation scripts (`./scripts/release-agent.sh`), run tests, cmake, ninja
   - `get_errors` - Check for build or lint errors in files or workspace
   - `get_terminal_output` - Review command output from background processes

### Agent Configuration

**Both release-agent and release-validator MUST have `tools` configured with full access:**

```yaml
tools: ["read", "search", "run_terminal", "edit", "multi_edit", "create", "git"]
```

Without these tools, the agent can only document what needs to be done, not implement it.

### Agent Selection for Releases

**✅ Use: release-validator agent (`@release-validator`)**
- NOW has full tool access including file editing
- Can edit files, run commands, create commits
- Can execute entire release workflow end-to-end
- **Preferred agent for release operations**

**✅ Use: Alteriom AI Agent (`@alteriom-ai-agent`)**
- Has full tool access via MCP (Model Context Protocol)
- Can edit files, run commands, create commits
- Can execute entire release workflow end-to-end
- **Alternative agent with full capabilities**

**❌ Do NOT use: painlessMesh Coordinator (`@painlessmesh-coordinator`)**
- Repository coordinator lacks file editing tools
- Cannot execute release workflow tasks
- Can only provide context and planning
- Will document what needs to be done but cannot implement

**✅ Use: Release Validation Script (`./scripts/release-agent.sh`)**
- Automated bash script for pre-release validation
- Checks version consistency, tests, documentation
- Can be run manually or in CI/CD pipelines
- Does not require agent interaction

### Workflow Example

```
User: "Prepare release v1.9.9"
  ↓
@alteriom-ai-agent (with full tools):
  ✅ Updates library.properties version
  ✅ Updates library.json version
  ✅ Updates package.json version
  ✅ Updates painlessMesh.h @version and @date
  ✅ Updates AlteriomPainlessMesh.h VERSION constants
  ✅ Updates README.md version banner
  ✅ Restructures CHANGELOG.md with release section
  ✅ Runs validation: ./scripts/release-agent.sh
  ✅ Commits: "release: v1.9.9 - Description"
  ✅ Pushes to trigger automated workflows
  ✓ Complete release prepared
```

### Files Requiring Updates for Each Release

The following 7 files MUST be updated for every release:

1. **library.properties** - `version=X.Y.Z`
2. **library.json** - `"version": "X.Y.Z"`
3. **package.json** - `"version": "X.Y.Z"`
4. **src/painlessMesh.h** - `@version X.Y.Z` and `@date YYYY-MM-DD`
5. **src/AlteriomPainlessMesh.h** - Version string and VERSION_PATCH constant
6. **README.md** - Version banner (optional but recommended)
7. **CHANGELOG.md** - Move [Unreleased] items to [X.Y.Z] section with date

**Version Synchronization Tool:**
```bash
# Use bump-version.sh for consistent updates
./scripts/bump-version.sh patch X.Y.Z
```

## Agent Responsibilities

### Pre-Release Validation

The release agent MUST perform the following checks before allowing a release:

1. **Version Consistency Check**
   - Verify `library.properties`, `library.json`, and `package.json` all have the same version
   - Ensure version follows semantic versioning (X.Y.Z format)
   - Confirm version hasn't been released previously (no existing git tag)

2. **Documentation Validation**
   - Verify CHANGELOG.md has an entry for the current version
   - Check that CHANGELOG entry includes date in format: `## [X.Y.Z] - YYYY-MM-DD`
   - Ensure CHANGELOG entry has at least one of: Added, Changed, Fixed, or Deprecated sections
   - Verify README.md mentions the latest version or says "Development Version"
   - Check that all internal documentation links are valid (no broken relative paths)

3. **Code Quality Checks**
   - All unit tests must pass (run via `run-parts --regex catch_ bin/`)
   - CMake configuration must succeed
   - No uncommitted changes in tracked files (except in CI environment)
   - Build artifacts should not be committed (check .gitignore)

4. **Dependency Validation**
   - Verify all dependencies in library.properties are also in library.json
   - Check that dependency versions are specified with ranges (^X.Y.Z)
   - Ensure platform-specific dependencies (AsyncTCP, ESPAsyncTCP) are properly configured

5. **Example Code Validation**
   - All examples in `examples/` directory must have valid platformio.ini files
   - Examples must include proper library dependencies
   - No syntax errors in example .ino files (basic validation)

6. **Release Workflow Validation**
   - Verify .github/workflows/release.yml exists and is properly configured
   - Check that release workflow has required permissions (contents: write, packages: write)
   - Ensure NPM_TOKEN and PLATFORMIO_AUTH_TOKEN secrets are configured (check documentation)
   - Validate that all publishing steps are enabled

### Release Process

The agent SHOULD guide the release through these steps:

1. **Preparation Phase**
   - Run `./scripts/validate-release.sh` to perform all pre-release checks
   - Generate a release summary showing:
     - Version number
     - Changes from CHANGELOG
     - Distribution channels (GitHub, NPM, PlatformIO, Arduino)
     - Required manual steps (if any)

2. **Commit Phase**
   - Ensure commit message starts with `release:` prefix
   - Commit message format: `release: vX.Y.Z - Brief description`
   - Example: `release: v1.7.9 - CI/CD improvements and documentation updates`

3. **Automation Phase**
   - Push to main branch triggers automated release workflow
   - Workflow creates git tag `vX.Y.Z`
   - Workflow creates GitHub release with CHANGELOG excerpt
   - Workflow publishes to NPM public registry
   - Workflow publishes to GitHub Packages (if configured)
   - Workflow updates GitHub Wiki with latest documentation

4. **Verification Phase**
   - Verify GitHub release was created: `https://github.com/Alteriom/painlessMesh/releases/tag/vX.Y.Z`
   - Verify NPM package was published: `npm view @alteriom/painlessmesh@X.Y.Z`
   - Check GitHub Packages (may fail gracefully if not configured)
   - Monitor PlatformIO Registry for automatic indexing (may take a few hours)
   - Verify Wiki was updated: `https://github.com/Alteriom/painlessMesh/wiki`

### Post-Release Tasks

After a successful release, the agent SHOULD:

1. **Update Documentation**
   - Ensure README.md shows the latest stable version
   - Update any "Development Version" references
   - Verify all badges show correct version numbers

2. **Prepare for Next Development Cycle**
   - Update CHANGELOG.md with new [Unreleased] section
   - Add TBD placeholders for Added, Changed, Fixed sections
   - Optionally bump version to next patch/minor/major (if planning ahead)

3. **Communication**
   - Generate release announcement with key changes
   - List all distribution channels where package is available
   - Provide installation instructions for each channel

4. **Monitoring**
   - Check GitHub Actions workflow status
   - Monitor for any publication failures
   - Track downloads/usage metrics if available

## Agent Decision Tree

```
START
  │
  ├─ Is version consistent across all files?
  │  ├─ NO → FAIL: Run ./scripts/bump-version.sh to fix
  │  └─ YES → Continue
  │
  ├─ Does CHANGELOG have entry for this version?
  │  ├─ NO → FAIL: Add CHANGELOG entry
  │  └─ YES → Continue
  │
  ├─ Do all tests pass?
  │  ├─ NO → FAIL: Fix failing tests
  │  └─ YES → Continue
  │
  ├─ Does git tag already exist?
  │  ├─ YES → FAIL: Version already released
  │  └─ NO → Continue
  │
  ├─ Are there uncommitted changes?
  │  ├─ YES → WARN: Consider committing changes
  │  └─ NO → Continue
  │
  ├─ Is release workflow properly configured?
  │  ├─ NO → FAIL: Check .github/workflows/release.yml
  │  └─ YES → Continue
  │
  ├─ All checks passed?
  │  └─ YES → READY FOR RELEASE
  │
  └─ Execute Release:
     ├─ Commit with "release: vX.Y.Z" message
     ├─ Push to main branch
     ├─ Wait for GitHub Actions
     ├─ Verify all publication steps
     └─ Generate success report
```

## Usage Instructions

### For Automated CI/CD

The release agent is integrated into GitHub Actions workflows:

1. **Validate Release Workflow** (`.github/workflows/validate-release.yml`)
   - Automatically runs on every push to main/develop
   - Performs pre-release validation checks
   - Blocks release if any validation fails

2. **Release Workflow** (`.github/workflows/release.yml`)
   - Triggered by commits with version changes
   - Requires commit message starting with `release:`
   - Executes full release pipeline

### For Manual Release

To manually trigger the release agent checks:

```bash
# 1. Validate current state
./scripts/validate-release.sh

# 2. If validation passes, commit and push
git add library.properties library.json package.json CHANGELOG.md
git commit -m "release: v1.7.9 - CI/CD improvements and documentation updates"
git push origin main

# 3. Monitor GitHub Actions
# Visit: https://github.com/Alteriom/painlessMesh/actions
```

### For Manual Intervention

If automated release fails, use manual publishing workflow:

```bash
# Trigger manual NPM/GitHub Packages publishing
gh workflow run manual-publish.yml
```

## Configuration

### Required Secrets

The following GitHub repository secrets must be configured:

- `NPM_TOKEN` - NPM authentication token for publishing to npmjs.com
  - Generate at: https://www.npmjs.com/settings/tokens
  - Scope: Automation token with publish permissions

- `PLATFORMIO_AUTH_TOKEN` - PlatformIO authentication token
  - Generate at: https://platformio.org/account/token
  - Used by platformio-publish.yml workflow

- `GITHUB_TOKEN` - Automatically provided by GitHub Actions
  - No manual configuration needed
  - Used for releases, wiki, and GitHub Packages

### Required Permissions

The release workflow requires:

```yaml
permissions:
  contents: write      # Create releases and tags
  packages: write      # Publish to GitHub Packages
  id-token: write      # NPM publishing
  actions: read        # Workflow status
```

## Release Checklist

Use this checklist for every release:

### Pre-Release Preparation
- [ ] **Verify agent has file editing tools** (`replace_string_in_file`, `multi_replace_string_in_file`)
- [ ] **Verify agent has git/terminal tools** (`run_in_terminal`, `get_changed_files`)
- [ ] All tests pass locally (`cmake -G Ninja . && ninja && run-parts --regex catch_ bin/`)
- [ ] Version bumped in all 7 files (see "Files Requiring Updates" section above)
  - [ ] library.properties
  - [ ] library.json
  - [ ] package.json
  - [ ] src/painlessMesh.h (@version and @date)
  - [ ] src/AlteriomPainlessMesh.h (VERSION string and PATCH)
  - [ ] README.md (version banner)
  - [ ] CHANGELOG.md (release section with date)
- [ ] No uncommitted changes or build artifacts
- [ ] Documentation reviewed and up to date
- [ ] Release validation passed (`./scripts/release-agent.sh`)

### Release Execution
- [ ] Commit with `release: vX.Y.Z` prefix
- [ ] Push to main branch
- [ ] Monitor GitHub Actions workflows

### Post-Release Verification
- [ ] Verify GitHub release created
- [ ] Verify NPM package published
- [ ] Verify Wiki updated
- [ ] Monitor PlatformIO Registry (automatic indexing, may take hours)
- [ ] Submit to Arduino Library Manager (first release only)

## Error Recovery

### Common Issues and Solutions

**Version Mismatch**
```bash
# Problem: Versions don't match across files
# Solution: Use bump script to synchronize
./scripts/bump-version.sh patch 1.7.9
```

**Missing CHANGELOG Entry**
```bash
# Problem: No CHANGELOG entry for version
# Solution: Add entry manually
vim CHANGELOG.md
# Add: ## [1.7.9] - 2025-11-08
```

**Tag Already Exists**
```bash
# Problem: Version tag already exists
# Solution: Either delete tag or bump version

# Option 1: Delete tag (if release was bad)
git push origin :refs/tags/v1.7.9
git tag -d v1.7.9

# Option 2: Bump to next version
./scripts/bump-version.sh patch
```

**Build Failures**
```bash
# Problem: Tests failing or build errors
# Solution: Fix issues before attempting release

# Run tests locally
cmake -G Ninja .
ninja
run-parts --regex catch_ bin/

# Debug specific test
./bin/catch_alteriom_packages -s
```

**NPM Publishing Failed**
```bash
# Problem: Automated NPM publish failed
# Solution: Use manual publish workflow

# Via GitHub CLI
gh workflow run manual-publish.yml

# Or via GitHub web interface
# Go to: Actions → Manual Package Publishing → Run workflow
```

**Wiki Update Failed**
```bash
# Problem: Wiki not updating automatically
# Solution: Initialize wiki manually

# 1. Visit: https://github.com/Alteriom/painlessMesh/wiki
# 2. Create a page to initialize wiki
# 3. Re-run release workflow
```

## Agent Maintenance

This release agent configuration should be reviewed and updated:

- **Quarterly**: Review for process improvements
- **After Failed Release**: Update with lessons learned
- **When Tools Change**: Update for new CI/CD tools or workflows
- **When Requirements Change**: Add new validation checks as needed

## Version History

- **v1.0** (2025-11-08): Initial release agent creation
  - Comprehensive pre-release validation
  - Multi-channel publishing (GitHub, NPM, PlatformIO, Arduino)
  - Automated wiki synchronization
  - Post-release verification steps

## References

- [RELEASE_GUIDE.md](../../RELEASE_GUIDE.md) - Detailed release process documentation
- [validate-release.sh](../../scripts/validate-release.sh) - Pre-release validation script
- [bump-version.sh](../../scripts/bump-version.sh) - Version management script
- [release.yml](../workflows/release.yml) - Automated release workflow
- [validate-release.yml](../workflows/validate-release.yml) - Release validation workflow
- [Keep a Changelog](https://keepachangelog.com/) - Changelog format standard
- [Semantic Versioning](https://semver.org/) - Version numbering standard
