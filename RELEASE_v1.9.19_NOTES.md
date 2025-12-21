# Release Notes: AlteriomPainlessMesh v1.9.19

**Release Date:** December 21, 2025  
**Version:** 1.9.19  
**Type:** Bug Fix

## 🎯 Overview

Version 1.9.19 fixes a critical issue where infrastructure errors (like router internet connectivity problems) were incorrectly treated as retryable, causing unnecessary delays, battery drain, and resource waste.

## 🐛 Bug Fixes

### Gateway Connectivity Error Non-Retryable Fix

Fixed an issue where gateway infrastructure errors were incorrectly retried, wasting time and resources on problems that require user intervention.

#### Problem Fixed

When nodes attempted `sendToInternet()` but the gateway had infrastructure issues, the system would:
- ❌ Retry indefinitely (up to max retries)
- ❌ Waste ~14+ seconds before final failure
- ❌ Drain battery with futile retry attempts
- ❌ Provide unclear feedback about non-transient problems

#### The Solution

The fix distinguishes between two types of errors:

**Non-Retryable (Infrastructure Issues):**
- ❌ "Router has no internet access - check WAN connection"
- ❌ "Gateway WiFi not connected"
- ❌ HTTP 4xx client errors (except 429 rate limiting)
- ❌ HTTP 3xx redirects

**Retryable (Transient Issues):**
- ✅ HTTP 203 - Cached/proxied response
- ✅ HTTP 5xx - Server errors
- ✅ HTTP 429 - Rate limiting
- ✅ HTTP 0 with generic network errors

#### Key Benefits

- **Immediate Failure Feedback**: No wasted retry delays for infrastructure issues
- **Resource Efficiency**: Saves battery and network resources
- **Clear Error Messages**: Users can quickly identify and fix infrastructure problems
- **Smart Retry Logic**: Transient errors still retry with exponential backoff
- **Comprehensive Testing**: New test suite with 25 assertions
- **100% Backward Compatible**: No API changes required

#### Technical Details

Modified `handleGatewayAck()` in `src/painlessmesh/mesh.hpp` to detect gateway connectivity errors by examining error message content:

```cpp
// Check if this is a gateway-level connectivity error (non-retryable)
if (ack.error.find("Router has no internet") != TSTRING::npos ||
    ack.error.find("Gateway WiFi not connected") != TSTRING::npos) {
  isGatewayConnectivityError = true;
  // Not marked as retryable
}
```

#### Documentation

Complete implementation details available in: `ISSUE_GATEWAY_CONNECTIVITY_NON_RETRYABLE_FIX.md`

## 📦 Installation

### PlatformIO

```ini
[env:esp32]
lib_deps = 
    alteriom/AlteriomPainlessMesh@^1.9.19
```

### Arduino Library Manager

1. Open Arduino IDE
2. Go to Tools → Manage Libraries
3. Search for "Alteriom PainlessMesh"
4. Install version 1.9.19

### NPM

```bash
npm install @alteriom/painlessmesh@1.9.19
```

## 🔄 Upgrade Guide

### From v1.9.18

No code changes required - this is a backward compatible bug fix. Simply update your library version to benefit from improved error handling.

### From Earlier Versions

Review the [CHANGELOG.md](CHANGELOG.md) for cumulative changes since your current version.

## 📊 Performance Impact

- **Memory**: No additional memory overhead
- **CPU**: Negligible - simple string comparison for error detection
- **Network**: Significantly improved - eliminates ~14+ seconds of wasted retry delays
- **Battery**: Improved - no futile retry attempts for infrastructure errors

## ✅ Testing

- All existing tests passing (1000+ assertions)
- New test suite: 25 assertions covering retry logic scenarios
- Backward compatibility verified
- No regressions introduced

Test results:
```
catch_internet_connectivity_check: 25 assertions passed
catch_http_status_codes: 50 assertions passed
catch_send_to_internet: 184 assertions passed
Total: 259 assertions across 23 test cases - ALL PASSING
```

## 🔐 Security

No security changes in this release.

## 📚 Documentation

- **Complete Documentation**: https://alteriom.github.io/painlessMesh/
- **API Reference**: https://alteriom.github.io/painlessMesh/#/api/doxygen
- **Examples**: https://alteriom.github.io/painlessMesh/#/tutorials/basic-examples
- **Changelog**: [CHANGELOG.md](CHANGELOG.md)

## 🙏 Acknowledgments

This fix improves the mesh network experience by providing intelligent retry logic that distinguishes between infrastructure issues and transient errors.

## 📋 Full Changelog

See [CHANGELOG.md](CHANGELOG.md) for complete version history.

## 🚀 Next Steps

After installation:

1. Review the [Bridge Examples](examples/bridge/)
2. Check out [sendToInternet Examples](examples/sendToInternet/)
3. Read about [Bridge Failover](examples/bridge_failover/)
4. Understand [Internet Connectivity Checks](ISSUE_INTERNET_CONNECTIVITY_CHECK.md)

## 💬 Support

- **Issues**: https://github.com/Alteriom/painlessMesh/issues
- **Discussions**: https://github.com/Alteriom/painlessMesh/discussions
- **Documentation**: https://alteriom.github.io/painlessMesh/

## 📄 License

LGPL-3.0 - See [LICENSE](LICENSE) file for details.

---

**Highly Recommended for:**
- ✅ Bridge nodes using `sendToInternet()`
- ✅ Battery-powered devices (reduces wasted power)
- ✅ Production deployments with intermittent router connectivity
- ✅ Systems where quick failure detection is critical
- ✅ All users of v1.9.18 and earlier

**100% Backward Compatible** - Safe upgrade for all users. No API changes required.

---

## Behavior Comparison

### Before v1.9.19

```
sendToInternet() → Gateway error "Router has no internet"
  ↓
Retry 1 (2s delay)  → Same error
Retry 2 (4s delay)  → Same error  
Retry 3 (8s delay)  → Same error
  ↓
Total time wasted: ~14 seconds
Battery wasted: Multiple radio transmissions
User feedback: Delayed and unclear
```

### After v1.9.19

```
sendToInternet() → Gateway error "Router has no internet"
  ↓
Immediate failure recognition (non-retryable)
  ↓
Total time: < 1 second
Battery saved: No futile retries
User feedback: Immediate and actionable
```

## Related Enhancements

This release builds on recent connectivity improvements:

- **v1.9.18**: Internet connectivity check with DNS resolution
- **v1.9.19**: Smart retry logic for infrastructure errors (this release)

Together, these provide robust connectivity handling for production mesh networks.
