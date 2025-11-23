# Release Checklist for v1.8.15

**Release Date**: November 23, 2025

## Pre-Release Verification

### Version Updates
- [x] Updated `library.json` to version 1.8.15
- [x] Updated `library.properties` to version 1.8.15
- [x] Updated `package.json` to version 1.8.15
- [x] Updated `CHANGELOG.md` with v1.8.15 entries
- [x] Created `RELEASE_NOTES_1.8.15.md`
- [x] Created this checklist

### Testing
- [x] All unit tests passing (119+ assertions)
  - [x] catch_tcp_integration (113 assertions)
  - [x] catch_connection (6 assertions)
  - [x] 30+ additional unit tests
- [x] Simulator tests passing
  - [x] Basic example test (5 nodes)
  - [x] Mesh formation validated
  - [x] Message broadcasting validated
- [x] CodeQL security scan passing
- [x] All CI/CD jobs passing
  - [x] Desktop builds
  - [x] Arduino builds
  - [x] PlatformIO builds
  - [x] Simulator tests
  - [x] Code quality checks

### Documentation
- [x] CHANGELOG.md updated
- [x] Release notes created
- [x] README.md reviewed (no updates needed)
- [x] DOCUMENTATION_INDEX.md reviewed (no updates needed)
- [x] New documentation files added:
  - [x] RELEASE_READINESS_PLAN.md
  - [x] TESTING_WITH_SIMULATOR.md
  - [x] docs/SIMULATOR_TESTING.md
  - [x] examples/basic/test/simulator/README.md

### Code Quality
- [x] No compiler warnings
- [x] No linting errors
- [x] Code formatted according to style guide
- [x] All examples compile successfully

### Security
- [x] Security vulnerabilities addressed (none found)
- [x] CodeQL scan passing
- [x] Dependencies up to date
- [x] No exposed secrets or credentials

## Release Process

### 1. Merge Pull Request
- [ ] Review final PR changes
- [ ] Ensure all CI checks pass
- [ ] Merge PR #164 to main branch
- [ ] Delete feature branch after merge

### 2. Create Git Tag
```bash
git checkout main
git pull origin main
git tag -a v1.8.15 -m "Release v1.8.15 - Simulator integration and release readiness"
git push origin v1.8.15
```

### 3. Create GitHub Release
- [ ] Go to GitHub Releases page
- [ ] Click "Draft a new release"
- [ ] Select tag: v1.8.15
- [ ] Release title: "v1.8.15 - Simulator Integration"
- [ ] Copy content from RELEASE_NOTES_1.8.15.md
- [ ] Mark as latest release
- [ ] Publish release

### 4. Publish to Package Registries

#### NPM Registry
```bash
# Ensure you're on main branch with latest changes
git checkout main
git pull origin main

# Verify package.json version is 1.8.15
cat package.json | grep version

# Test package locally
npm run validate-library

# Publish to NPM (requires npm login)
npm publish --access public
```

#### PlatformIO Registry
```bash
# PlatformIO will automatically detect the new tag
# Verify at: https://registry.platformio.org/libraries/alteriom/AlteriomPainlessMesh
# May take a few minutes to sync
```

#### Arduino Library Manager
```bash
# Arduino will automatically detect the new release from library.properties
# Verify at: https://www.arduino.cc/reference/en/libraries/alteriom-painlessmesh/
# May take 1-2 hours to sync
```

### 5. Post-Release Tasks

#### Update Documentation Website
- [ ] Ensure docs-website is updated (if applicable)
- [ ] Verify links to release notes work
- [ ] Update any version references in documentation

#### Close Related Issues
- [ ] Close issue #163 (Improve validation) with reference to v1.8.15
- [ ] Reference PR #164 in issue closure

#### Announcements
- [ ] Post announcement in GitHub Discussions (if enabled)
- [ ] Update README badges if needed
- [ ] Notify stakeholders/users of the release

#### Verification
- [ ] Verify NPM package: `npm view @alteriom/painlessmesh version`
- [ ] Verify PlatformIO: Check registry page
- [ ] Verify Arduino: Check library manager
- [ ] Test installation: `pio lib install AlteriomPainlessMesh@1.8.15`

## Post-Release Verification

### Installation Testing
```bash
# Test PlatformIO installation
pio lib install AlteriomPainlessMesh@1.8.15

# Test NPM installation
npm install @alteriom/painlessmesh@1.8.15

# Test Arduino IDE
# Library Manager -> Search "Alteriom PainlessMesh" -> Install v1.8.15
```

### Smoke Tests
- [ ] Basic example compiles and runs
- [ ] Bridge example compiles
- [ ] MQTT bridge example compiles
- [ ] Simulator test runs successfully

## Rollback Procedure (If Needed)

If critical issues are discovered after release:

1. **Immediate Actions**
   ```bash
   # Deprecate NPM package
   npm deprecate @alteriom/painlessmesh@1.8.15 "Critical issue found, use v1.8.14"
   
   # Mark GitHub release as pre-release
   # Edit release on GitHub, check "This is a pre-release"
   ```

2. **Fix and Re-release**
   - Create hotfix branch from v1.8.15 tag
   - Apply fix
   - Release as v1.8.16
   - Update deprecation message to point to v1.8.16

3. **Communication**
   - Post issue on GitHub with details
   - Update release notes with errata
   - Notify users through available channels

## Release Sign-off

- [ ] **Technical Lead**: All tests passing, code quality verified
- [ ] **Maintainer**: Documentation complete, ready for release
- [ ] **CI/CD**: All automated checks passing
- [ ] **Security**: No vulnerabilities detected
- [ ] **Final Review**: Release notes accurate and complete

## Notes

### Key Features in This Release
- Simulator integration for automated example validation
- CI/CD pipeline enhancement with simulator tests
- Comprehensive release readiness assessment
- Production-ready confirmation with 119+ passing tests

### Breaking Changes
None. Fully backward compatible with v1.8.14.

### Known Limitations
- Simulator currently tests only basic.ino example
- Additional example scenarios can be added incrementally
- Custom firmware requires contributions to painlessMesh-simulator repo

### Future Improvements
- Expand simulator test coverage to all examples
- Add performance benchmarking scenarios
- Test with 100+ node simulations
- Add network failure/recovery scenarios

## References

- [Release Guide](RELEASE_GUIDE.md)
- [Release Readiness Plan](RELEASE_READINESS_PLAN.md)
- [Changelog](CHANGELOG.md)
- [Release Notes](RELEASE_NOTES_1.8.15.md)
- [Issue #163](https://github.com/Alteriom/painlessMesh/issues/163)
- [PR #164](https://github.com/Alteriom/painlessMesh/pull/164)

---

**Checklist Status**: Ready for release ✅

All pre-release tasks completed. Proceed with merge and release process.
