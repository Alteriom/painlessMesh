# Release Notes: AlteriomPainlessMesh v1.9.18

**Release Date:** December 21, 2025  
**Version:** 1.9.18  
**Type:** Feature Enhancement

## 🎯 Overview

Version 1.9.18 introduces an important enhancement for bridge nodes that improves internet connectivity detection and provides better diagnostic information when network issues occur.

## ✨ What's New

### Internet Connectivity Check Enhancement

Bridge nodes now perform comprehensive internet connectivity verification before attempting HTTP requests, providing early failure detection and actionable error messages.

#### Key Features

- **DNS Resolution Verification**: New `hasActualInternetAccess()` function performs DNS lookup to verify actual internet connectivity
- **Early Failure Detection**: Detects router internet issues before attempting HTTP requests (avoiding 30+ second timeouts)
- **Enhanced Diagnostics**: Clear, actionable error messages help troubleshoot connectivity problems:
  - "Gateway WiFi not connected" - ESP not associated with WiFi network
  - "Router has no internet access - check WAN connection" - WiFi connected but router has no WAN
- **Minimal Performance Impact**: ~100ms DNS check vs 30+ second HTTP timeout on failure
- **Comprehensive Testing**: New test suite with 22 assertions covering various connectivity scenarios
- **Backward Compatible**: No API changes required - existing code works unchanged

#### Technical Details

The enhancement checks both:
1. **WiFi Association**: Verifies ESP is connected to router
2. **DNS Resolution**: Confirms router has working internet connection via DNS lookup

This two-stage verification prevents wasted HTTP request attempts when:
- ESP is not connected to WiFi
- Router has no WAN/internet connection
- DNS is not functional

#### Documentation

Complete implementation details available in: `ISSUE_INTERNET_CONNECTIVITY_CHECK.md`

## 📦 Installation

### PlatformIO

```ini
[env:esp32]
lib_deps = 
    alteriom/AlteriomPainlessMesh@^1.9.18
```

### Arduino Library Manager

1. Open Arduino IDE
2. Go to Tools → Manage Libraries
3. Search for "Alteriom PainlessMesh"
4. Install version 1.9.18

### NPM

```bash
npm install @alteriom/painlessmesh@1.9.18
```

## 🔄 Upgrade Guide

### From v1.9.17

No code changes required - this is a backward compatible enhancement. Simply update your library version.

### From Earlier Versions

Review the [CHANGELOG.md](CHANGELOG.md) for cumulative changes since your current version.

## 🐛 Bug Fixes

No bug fixes in this release - purely an enhancement to internet connectivity detection.

## 📊 Performance Impact

- **Memory**: No additional memory overhead
- **CPU**: Minimal - ~100ms one-time DNS check when verifying connectivity
- **Network**: Single DNS query when checking internet access (vs. 30+ second HTTP timeout on failure)

## ✅ Testing

- All existing tests passing (1000+ assertions)
- New test suite: 22 assertions covering connectivity scenarios
- Backward compatibility verified
- No regressions introduced

## 🔐 Security

No security changes in this release.

## 📚 Documentation

- **Complete Documentation**: https://alteriom.github.io/painlessMesh/
- **API Reference**: https://alteriom.github.io/painlessMesh/#/api/doxygen
- **Examples**: https://alteriom.github.io/painlessMesh/#/tutorials/basic-examples
- **Changelog**: [CHANGELOG.md](CHANGELOG.md)

## 🙏 Acknowledgments

This enhancement improves the bridge node experience by providing better diagnostics for common connectivity issues.

## 📋 Full Changelog

See [CHANGELOG.md](CHANGELOG.md) for complete version history.

## 🚀 Next Steps

After installation:

1. Review the [Bridge Examples](examples/bridge/)
2. Check out [sendToInternet Examples](examples/sendToInternet/)
3. Read about [Bridge Failover](examples/bridge_failover/)

## 💬 Support

- **Issues**: https://github.com/Alteriom/painlessMesh/issues
- **Discussions**: https://github.com/Alteriom/painlessMesh/discussions
- **Documentation**: https://alteriom.github.io/painlessMesh/

## 📄 License

LGPL-3.0 - See [LICENSE](LICENSE) file for details.

---

**Recommended for:**
- ✅ Bridge nodes using `sendToInternet()`
- ✅ Systems requiring better connectivity diagnostics
- ✅ Production deployments where quick failure detection is important
- ✅ All users of v1.9.17 and earlier

**100% Backward Compatible** - Safe upgrade for all users.
