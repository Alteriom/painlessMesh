# AlteriomPainlessMesh v1.8.7

**Release Date:** November 12, 2025  
**Type:** Patch Release - Bug Fixes & Documentation

## 🐛 Bug Fixes

### Bridge Internet Connectivity Detection

Fixed incorrect internet status reporting in bridge nodes:
- **Before:** Bridge → Router (with Internet) → Reports "Internet: NO" ❌
- **After:** Bridge → Router (with Internet) → Reports "Internet: YES" ✅

Bridge nodes now properly check for actual internet connectivity by verifying both WiFi connection status AND valid gateway IP (not 0.0.0.0).

**Impact:**
- ✅ Accurate internet status for bridge nodes
- ✅ Correct bridge failover behavior
- ✅ No more false positives on regular nodes

**Files changed:** `src/arduino/wifi.hpp`

### Version Documentation Consistency

Fixed confusing version numbers in header files:
- Updated `painlessMesh.h` version comment: 1.8.4 → 1.8.7
- Updated `AlteriomPainlessMesh.h` version defines: 1.6.1 → 1.8.7
- Created comprehensive VERSION_MANAGEMENT.md documentation

**Clarification:** Header file version comments indicate the library version when documentation was reviewed, not file-specific versioning. Use `library.properties` for the official version.

## 📚 New Documentation

### VERSION_MANAGEMENT.md

New comprehensive guide explaining:
- ✅ What version numbers in header files mean
- ✅ Where to find the official library version
- ✅ How to track file modification history
- ✅ Version update workflow and best practices

**Essential reading for contributors!**

## 📦 Installation

### Arduino Library Manager
```
Sketch → Include Library → Manage Libraries → 
Search "AlteriomPainlessMesh" → Update to 1.8.7
```

### PlatformIO
```ini
lib_deps = alteriom/AlteriomPainlessMesh@^1.8.7
```

### NPM
```bash
npm install @alteriom/painlessmesh@1.8.7
```

## 🔄 Upgrade Notes

**Fully backward compatible** with v1.8.6. No breaking changes.

## 📖 Documentation

- **Release Notes:** [docs/releases/RELEASE_NOTES_v1.8.7.md](docs/releases/RELEASE_NOTES_v1.8.7.md)
- **Version Management Guide:** [docs/VERSION_MANAGEMENT.md](docs/VERSION_MANAGEMENT.md)
- **Full Changelog:** [CHANGELOG.md](CHANGELOG.md)
- **Online Documentation:** https://alteriom.github.io/painlessMesh/

## 🔗 Links

- **NPM Package:** https://www.npmjs.com/package/@alteriom/painlessmesh
- **PlatformIO Registry:** https://registry.platformio.org/libraries/alteriom/AlteriomPainlessMesh
- **Report Issues:** https://github.com/Alteriom/painlessMesh/issues

## ✅ Checksums

**Library Package:** `painlessMesh-v1.8.7.zip`
- Will be generated automatically by GitHub Actions

## 🙏 Credits

Thanks to our users for reporting issues and helping improve the library!

---

**Full Changelog:** https://github.com/Alteriom/painlessMesh/compare/v1.8.6...v1.8.7
