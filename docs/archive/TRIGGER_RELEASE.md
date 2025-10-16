# Release Trigger Guide - Alteriom painlessMesh

## 🚀 Quick Release Process

### Standard Release (Recommended)

```bash
# 1. Update version using the bump script
./scripts/bump-version.sh patch  # or minor, major

# 2. Update CHANGELOG.md with your changes
# Add your changes under the new version section

# 3. Commit and trigger release
git add library.properties library.json CHANGELOG.md package.json
git commit -m "release: v1.6.1"
git push origin main
```

That's it! GitHub Actions will automatically:
- ✅ Run comprehensive test suite
- ✅ Create git tag
- ✅ Generate GitHub release with changelog
- ✅ Publish to NPM (public registry)
- ✅ Publish to GitHub Packages
- ✅ Update GitHub Wiki
- ✅ Package library for Arduino Library Manager
- ✅ Validate all distribution channels

## 📋 Release Checklist

### Pre-Release Validation
- [ ] All tests passing locally (`npm run test`)
- [ ] Version consistency verified (`./scripts/validate-release.sh`)
- [ ] Changelog updated with new version
- [ ] Examples compile successfully
- [ ] Documentation updated if needed

### Release Execution
- [ ] Version bumped in all files
- [ ] Commit message starts with "release:"
- [ ] Pushed to main branch
- [ ] GitHub Actions workflows completed successfully

### Post-Release Verification
- [ ] GitHub release created
- [ ] NPM package published
- [ ] GitHub Packages updated
- [ ] Wiki synchronized
- [ ] Library available in distribution channels

## 🎯 Distribution Channels

### Automatic (Zero Manual Work)
1. **GitHub Releases** - Created automatically with changelog
2. **NPM Public Registry** - Published to https://www.npmjs.com/package/@alteriom/painlessmesh
3. **GitHub Packages** - Published to GitHub's NPM registry
4. **PlatformIO Registry** - Automatically indexed from GitHub releases
5. **GitHub Wiki** - Documentation synchronized automatically

### Semi-Automatic (Requires Manual Submission)
6. **Arduino Library Manager** - Requires one-time submission to Arduino team

## 📝 Version Management

### Semantic Versioning

```bash
# Patch version (1.6.0 → 1.6.1) - Bug fixes
./scripts/bump-version.sh patch

# Minor version (1.6.0 → 1.7.0) - New features
./scripts/bump-version.sh minor

# Major version (1.6.0 → 2.0.0) - Breaking changes
./scripts/bump-version.sh major

# Specific version
./scripts/bump-version.sh patch 1.6.2
```

### Version Consistency

The script automatically updates:
- `library.properties` (Arduino)
- `library.json` (PlatformIO)
- `package.json` (NPM)

## 🔧 NPM Publishing Details

### Dual Publishing Strategy

The release workflow publishes to **two NPM registries**:

1. **Public NPM** (npmjs.com)
   - Package: `@alteriom/painlessmesh`
   - Anyone can install: `npm install @alteriom/painlessmesh`
   - No authentication required for installation

2. **GitHub Packages** (npm.pkg.github.com)
   - Package: `@alteriom/painlessmesh`
   - Scoped to Alteriom organization
   - Requires authentication for installation

### Installation Methods

```bash
# Public NPM (recommended for users)
npm install @alteriom/painlessmesh

# GitHub Packages (for Alteriom team/authenticated users)
echo '@alteriom:registry=https://npm.pkg.github.com' >> .npmrc
npm install @alteriom/painlessmesh
```

## 📚 Wiki Publishing

### Automatic Synchronization

The wiki is automatically updated with each release:

- **Home Page** - Generated from README.md
- **Release Guide** - From RELEASE_GUIDE.md
- **Changelog** - From CHANGELOG.md
- **Examples** - Auto-generated from examples directory
- **API Reference** - Generated documentation pages
- **CI/CD Pipeline** - Automated documentation

### Manual Wiki Override

If needed, you can manually update the wiki:

```bash
# Clone wiki repository
git clone https://github.com/Alteriom/painlessMesh.wiki.git

# Edit markdown files
# Commit and push changes
```

## 🛠️ Arduino Library Manager Submission

### One-Time Setup (Required)

After your first release, submit to Arduino Library Manager:

1. Go to: https://github.com/arduino/library-registry
2. Create a new issue with the following template:

```
Title: Add painlessMesh library

Body:
Repository URL: https://github.com/Alteriom/painlessMesh
Release tag: v1.6.0
Library name: painlessMesh
Version: 1.6.0

This is the Alteriom fork of the painlessMesh library with enhanced 
CI/CD, automated releases, and improved Arduino Library Manager compatibility.
Includes SensorPackage, CommandPackage, and StatusPackage extensions.
```

3. Monitor the issue for Arduino team approval
4. Once approved, all future releases are automatically indexed

## 🔍 Validation Scripts

### Pre-Release Validation
```bash
# Comprehensive validation
./scripts/validate-release.sh

# Check version consistency
./scripts/bump-version.sh --verify

# Test library compilation
npm run build
npm run test
```

### Release Monitoring
```bash
# Check GitHub Actions status
# Visit: https://github.com/Alteriom/painlessMesh/actions

# Verify NPM publication
npm view @alteriom/painlessmesh version

# Check GitHub Packages
# Visit: https://github.com/Alteriom/painlessMesh/packages

# Verify wiki update
# Visit: https://github.com/Alteriom/painlessMesh/wiki
```

## 🚨 Troubleshooting

### Common Issues

**Version Mismatch Error**
```bash
# Fix version inconsistencies
./scripts/bump-version.sh patch 1.6.1  # Force set version
```

**NPM Publication Failure**
```bash
# Check NPM token is valid
npm whoami

# Verify package.json configuration
npm run validate-library
```

**GitHub Packages Authentication**
```bash
# Verify GitHub token has packages:write permission
# Check repository settings → Actions → General → Permissions
```

**Wiki Update Failure**
```bash
# Wiki may need to be manually initialized
# Go to: https://github.com/Alteriom/painlessMesh/wiki
# Create any page to initialize wiki, then re-run release
```

### Manual Override

If automation fails, you can manually perform any step:

```bash
# Manual NPM publish
npm publish --access public

# Manual GitHub release
gh release create v1.6.1 --title "painlessMesh v1.6.1" --notes-file CHANGELOG.md

# Manual tag creation
git tag v1.6.1
git push origin v1.6.1
```

## 📊 Release Success Indicators

### Successful Release Shows:
- ✅ GitHub release created with changelog
- ✅ Git tag pushed to repository
- ✅ NPM package published (check npmjs.com)
- ✅ GitHub Packages updated
- ✅ Wiki pages synchronized
- ✅ All GitHub Actions workflows completed

### Distribution Verification:
```bash
# NPM
npm view @alteriom/painlessmesh

# GitHub Packages  
npm view @alteriom/painlessmesh --registry=https://npm.pkg.github.com

# PlatformIO (updated within 24 hours)
# Check: https://registry.platformio.org/libraries/alteriom/painlessMesh
```

## 🎉 Success!

Once your release is complete, the library will be available through:
- GitHub Releases
- NPM (public + GitHub Packages)
- PlatformIO Registry (automatic)
- Arduino Library Manager (after initial submission)
- GitHub Wiki (documentation)

All future releases follow the same simple process - just bump version, update changelog, commit with "release:" prefix, and push!

---

**Questions?** Create an issue with the `ci/cd` label for release process support.