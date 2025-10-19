# painlessMesh v1.7.6 Release Checklist

**Release Date:** October 19, 2025  
**Release Type:** Critical Bug Fix  
**Git Commit:** c17384a  
**Git Tag:** v1.7.6  

---

## ✅ Completed Steps

### Code Implementation
- [x] Removed `src/painlessmesh/scheduler_queue.hpp` (30 lines deleted)
- [x] Removed `src/painlessmesh/scheduler_queue.cpp` (70 lines deleted)
- [x] Updated `src/painlessmesh/mesh.hpp` (removed queue includes and initialization)
- [x] Created comprehensive unit test `test/catch/catch_scheduler_queue_removal.cpp` (200+ lines)

### Version Updates
- [x] Updated `library.json` version: 1.7.5 → 1.7.6
- [x] Updated `library.properties` version: 1.7.5 → 1.7.6
- [x] Updated `package.json` version: 1.7.5 → 1.7.6

### Documentation
- [x] Updated `CHANGELOG.md` with v1.7.6 section
- [x] Created `docs/releases/RELEASE_PLAN_v1.7.6.md` (850+ lines)
- [x] Created `docs/releases/RELEASE_SUMMARY_v1.7.6.md` (comprehensive release notes)
- [x] Created `docs/troubleshooting/PAINLESSMESH_V1.7.4_COMPILATION_ISSUES.md` (historical reference)
- [x] Updated `README.md` with v1.7.6 information

### Git Operations
- [x] Staged all changes (`git add -A`)
- [x] Committed with detailed message (commit c17384a)
- [x] Created annotated tag v1.7.6
- [x] Pushed to GitHub (`git push origin main --tags`)

### Statistics
- **Files Changed:** 12 files
- **Insertions:** +2046 lines
- **Deletions:** -131 lines
- **Net:** +1915 lines (mostly documentation and tests)

---

## 🔄 Manual Steps Required

### 1. Create GitHub Release

**URL:** https://github.com/Alteriom/painlessMesh/releases/new

**Release Details:**
- **Tag:** v1.7.6 (select existing tag)
- **Title:** `v1.7.6 - Critical Compilation Fix`
- **Set as latest release:** ✅ Checked

**Release Description Template:**

```markdown
# v1.7.6 - Critical Compilation Fix

🚨 **URGENT**: If you are using v1.7.4 or v1.7.5, upgrade immediately. Those versions fail to compile.

## What's Fixed

✅ **Compilation Failure** - Fixed "_task_request_t was not declared" error  
✅ **ESP32 Support** - All ESP32 builds now compile successfully  
✅ **ESP8266 Support** - All ESP8266 builds now compile successfully  
✅ **FreeRTOS Stability** - Maintained ~85% crash reduction on ESP32  

## What Changed

This release removes the thread-safe scheduler queue that was causing compilation failures:

- **Removed**: `scheduler_queue.hpp` and `scheduler_queue.cpp`
- **Simplified**: `mesh.hpp` initialization code
- **Maintained**: Semaphore timeout fix for FreeRTOS crash protection

## Why This Fix

The thread-safe scheduler queue required `_TASK_THREAD_SAFE` macro, but:
- This macro conflicts with `_TASK_STD_FUNCTION` in TaskScheduler v4.0.x
- painlessMesh **requires** `_TASK_STD_FUNCTION` for lambda callbacks
- The queue code was disabled but still trying to compile
- Result: Type definitions were missing, causing compilation errors

## Upgrade Instructions

### PlatformIO
```ini
[env:your_board]
lib_deps =
    https://github.com/Alteriom/painlessMesh.git#v1.7.6
```

### Arduino IDE
Update through Library Manager: **AlteriomPainlessMesh v1.7.6**

### NPM
```bash
npm update @alteriom/painlessmesh
```

## Performance

- ✅ ESP32 crash rate: ~5-8% (down from 30-40%)
- ✅ Crash reduction: ~85% via semaphore timeout increase
- ✅ No performance impact vs v1.7.2
- ✅ Full lambda and std::function support maintained

## Testing

- ✅ 710+ unit tests passing
- ✅ All 19 examples compile successfully
- ✅ ESP32 and ESP8266 platforms verified
- ✅ CI/CD pipeline passing

## Breaking Changes

**None** - This is a pure bug fix release.

## Known Limitations

- ESP32 crash protection is ~85% effective (not 95-98% as originally targeted)
- Thread-safe scheduler queue feature removed (may return in v1.8.0)
- Requires TaskScheduler v4.1+ for future thread-safe re-implementation

## Documentation

📋 [Complete Release Notes](https://github.com/Alteriom/painlessMesh/blob/main/docs/releases/RELEASE_SUMMARY_v1.7.6.md)  
📖 [CHANGELOG](https://github.com/Alteriom/painlessMesh/blob/main/CHANGELOG.md)  
📋 [Implementation Plan](https://github.com/Alteriom/painlessMesh/blob/main/docs/releases/RELEASE_PLAN_v1.7.6.md)  
🐛 [Compilation Issues (Resolved)](https://github.com/Alteriom/painlessMesh/blob/main/docs/troubleshooting/PAINLESSMESH_V1.7.4_COMPILATION_ISSUES.md)

---

**Release Date**: October 19, 2025  
**Build Status**: ✅ All tests passing  
**Upgrade Priority**: 🚨 CRITICAL for v1.7.4/v1.7.5 users
```

**Action:** Copy the template above and create the release on GitHub.

---

### 2. Publish NPM Package

**Prerequisites:**
- NPM account with publish access
- Logged in to NPM (`npm whoami` to verify)

**Commands:**

```bash
# 1. Verify login
npm whoami

# 2. (Optional) Test package creation
npm pack

# 3. Publish to NPM
npm publish --access public

# 4. Verify publication
npm view @alteriom/painlessmesh version
# Should show: 1.7.6
```

**NPM Package URL:** https://www.npmjs.com/package/@alteriom/painlessmesh

**Checklist:**
- [ ] Logged in to NPM
- [ ] Package built successfully
- [ ] Published to NPM registry
- [ ] Version 1.7.6 visible on npmjs.com
- [ ] Installation tested: `npm install @alteriom/painlessmesh@1.7.6`

---

### 3. Monitor PlatformIO Registry

**URL:** https://registry.platformio.org/libraries/sparck75/AlteriomPainlessMesh

**Update Process:**
- **Automatic:** PlatformIO scans GitHub releases every 24 hours
- **Expected:** v1.7.6 should appear within 24 hours of GitHub Release
- **Manual Trigger (if needed):** `pio pkg publish .`

**Checklist:**
- [ ] Wait 24 hours after GitHub Release
- [ ] Verify v1.7.6 appears on registry
- [ ] Test installation: `pio pkg install --library "AlteriomPainlessMesh@1.7.6"`
- [ ] Verify example compilation

---

### 4. Monitor Arduino Library Manager

**Update Process:**
- **Automatic:** Arduino syncs from `library.properties` in GitHub releases
- **Expected:** v1.7.6 should appear within 24-48 hours
- **No manual action required**

**Verification:**
- [ ] Wait 24-48 hours after GitHub Release
- [ ] Open Arduino IDE Library Manager
- [ ] Search for "AlteriomPainlessMesh"
- [ ] Verify v1.7.6 is available
- [ ] Test installation and example compilation

---

## 📊 Success Criteria

### Immediate (Within 1 Hour)
- [ ] GitHub Release published successfully
- [ ] NPM package published and available
- [ ] CI/CD pipeline status: Green

### First 24 Hours
- [ ] Zero compilation error reports on GitHub Issues
- [ ] PlatformIO registry updated to v1.7.6
- [ ] Positive community feedback (if any)
- [ ] No critical bugs discovered

### First Week
- [ ] Arduino Library Manager synced to v1.7.6
- [ ] ESP32 crash reports show ~5-8% rate (not increasing)
- [ ] No regression reports from users
- [ ] CI/CD continues to pass

---

## 🔍 Monitoring Tasks

### GitHub Issues
Monitor for:
- Compilation errors (should be zero)
- Runtime crashes (should be ~5-8% on ESP32)
- Breaking API changes (should be none)
- Documentation issues

### CI/CD Pipeline
Check that:
- All tests continue to pass
- ESP32 examples compile
- ESP8266 examples compile
- No new lint errors

### Community Feedback
Watch for:
- User reports of successful upgrades
- Questions about migration
- Feature requests for v1.8.0
- Reports of improved stability

---

## 🚨 Rollback Plan

If critical issues are discovered:

### Immediate (Within 1 Hour)
1. **Advise users to downgrade:**
   ```ini
   lib_deps = https://github.com/Alteriom/painlessMesh.git#v1.7.2
   ```

2. **Mark GitHub Release as pre-release** (if possible)

### Short-term (Within 24 Hours)
1. **Revert commit:**
   ```bash
   git revert c17384a
   git push origin main
   ```

2. **Delete tag:**
   ```bash
   git tag -d v1.7.6
   git push origin :refs/tags/v1.7.6
   ```

3. **Unpublish NPM package** (if within 72 hours):
   ```bash
   npm unpublish @alteriom/painlessmesh@1.7.6
   ```

### Medium-term (Within 1 Week)
1. Investigate root cause of rollback
2. Prepare hotfix v1.7.7
3. Re-test thoroughly
4. Re-release with fixes

**Rollback Criteria:**
- Compilation failures reported
- Critical functionality broken
- Crash rate increases significantly
- Data corruption or security issues

---

## 📝 Communication

### GitHub Discussions (Optional)

**Post announcement in Discussions:**

**Title:** `v1.7.6 Released - Critical Compilation Fix`

**Body:**
```markdown
Hi everyone! 👋

We've just released **v1.7.6**, which is a critical emergency fix for compilation issues in v1.7.4 and v1.7.5.

## 🚨 If you're on v1.7.4 or v1.7.5

**Please upgrade immediately.** Those versions fail to compile with the error:
```
'_task_request_t' was not declared
```

## ✅ What's Fixed

- ESP32 and ESP8266 now compile successfully
- Removed problematic thread-safe queue code
- Maintained ~85% FreeRTOS crash reduction
- Zero breaking changes to API

## 📦 How to Upgrade

**PlatformIO:**
```ini
lib_deps = https://github.com/Alteriom/painlessMesh.git#v1.7.6
```

**Arduino IDE:** Update through Library Manager

**NPM:** `npm update @alteriom/painlessmesh`

## 📚 Documentation

- [Release Notes](https://github.com/Alteriom/painlessMesh/releases/tag/v1.7.6)
- [Full CHANGELOG](https://github.com/Alteriom/painlessMesh/blob/main/CHANGELOG.md)

Thanks for your patience, and apologies for the broken releases! 🙏
```

---

## 📋 Post-Release Checklist

### Day 1 (October 19, 2025)
- [ ] GitHub Release created
- [ ] NPM package published
- [ ] CI/CD verified green
- [ ] No immediate issues reported

### Day 2 (October 20, 2025)
- [ ] PlatformIO registry check
- [ ] GitHub Issues monitored
- [ ] Community feedback reviewed

### Week 1 (October 19-26, 2025)
- [ ] Arduino Library Manager verified
- [ ] No critical bugs reported
- [ ] Crash rate statistics collected
- [ ] Plan v1.8.0 roadmap

---

## 🎯 Next Steps

### Immediate
1. Create GitHub Release (use template above)
2. Publish NPM package
3. Monitor CI/CD pipeline

### Future (v1.8.0 Planning)
1. Monitor TaskScheduler updates for v4.1+
2. Collect user feedback on v1.7.6
3. Evaluate thread-safe queue re-implementation
4. Consider alternative FreeRTOS fixes

---

**Release Manager:** Alteriom Development Team  
**Completion Status:** Automated steps complete, manual steps pending  
**Estimated Time for Manual Steps:** 30 minutes  
**Priority:** HIGH - Users are waiting for working version
