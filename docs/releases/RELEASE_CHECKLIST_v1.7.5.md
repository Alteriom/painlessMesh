# Release Checklist - painlessMesh v1.7.5

**Release Date:** October 19, 2025  
**Release Manager:** Alteriom

## Pre-Release ✅ COMPLETE

- [x] All CI/CD tests passing
- [x] Version numbers updated (library.json, library.properties, package.json)
- [x] CHANGELOG.md updated with v1.7.5 changes
- [x] Release summary document created (RELEASE_SUMMARY_v1.7.5.md)
- [x] Git commit created with release changes
- [x] Git tag v1.7.5 created and pushed
- [x] All changes pushed to GitHub main branch

## GitHub Release 📦

- [ ] **Navigate to:** https://github.com/Alteriom/painlessMesh/releases/new
- [ ] **Tag:** Select `v1.7.5` from dropdown
- [ ] **Title:** `v1.7.5 - TaskScheduler Compatibility & CI/CD Fixes`
- [ ] **Description:** Use template below
- [ ] **Set as latest release:** ✅ Checked
- [ ] **Create discussion:** ✅ Checked (optional)
- [ ] Click **Publish Release**

### GitHub Release Description Template

```markdown
# painlessMesh v1.7.5 - TaskScheduler Compatibility & CI/CD Fixes

**Release Date:** October 19, 2025  
**Release Type:** Patch Release (Critical Bug Fix)

## 🚨 Critical Fixes

This release addresses TaskScheduler compatibility issues and CI/CD build failures discovered during v1.7.4 testing.

### Key Changes

- **Reverted Thread-Safe Mode**: Disabled `_TASK_THREAD_SAFE` due to architectural incompatibility with TaskScheduler v4.0.x
- **Fixed Build System**: Added `lib_ldf_mode = deep+` to all example configurations
- **Corrected Include Order**: Examples now include `painlessTaskOptions.h` before `TaskScheduler.h`
- **FreeRTOS Crash Reduction**: Maintained at ~85% via semaphore timeout increase (Option A)

## 📋 What's Changed

### TaskScheduler Compatibility

TaskScheduler v4.0.x cannot use `_TASK_THREAD_SAFE` and `_TASK_STD_FUNCTION` simultaneously. Since painlessMesh requires std::function for lambda callbacks throughout the codebase, we've disabled thread-safe mode and rely solely on the semaphore timeout fix.

**Trade-off:**
- ✅ Library functionality fully restored
- ✅ All CI/CD builds passing
- ⚠️ FreeRTOS crash reduction: ~85% (vs 95-98% with dual approach)
- ✅ ESP32 crash rate: ~5-8% (acceptable for production)

### CI/CD Improvements

- Fixed PlatformIO Library Dependency Finder configuration
- Corrected include order in 6 example sketches
- Added feature build flags for `alteriomImproved` example
- All 19 examples now build successfully

## 🔧 Installation

### PlatformIO
```bash
pio pkg update sparck75/AlteriomPainlessMesh
```

### NPM
```bash
npm update @alteriom/painlessmesh
```

### Arduino IDE
Library Manager → Search "painlessMesh" → Update to v1.7.5

## 📚 Documentation

- [CHANGELOG.md](CHANGELOG.md) - Full version history
- [Release Summary](docs/releases/RELEASE_SUMMARY_v1.7.5.md) - Technical details
- [Troubleshooting Guide](docs/troubleshooting/SENSOR_NODE_CONNECTION_CRASH.md) - FreeRTOS crash information

## ⚙️ Technical Details

**Commits:** 8869db8, f7ad959, 6f477a0, fe17e72, d8d2ff0  
**Files Changed:** 32 files  
**Test Status:** ✅ 710+ tests passing  
**Build Status:** ✅ All platforms passing

## 🆕 What's Next?

Future releases will explore:
- TaskScheduler v4.1+ compatibility when available
- Alternative task scheduler options
- Custom thread-safe implementation
- Further FreeRTOS crash mitigation strategies

## 🙏 Acknowledgments

Thank you to the community for testing and reporting the v1.7.4 CI/CD issues.

---

**Full Changelog**: https://github.com/Alteriom/painlessMesh/compare/v1.7.4...v1.7.5
```

## NPM Publishing 📦

### Prerequisites
- [ ] Verify you're logged in to NPM: `npm whoami`
- [ ] Verify package.json version is 1.7.5
- [ ] Verify you're on main branch with latest changes

### Steps

1. **Test the package locally** (optional but recommended):
   ```bash
   npm pack
   # This creates alteriom-painlessmesh-1.7.5.tgz
   ```

2. **Publish to NPM:**
   ```bash
   npm publish --access public
   ```

3. **Verify publication:**
   ```bash
   npm view @alteriom/painlessmesh version
   # Should show: 1.7.5
   ```

4. **Check on NPM website:**
   - Visit: https://www.npmjs.com/package/@alteriom/painlessmesh
   - Verify version shows 1.7.5
   - Check that "Latest" tag is applied

### NPM Checklist
- [ ] Package built successfully (npm pack)
- [ ] Published to NPM registry
- [ ] Version 1.7.5 visible on npmjs.com
- [ ] "Latest" tag applied correctly
- [ ] Package downloadable: `npm install @alteriom/painlessmesh@1.7.5`

## PlatformIO Registry 📦

### Prerequisites
- [ ] Verify PlatformIO account credentials
- [ ] Verify library.json version is 1.7.5

### Automatic Registry Update

PlatformIO automatically detects new GitHub releases and updates the registry within 24 hours.

**Monitor at:** https://registry.platformio.org/libraries/sparck75/AlteriomPainlessMesh

### Manual Trigger (if needed)

If automatic update doesn't occur within 24 hours:

1. **Via PlatformIO CLI:**
   ```bash
   pio pkg publish .
   ```

2. **Via Web Interface:**
   - Login to https://platformio.org
   - Navigate to your library page
   - Click "Sync from GitHub"

### PlatformIO Checklist
- [ ] Monitored registry for automatic update
- [ ] Version 1.7.5 appears in registry (within 24 hours)
- [ ] Library downloadable: `pio pkg install sparck75/AlteriomPainlessMesh@1.7.5`
- [ ] Verified on: https://registry.platformio.org/libraries/sparck75/AlteriomPainlessMesh

## Arduino Library Manager 📦

### Automatic Sync

Arduino Library Manager automatically syncs from GitHub releases within 24-48 hours.

**No manual action required!**

### Verification Steps

After 24-48 hours:

1. **Check Arduino Library Manager:**
   - Open Arduino IDE
   - Tools → Manage Libraries
   - Search: "painlessMesh" or "Alteriom"
   - Verify version 1.7.5 is available

2. **Check Arduino Library List:**
   - Visit: https://www.arduinolibraries.info/libraries/alteriom-painless-mesh
   - Verify latest version shows 1.7.5

### Arduino Checklist
- [ ] Waited 24-48 hours for automatic sync
- [ ] Version 1.7.5 visible in Arduino IDE Library Manager
- [ ] Library installable from Arduino IDE
- [ ] Verified on arduinolibraries.info

## Documentation Updates 📝

- [ ] Update troubleshooting docs with v1.7.5 reference
- [ ] Update main README.md if needed
- [ ] Verify all internal links work
- [ ] Check docs website (if applicable)

## Communication 📢

### Announcements

- [ ] **GitHub Discussions:** Create announcement post
- [ ] **Issues:** Close related issues (#TODO if applicable)
- [ ] **Pull Requests:** Reference in relevant PRs
- [ ] **Social Media:** Announce on relevant platforms (if applicable)

### Template for GitHub Discussion

```markdown
# painlessMesh v1.7.5 Released! 🎉

We're pleased to announce painlessMesh v1.7.5, a critical patch release addressing TaskScheduler compatibility and CI/CD build issues.

## What's Fixed

- ✅ TaskScheduler compatibility (reverted thread-safe mode)
- ✅ CI/CD build system configuration
- ✅ Example include order corrections
- ✅ FreeRTOS crash reduction maintained at ~85%

## Upgrade Now

**PlatformIO:** `pio pkg update sparck75/AlteriomPainlessMesh`  
**NPM:** `npm update @alteriom/painlessmesh`  
**Arduino IDE:** Library Manager → Update

## Details

See the full [Release Notes](https://github.com/Alteriom/painlessMesh/releases/tag/v1.7.5) and [Technical Summary](https://github.com/Alteriom/painlessMesh/blob/main/docs/releases/RELEASE_SUMMARY_v1.7.5.md).

Questions? Ask below! 👇
```

## Monitoring & Follow-up 👀

### First 24 Hours

- [ ] Monitor GitHub issues for bug reports
- [ ] Check CI/CD for any failures
- [ ] Verify all download links work
- [ ] Respond to community questions

### First Week

- [ ] Verify PlatformIO registry updated
- [ ] Verify Arduino Library Manager sync completed
- [ ] Check download statistics
- [ ] Gather user feedback

### First Month

- [ ] Review crash reports from ESP32 deployments
- [ ] Monitor for TaskScheduler upstream updates
- [ ] Plan next release if issues found
- [ ] Update roadmap based on feedback

## Rollback Plan 🔄

If critical issues are discovered:

1. **Immediate:**
   - Mark GitHub Release as "Pre-release"
   - Add warning to release description
   - Create GitHub Discussion with details

2. **Within 24 hours:**
   - Prepare hotfix release (v1.7.6)
   - Or revert to v1.7.4 if necessary
   - Publish corrected version

3. **Communication:**
   - Update all announcements
   - Notify users via GitHub Issues
   - Update documentation

## Success Criteria ✅

Release is considered successful when:

- [x] Git tag v1.7.5 pushed to GitHub
- [ ] GitHub Release published
- [ ] NPM package published and downloadable
- [ ] PlatformIO registry updated (within 24 hours)
- [ ] Arduino Library Manager synced (within 48 hours)
- [ ] No critical bugs reported in first 48 hours
- [ ] CI/CD builds passing on all platforms
- [ ] Community feedback positive or neutral

## Notes

**Date Started:** October 19, 2025  
**Date Completed:** _To be filled_  
**Time to Complete:** _To be filled_  
**Issues Encountered:** _To be filled_

---

**Release Manager Signature:** _______________  
**Date:** October 19, 2025
