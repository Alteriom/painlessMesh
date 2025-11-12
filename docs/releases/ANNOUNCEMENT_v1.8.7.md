# 🚀 AlteriomPainlessMesh v1.8.7 Released

**Date:** November 12, 2025

We're pleased to announce the release of **AlteriomPainlessMesh v1.8.7**, a patch release focused on bug fixes and documentation improvements.

## 🎯 What's New

### Bridge Internet Connectivity Detection Fixed

Bridge nodes now correctly detect and report internet connectivity status. The previous implementation only checked WiFi connection status, which led to incorrect reporting when bridges were connected to routers with internet access.

**Key Improvements:**
- ✅ Accurate internet status reporting for bridge nodes
- ✅ Proper gateway IP validation (not 0.0.0.0)
- ✅ Fixed false positives on regular nodes
- ✅ Better bridge failover logic

### Version Documentation Clarity

We've resolved confusion about version numbers in header files by creating comprehensive documentation and synchronizing all version references.

**What Changed:**
- ✅ All header file version comments now match library version
- ✅ New VERSION_MANAGEMENT.md document explains versioning
- ✅ Clear distinction between library version and file modification history
- ✅ Best practices for version management established

## 📚 New Documentation

### VERSION_MANAGEMENT.md

A comprehensive guide that answers:
- What do version numbers in header files mean?
- Where is the official library version defined?
- How to check if a file has been modified?
- Best practices for version comments
- Complete version update workflow

**Read it here:** [docs/VERSION_MANAGEMENT.md](https://github.com/Alteriom/painlessMesh/blob/main/docs/VERSION_MANAGEMENT.md)

## 🔧 For Developers

### Version Comment Best Practices

When releasing new versions, remember to update header file version comments:

```cpp
/**
 * @file painlessMesh.h
 * @version 1.8.7  // ← Keep this synchronized with library version
 * @date 2025-11-12
 */
```

This helps maintain documentation consistency and prevents user confusion.

## 📦 How to Update

### Arduino Library Manager
```
Sketch → Include Library → Manage Libraries → 
Search "AlteriomPainlessMesh" → Update to 1.8.7
```

### PlatformIO
```ini
[env:myenv]
lib_deps = 
    alteriom/AlteriomPainlessMesh@^1.8.7
```

### NPM
```bash
npm update @alteriom/painlessmesh
```

## ✅ Compatibility

This release is **fully backward compatible** with v1.8.6. No breaking changes.

- ✅ ESP32 (all variants)
- ✅ ESP8266 (all variants)
- ✅ Arduino Core 2.x and 3.x
- ✅ PlatformIO

## 🐛 Bug Reports

If you encounter any issues with this release, please:

1. Check the [VERSION_MANAGEMENT.md](https://github.com/Alteriom/painlessMesh/blob/main/docs/VERSION_MANAGEMENT.md) for version-related questions
2. Review the [CHANGELOG.md](https://github.com/Alteriom/painlessMesh/blob/main/CHANGELOG.md) for detailed changes
3. [Open an issue](https://github.com/Alteriom/painlessMesh/issues) with a detailed description

## 🙏 Thank You

Thanks to our users for reporting issues and helping improve the library!

Special thanks to the user who raised the question about version comments in `painlessMesh.h`, leading to better documentation clarity.

## 📖 Resources

- **Release Notes:** [RELEASE_NOTES_v1.8.7.md](RELEASE_NOTES_v1.8.7.md)
- **CHANGELOG:** [CHANGELOG.md](../../CHANGELOG.md)
- **Documentation:** https://alteriom.github.io/painlessMesh/
- **GitHub Repository:** https://github.com/Alteriom/painlessMesh
- **Issues:** https://github.com/Alteriom/painlessMesh/issues

---

**Happy Meshing!** 🌐

— The Alteriom Team
