# Release Notes - Version 1.8.7

**Release Date:** November 12, 2025  
**Release Type:** Patch Release  
**Focus:** Bug fixes and documentation improvements

## 🐛 Bug Fixes

### Bridge Internet Connectivity Detection

**Issue:** Bridge nodes were incorrectly reporting internet connectivity status.

**Problem:**
- Bridge connected to router would report "Internet: NO" even when the router had internet
- The `hasInternetConnection()` function was returning false positives on regular nodes
- Only checked WiFi connection status, not actual internet connectivity

**Solution:**
- Enhanced connectivity check to verify both WiFi status AND valid gateway IP
- Check now verifies `WiFi.status() == WL_CONNECTED` AND gateway IP is not 0.0.0.0
- Regular nodes without WiFi connection correctly report no internet

**Impact:**
- Bridge failover logic now works correctly
- Internet connectivity status is accurate
- Better network topology awareness

**Files Modified:**
- `src/arduino/wifi.hpp` (lines 1188-1192)
- `examples/bridge_failover/README.md` (added troubleshooting)

**Before:**
```
Bridge → Router (with Internet) → Reports: "Internet: NO" ❌
```

**After:**
```
Bridge → Router (with Internet) → Reports: "Internet: YES" ✅
```

### Version Documentation Consistency

**Issue:** Header file version comments were out of sync with library version, causing user confusion.

**Problem:**
- `painlessMesh.h` showed version 1.8.4 in header comment
- `AlteriomPainlessMesh.h` showed version 1.6.1 in version defines
- Library files showed version 1.8.6
- Users were confused about whether files had been modified since those versions

**Solution:**
- Updated `painlessMesh.h` header comment to version 1.8.7
- Updated `AlteriomPainlessMesh.h` version defines to 1.8.7
- Created comprehensive VERSION_MANAGEMENT.md documentation
- Clarified that header versions reflect library version, not file-specific versioning
- Added version management guidance to RELEASE_GUIDE.md

**Impact:**
- Clear documentation about version management
- No more confusion about header file versions
- Established best practices for version comments

**Files Modified:**
- `src/painlessMesh.h`
- `src/AlteriomPainlessMesh.h`
- `docs/VERSION_MANAGEMENT.md` (new)
- `RELEASE_GUIDE.md` (enhanced)

**Documentation Added:**
- Explains version comment purpose and meaning
- Clarifies difference between library version and file modification history
- Provides best practices for version management
- Documents the complete version update process

## 📚 Documentation Improvements

### New Documentation

1. **VERSION_MANAGEMENT.md**
   - Comprehensive guide to version management
   - Explains version comment purpose
   - Clarifies common questions
   - Documents version update workflow
   - Best practices for version comments

### Enhanced Documentation

1. **RELEASE_GUIDE.md**
   - Added reference to VERSION_MANAGEMENT.md
   - Updated release process to include header version updates
   - Clarified version management workflow

2. **bridge_failover/README.md**
   - Added troubleshooting for internet connectivity issues
   - Explained the enhanced connectivity detection

## 🔄 Migration Guide

No breaking changes. This release is fully backward compatible with version 1.8.6.

### For Users

**No action required.** Simply update to version 1.8.7:

**Arduino Library Manager:**
```
Sketch → Include Library → Manage Libraries → Search "AlteriomPainlessMesh" → Update
```

**PlatformIO:**
```ini
[env:myenv]
lib_deps = 
    alteriom/AlteriomPainlessMesh@^1.8.7
```

**NPM:**
```bash
npm update @alteriom/painlessmesh
```

### For Contributors

**When releasing future versions:**
1. Update header file version comments to match library version
2. Follow the VERSION_MANAGEMENT.md guidelines
3. Use the enhanced release process in RELEASE_GUIDE.md

## 📊 Technical Details

### Files Changed
- `src/painlessMesh.h` - Version comment updated
- `src/AlteriomPainlessMesh.h` - Version defines updated
- `src/arduino/wifi.hpp` - Internet connectivity detection fixed
- `examples/bridge_failover/README.md` - Troubleshooting added
- `docs/VERSION_MANAGEMENT.md` - New documentation
- `RELEASE_GUIDE.md` - Version management guidance added
- `CHANGELOG.md` - Release notes added
- `library.properties` - Version 1.8.7
- `library.json` - Version 1.8.7
- `package.json` - Version 1.8.7

### Testing

All existing tests pass. No new tests required as changes are:
- Documentation updates
- Bug fixes to existing functionality
- Version comment updates

### Compatibility

- ✅ **ESP32**: All variants supported
- ✅ **ESP8266**: All variants supported
- ✅ **Arduino Core 2.x**: Fully compatible
- ✅ **Arduino Core 3.x**: Fully compatible
- ✅ **PlatformIO**: All platforms supported

## 🙏 Acknowledgments

**Issue Reporter:**
- User question about version comments in painlessMesh.h

**Contributors:**
- @Alteriom - Bug fixes and documentation

## 📖 Related Resources

- **VERSION_MANAGEMENT.md** - Complete version management guide
- **RELEASE_GUIDE.md** - Release process documentation
- **CHANGELOG.md** - Complete version history
- **GitHub Issues** - [Report issues](https://github.com/Alteriom/painlessMesh/issues)

## 🔗 Links

- **GitHub Release:** https://github.com/Alteriom/painlessMesh/releases/tag/v1.8.7
- **NPM Package:** https://www.npmjs.com/package/@alteriom/painlessmesh
- **PlatformIO:** https://registry.platformio.org/libraries/alteriom/AlteriomPainlessMesh
- **Documentation:** https://alteriom.github.io/painlessMesh/

---

**Previous Release:** [v1.8.6](RELEASE_NOTES_v1.8.6.md) - November 12, 2025  
**Next Release:** TBD
