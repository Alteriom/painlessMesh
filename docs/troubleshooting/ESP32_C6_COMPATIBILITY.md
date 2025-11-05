# ESP32-C6 Compatibility Guide

## Overview

ESP32-C6 is a newer ESP32 variant that requires updated dependencies to work correctly with painlessMesh. This guide addresses common issues and solutions for using painlessMesh on ESP32-C6.

## Known Issue: TCP Allocation Crash

### Symptom

The device crashes on startup or during mesh initialization with the following error:

```
assert failed: tcp_alloc /IDF/components/lwip/lwip/src/core/tcp.c:1854 (Required to lock TCPIP core functionality!)
```

The device enters an endless reboot loop, preventing normal mesh operation.

### Root Cause

This issue is caused by incompatibility between:
- ESP32-C6 hardware
- Arduino ESP32 core v3.1.0 or later
- Older versions of the AsyncTCP library

The newer ESP32 Arduino core enforces stricter LWIP (Lightweight IP) thread safety requirements. Operations that modify TCP/IP data structures must now be protected with proper mutex locking. Older AsyncTCP versions (< v3.3.0) do not implement this locking, causing runtime assertions and crashes.

### Solution

#### Option 1: Update AsyncTCP Library (Recommended)

Use the latest version of AsyncTCP that includes proper LWIP locking:

**For PlatformIO:**

Update your `platformio.ini`:

```ini
[env:esp32c6]
platform = espressif32
board = esp32-c6-devkitc-1  ; or your specific board
framework = arduino

lib_deps =
    https://github.com/Alteriom/painlessMesh.git
    esp32async/AsyncTCP @ ^3.4.7  ; Use latest version with LWIP locking
    bblanchon/ArduinoJson @ ^7.4.2
    arkhipenko/TaskScheduler @ ^4.0.0
```

**For Arduino IDE:**

1. Remove any existing AsyncTCP library installation
2. Download the latest AsyncTCP from: https://github.com/ESP32Async/AsyncTCP
3. Install using "Sketch" → "Include Library" → "Add .ZIP Library"
4. **Important:** Do not use the Arduino Library Manager for AsyncTCP, as it may install an outdated version

#### Option 2: Downgrade Arduino Core (Temporary Workaround)

If you need an immediate solution and cannot update AsyncTCP:

1. In Arduino IDE: Tools → Board Manager → ESP32 by Espressif Systems
2. Install version **3.0.7** instead of 3.1.0+
3. This is not recommended long-term as you'll miss security updates and new features

#### Option 3: Use Recommended Build Flags

Add these configuration options to improve stability (PlatformIO):

```ini
build_flags =
    -D CONFIG_ASYNC_TCP_MAX_ACK_TIME=5000
    -D CONFIG_ASYNC_TCP_PRIORITY=10
    -D CONFIG_ASYNC_TCP_QUEUE_SIZE=64
    -D CONFIG_ASYNC_TCP_RUNNING_CORE=1
    -D CONFIG_ASYNC_TCP_STACK_SIZE=4096
```

These flags help maintain proper task scheduling and LWIP event handling.

## Verification

After applying the fix, verify your setup:

1. Upload a simple bridge example
2. Monitor serial output for clean startup
3. Check for successful mesh initialization without crashes
4. Verify mesh connectivity with other nodes

Example verification output:
```
setLogLevel: ERROR | STARTUP | CONNECTION |
STARTUP: init(): 1
STARTUP: stationManual() Starting WiFi connection
STARTUP: Connection established
STARTUP: IP address: 192.168.1.100
```

## Additional ESP32-C6 Considerations

### Hardware-Specific Notes

- ESP32-C6 uses RISC-V architecture (not Xtensa like older ESP32)
- Built-in WiFi 6 (802.11ax) support - but mesh uses 802.11n
- Lower power consumption compared to ESP32
- Different GPIO pinout - verify your pin assignments

### Memory Constraints

ESP32-C6 typically has:
- 320 KB SRAM
- 4 MB Flash (typical configuration)

This is adequate for painlessMesh, but be mindful of:
- Maximum node count in mesh (recommend < 20 nodes)
- Message queue sizes
- JSON message complexity

### Performance Optimization

For best results on ESP32-C6:

1. **WiFi Channel Selection**: Use channels 1, 6, or 11 for best performance
2. **Mesh Size**: Keep mesh networks under 15-20 nodes
3. **Message Frequency**: Avoid sending messages more than once per second per node
4. **Power Management**: Consider WiFi power save modes for battery operation

## Troubleshooting Checklist

If you're still experiencing issues:

- [ ] Confirmed AsyncTCP version is 3.3.0 or newer
- [ ] Removed all old AsyncTCP library installations
- [ ] Arduino ESP32 core is 3.0.7 or 3.1.0+ with updated AsyncTCP
- [ ] Verified board definition matches your hardware
- [ ] Checked serial monitor for actual error messages
- [ ] Tested with minimal example (examples/basic/basic.ino)
- [ ] Verified WiFi credentials are correct
- [ ] Confirmed router and mesh use same WiFi channel (for bridge mode)

## References

- [AsyncTCP Library (ESP32Async)](https://github.com/ESP32Async/AsyncTCP)
- [ESP32 Arduino Core Release Notes](https://github.com/espressif/arduino-esp32/releases)
- [painlessMesh GitHub Issues](https://github.com/Alteriom/painlessMesh/issues)

## Related Documentation

- [Common Issues](common-issues.md)
- [FAQ](faq.md)
- [Bridge to Internet Guide](../../BRIDGE_TO_INTERNET.md)
- [Debugging Guide](debugging.md)

---

**Last Updated:** 2025-11-05  
**Applies to:** painlessMesh v1.7.8+, ESP32-C6, Arduino ESP32 Core 3.0+
