# painlessMesh v1.7.4 & v1.7.5 Compilation Issues

> ## ✅ RESOLVED IN v1.7.6 (October 19, 2025)
>
> **The compilation issues described in this document have been fixed in v1.7.6.**
>
> **Solution:** Removed `scheduler_queue.hpp` and `scheduler_queue.cpp` files that required disabled `_TASK_THREAD_SAFE` macro.
>
> **Action Required:** Upgrade to v1.7.6 immediately if you're on v1.7.4 or v1.7.5.
>
> **Details:** See [RELEASE_SUMMARY_v1.7.6.md](../releases/RELEASE_SUMMARY_v1.7.6.md)

**Date**: October 19, 2025  
**Affected Versions**: v1.7.4, v1.7.5  
**Resolved Version**: v1.7.6  
**Working Version**: v1.7.2  
**Platform**: ESP32 (espressif32@6.8.1)  
**Framework**: Arduino  
**Project**: Alteriom Firmware

---

## Executive Summary

painlessMesh versions v1.7.4 and v1.7.5, which include FreeRTOS crash fixes, fail to compile with TaskScheduler v4.0.2 due to missing type definitions (`_task_request_t`) in the thread-safe scheduler queue implementation. The issue stems from an incompatibility between the thread-safe mode (`_TASK_THREAD_SAFE`) and the standard TaskScheduler configuration.

**Impact**: Cannot upgrade from v1.7.2 to access FreeRTOS crash fixes, leaving devices vulnerable to `vTaskPriorityDisinheritAfterTimeout` assertion failures when sensor nodes connect.

---

## Problem Description

### Symptom

Compilation fails when building with painlessMesh v1.7.4 or v1.7.5:

```
.pio/libdeps/universal-gateway/AlteriomPainlessMesh/src/painlessmesh/scheduler_queue.cpp:16:49: 
error: '_task_request_t' was not declared in this scope
     tsQueue = xQueueCreate(TS_QUEUE_LEN, sizeof(_task_request_t));
                                                 ^~~~~~~~~~~~~~~
```

### Root Cause

The `scheduler_queue.cpp` implementation expects the `_task_request_t` type to be defined by TaskScheduler when `_TASK_THREAD_SAFE` is enabled, but:

1. **TaskScheduler v4.0.2** only defines `_task_request_t` when `_TASK_THREAD_SAFE` is enabled **before** including `TaskScheduler.h`
2. **painlessMesh v1.7.4/v1.7.5** enables `_TASK_THREAD_SAFE` in `painlessTaskOptions.h` but this happens **after** TaskScheduler headers are processed
3. **Timing Issue**: The macro definition occurs too late in the compilation sequence

### What Changed in v1.7.4+

painlessMesh v1.7.4 introduced a thread-safe scheduler queue to fix FreeRTOS crashes:

**New Files**:
- `src/painlessmesh/scheduler_queue.hpp`
- `src/painlessmesh/scheduler_queue.cpp`

**Modified Files**:
- `src/painlessTaskOptions.h` - Added `#define _TASK_THREAD_SAFE` for ESP32
- `src/painlessmesh/mesh.hpp` - Added queue initialization

**Purpose**: Prevent FreeRTOS `vTaskPriorityDisinheritAfterTimeout` assertion failures by using FreeRTOS queues for thread-safe task control.

---

## Compilation Error Details

### v1.7.4 Build Attempt (Build 8022)

**Command**:
```bash
$env:BUILD_NUMBER_OVERRIDE="8022"; docker-compose run --rm alteriom-builder pio run -e universal-gateway
```

**Error Output**:
```
Compiling .pio/build/universal-gateway/lib55b/AlteriomPainlessMesh/painlessmesh/scheduler_queue.cpp.o

.pio/libdeps/universal-gateway/AlteriomPainlessMesh/src/painlessmesh/scheduler_queue.cpp: 
In function 'bool painlessmesh::scheduler::initQueue()':

.pio/libdeps/universal-gateway/AlteriomPainlessMesh/src/painlessmesh/scheduler_queue.cpp:16:49: 
error: '_task_request_t' was not declared in this scope
     tsQueue = xQueueCreate(TS_QUEUE_LEN, sizeof(_task_request_t));
                                                 ^~~~~~~~~~~~~~~

.pio/libdeps/universal-gateway/AlteriomPainlessMesh/src/painlessmesh/scheduler_queue.cpp:29:28: 
error: '_task_request_t' was not declared in this scope
 bool _task_enqueue_request(_task_request_t* req) {
                            ^~~~~~~~~~~~~~~

.pio/libdeps/universal-gateway/AlteriomPainlessMesh/src/painlessmesh/scheduler_queue.cpp:29:45: 
error: 'req' was not declared in this scope
 bool _task_enqueue_request(_task_request_t* req) {
                                             ^~~

.pio/libdeps/universal-gateway/AlteriomPainlessMesh/src/painlessmesh/scheduler_queue.cpp:56:28: 
error: '_task_request_t' was not declared in this scope
 bool _task_dequeue_request(_task_request_t* req) {
                            ^~~~~~~~~~~~~~~

.pio/libdeps/universal-gateway/AlteriomPainlessMesh/src/painlessmesh/scheduler_queue.cpp:56:45: 
error: 'req' was not declared in this scope
 bool _task_dequeue_request(_task_request_t* req) {
                                             ^~~

*** [.pio/build/universal-gateway/lib55b/AlteriomPainlessMesh/painlessmesh/scheduler_queue.cpp.o] Error 1
```

### v1.7.5 Build Attempt (Build 8024)

**Command**:
```bash
$env:BUILD_NUMBER_OVERRIDE="8024"; docker-compose run --rm alteriom-builder pio run -e universal-gateway
```

**Result**: **IDENTICAL ERRORS** to v1.7.4

**Library Version Installed**: `AlteriomPainlessMesh@1.7.5+sha.499d597`

**Conclusion**: v1.7.5 did not resolve the compilation issues present in v1.7.4.

---

## Technical Analysis

### Expected Type Definition

The `_task_request_t` type should be defined in TaskScheduler when thread-safe mode is enabled:

**Expected from TaskScheduler** (`TaskSchedulerDeclarations.h`):
```cpp
#ifdef _TASK_THREAD_SAFE
typedef struct {
    Task* task;
    unsigned long param1;
    long param2;
    TaskCallback param3;
    TaskOnEnable param4;
    TaskOnDisable param5;
    int request_type;
} _task_request_t;
#endif
```

### Actual Configuration

**painlessMesh v1.7.4/v1.7.5** (`src/painlessTaskOptions.h`):
```cpp
#define _TASK_PRIORITY      // Support for layered scheduling priority

// Thread-safe scheduler for ESP32 to prevent FreeRTOS assertion failures
#ifdef ESP32
#define _TASK_THREAD_SAFE   // Enable FreeRTOS queue-based task control
// Note: _TASK_STD_FUNCTION is incompatible with _TASK_THREAD_SAFE in TaskScheduler v4.0.x
#else
#define _TASK_STD_FUNCTION  // Support for std::function (ESP8266 ONLY)
#endif
```

### Include Order Problem

The issue is with the include order and macro visibility:

1. **Step 1**: Project includes `painlessMesh.h`
2. **Step 2**: `painlessMesh.h` includes `TaskSchedulerDeclarations.h` (before `painlessTaskOptions.h`)
3. **Step 3**: TaskScheduler processes without `_TASK_THREAD_SAFE` defined
4. **Step 4**: `_task_request_t` type is NOT defined
5. **Step 5**: Later, `scheduler_queue.cpp` tries to use undefined type

### Additional Incompatibility

painlessMesh documentation notes:
> "_TASK_STD_FUNCTION is incompatible with _TASK_THREAD_SAFE in TaskScheduler v4.0.x"

This creates a fundamental conflict:
- **ESP8266**: Uses `_TASK_STD_FUNCTION` (std::function callbacks)
- **ESP32**: Needs `_TASK_THREAD_SAFE` (FreeRTOS queue)
- **TaskScheduler v4.0.2**: Cannot support both simultaneously

---

## Build Environment Details

### Successful Build (v1.7.2)

**Version**: `AlteriomPainlessMesh@1.7.2+sha.1d0ba8d`  
**Build Number**: 8021  
**Compilation**: ✅ **SUCCESS**  
**Runtime**: ❌ Crashes with FreeRTOS assertion when sensor nodes connect

**Build Output**:
```
Library Manager: AlteriomPainlessMesh@1.7.2+sha.1d0ba8d has been installed!
[SUCCESS] Took 260.55 seconds
RAM:   [====      ]  36.2% (used 118620 bytes from 327680 bytes)
Flash: [========= ]  85.6% (used 1570349 bytes from 1835008 bytes)
```

### Failed Build (v1.7.4)

**Version**: `AlteriomPainlessMesh@1.7.4+sha.7cd66ac`  
**Build Number**: 8022  
**Compilation**: ❌ **FAILED**  
**Error**: Missing `_task_request_t` type definition

**Build Time to Failure**: ~288 seconds (4 min 48 sec)  
**Failure Point**: Compiling `scheduler_queue.cpp`

### Failed Build (v1.7.5)

**Version**: `AlteriomPainlessMesh@1.7.5+sha.499d597`  
**Build Number**: 8024  
**Compilation**: ❌ **FAILED**  
**Error**: Missing `_task_request_t` type definition (identical to v1.7.4)

**Build Time to Failure**: ~242 seconds (4 min 2 sec)  
**Failure Point**: Compiling `scheduler_queue.cpp`

### Docker Build Environment

**Platform**: Espressif 32 (6.8.1)  
**Framework**: Arduino (framework-arduinoespressif32 @ 3.20017.241212+sha.dcc1105b)  
**Toolchain**: toolchain-xtensa-esp32 @ 8.4.0+2021r2-patch5  
**Board**: ESP32-D0WD-V3 (revision 3)

**Libraries**:
- **TaskScheduler**: v4.0.2 (arkhipenko/TaskScheduler)
- **ArduinoJson**: v7.4.2
- **AsyncTCP**: v3.4.9
- **PubSubClient**: v2.8.0

---

## Attempted Workarounds

### Attempt 1: Use v1.7.3

**Theory**: v1.7.3 might be intermediate version between v1.7.2 (working) and v1.7.4 (broken)

**Configuration**:
```ini
[common_libs]
lib_deps = 
    https://github.com/Alteriom/painlessMesh.git#v1.7.3
```

**Result**: ❌ **FAILED**  
- Git tag v1.7.3 exists but resolves to SHA `86e2ccc` which is **v1.7.2**
- Library Manager shows: `AlteriomPainlessMesh@1.7.2+sha.86e2ccc`
- Conclusion: v1.7.3 tag is mislabeled

### Attempt 2: Use v1.7.5 (Latest)

**Theory**: v1.7.5 might fix the v1.7.4 compilation issues

**Configuration**:
```ini
[common_libs]
lib_deps = 
    https://github.com/Alteriom/painlessMesh.git#v1.7.5
```

**Result**: ❌ **FAILED**  
- Identical compilation errors to v1.7.4
- Same missing `_task_request_t` type
- No improvement in include order or type visibility

### Attempt 3: Uninstall and Reinstall Library

**Theory**: Cached library might be corrupt or using wrong version

**Commands**:
```bash
docker-compose run --rm alteriom-builder pio pkg uninstall --library "https://github.com/Alteriom/painlessMesh.git#v1.7.2"
docker-compose run --rm alteriom-builder pio pkg install --library "https://github.com/Alteriom/painlessMesh.git#v1.7.4"
```

**Result**: ❌ **FAILED**  
- Fresh library download still has compilation errors
- Confirms issue is in library code, not local environment

---

## Current Workaround

### Stay on v1.7.2 with Known Limitations

**Configuration** (`platformio.ini`):
```ini
[common_libs]
lib_deps = 
    https://github.com/Alteriom/painlessMesh.git#v1.7.2   ; Only version that compiles
    knolleary/PubSubClient@^2.8
```

**Trade-offs**:
- ✅ **Compiles successfully**
- ✅ **Can test MQTT buffer fixes** (for ~15 seconds before crash)
- ❌ **FreeRTOS crashes** when sensor nodes connect (~15-20 seconds uptime)
- ❌ **Cannot achieve stable mesh network**

**Crash Pattern**:
```
[GATEWAY MESH DEBUG] Current connected nodes: 1
[GATEWAY MESH DEBUG] Node IDs: 1693975713
CONNECTION: newConnectionTask(): adding 1693975713 now= 15192604

assert failed: vTaskPriorityDisinheritAfterTimeout tasks.c:5034 
(pxTCB != pxCurrentTCB[xPortGetCoreID()])

Backtrace: 0x40083831:0x3ffb1e70 0x4008ee75:0x3ffb1e90 0x4009465d:0x3ffb1eb0

E (15264) esp_core_dump_flash: Core dump flash config is corrupted!
Rebooting...
```

---

## Impact on Alteriom Project

### Development Blocked

1. **Phase 2 MQTT Commands**: Cannot fully test due to 15-second crash loop
2. **Mesh Stability**: Cannot achieve multi-node mesh networks
3. **Production Deployment**: Cannot deploy gateway firmware with sensor nodes

### Current Status

**Build 8021** (Current):
- **Firmware Version**: GW 2.3.7
- **Build Number**: 8021 (32-MSH)
- **painlessMesh**: v1.7.2
- **MQTT Buffer**: 4096 bytes (increased from 256 - **READY TO TEST**)
- **Status**: Compiles ✅, Crashes ❌

**Testing Limitations**:
- Can observe MQTT diagnostics for ~15 seconds
- Can send commands but device reboots before response
- Cannot validate MQTT buffer fix effectiveness
- Cannot test `get_config` command responses

---

## Recommended Solutions

### Option 1: Wait for painlessMesh v1.7.6 (RECOMMENDED)

**Requirements**:
- Fix `_task_request_t` type visibility issue
- Ensure `_TASK_THREAD_SAFE` macro is defined before TaskScheduler inclusion
- Test compilation with TaskScheduler v4.0.2
- Validate thread-safe queue implementation works

**Timeline**: Unknown - depends on upstream maintainers

**Risk**: Low - lets experts fix the issue properly

### Option 2: Manual Patch v1.7.5 Locally

**Steps**:
1. Clone painlessMesh v1.7.5 locally
2. Modify include order in `scheduler_queue.hpp`:
   ```cpp
   #ifdef ESP32
   
   // MUST include TaskScheduler declarations FIRST
   #define _TASK_THREAD_SAFE  // Define BEFORE including TaskScheduler
   #include <TaskSchedulerDeclarations.h>
   
   #include <freertos/FreeRTOS.h>
   #include <freertos/queue.h>
   // ... rest of includes
   ```
3. Test compilation
4. Submit pull request to painlessMesh

**Risk**: Medium - requires understanding of build system

### Option 3: Increase Semaphore Timeout in v1.7.2

**Workaround**: Apply FreeRTOS fix manually without thread-safe queue

**Implementation**:
Modify `.pio/libdeps/universal-gateway/AlteriomPainlessMesh/src/painlessmesh/mesh.hpp` line 544:

**Change from**:
```cpp
return xSemaphoreTake(xSemaphore, (TickType_t)10) == pdTRUE;
```

**Change to**:
```cpp
return xSemaphoreTake(xSemaphore, (TickType_t)100) == pdTRUE;
```

**Effectiveness**: ~80-85% reduction in crashes (according to painlessMesh docs)  
**Risk**: Medium - modifies library code directly, lost on clean builds  
**Benefit**: No compilation issues, simpler fix

---

## Files Affected

### Working Version (v1.7.2)

**Modified in Alteriom Project**:
- `platformio.ini` - Library version pinned to v1.7.2
- `src/gateway/common_functions.cpp` - Added MQTT diagnostics (Build 8021)
- `platformio.ini` - Added `-D MQTT_MAX_PACKET_SIZE=4096` (Build 8021)

### Problematic Files (v1.7.4/v1.7.5)

**painlessMesh Library**:
- `src/painlessTaskOptions.h` - Defines `_TASK_THREAD_SAFE` (too late in compilation)
- `src/painlessmesh/scheduler_queue.hpp` - Declares queue functions
- `src/painlessmesh/scheduler_queue.cpp` - **FAILS TO COMPILE** - missing type definitions
- `src/painlessmesh/mesh.hpp` - Calls `scheduler::initQueue()`

---

## Testing Evidence

### Build Timeline

| Build | Version | Status | Duration | Outcome |
|-------|---------|--------|----------|---------|
| 8020 | v1.7.2 | ✅ Success | 260s | Compiles, crashes in runtime |
| 8021 | v1.7.2 | ✅ Success | ~260s | Compiles, crashes in runtime, **has MQTT fix** |
| 8022 | v1.7.4 | ❌ Failed | 288s | Compilation error: `_task_request_t` |
| 8023 | v1.7.3 | ❌ Failed | 291s | Actually v1.7.2, same issues |
| 8024 | v1.7.5 | ❌ Failed | 242s | Compilation error: `_task_request_t` |

### Serial Output (v1.7.2 Runtime Crash)

```
=== GATEWAY NODE BOOT SEQUENCE ===
Firmware Version: GW 2.3.7
Build Info: Build 8021 (32-MSH)
Gateway Device ID: ALT-6825DD341CA4

[PHASE 1] Display initialization... ✓
[PHASE 2] Configuration loading... ✓
[PHASE 3] WiFi connection... ✓
WiFi connected! IP: 192.168.1.178

[PHASE 4] Time synchronization... ✓
[PHASE 5] MQTT connection... ✓
✅ MQTT: Connected successfully
✅ MQTT: Subscribed to gateway command topic: alteriom/gateway/ALT-6825DD341CA4/command

[PHASE 6] Mesh initialization... ✓
[PHASE 7] Sensor initialization... ✓
[PHASE 8] Display ready... ✓

Total Boot Time: 11389 ms
Ready for operation.

[INFO] Active mesh nodes: 0

[15 seconds later]

[GATEWAY MESH DEBUG] Current connected nodes: 1
[GATEWAY MESH DEBUG] Node IDs: 1693975713
CONNECTION: newConnectionTask(): adding 1693975713 now= 15192604

assert failed: vTaskPriorityDisinheritAfterTimeout tasks.c:5034 
(pxTCB != pxCurrentTCB[xPortGetCoreID()])

Backtrace: 0x40083831:0x3ffb1e70 0x4008ee75:0x3ffb1e90 0x4009465d:0x3ffb1eb0 ...

Rebooting...
```

**Crash Frequency**: 100% reproducible when sensor node connects

---

## GitHub Issue Template

### Title
`[ESP32] v1.7.4/v1.7.5 Compilation Failure: _task_request_t not declared`

### Description
```markdown
## Environment
- **Platform**: ESP32 (espressif32@6.8.1)
- **Framework**: Arduino
- **TaskScheduler Version**: v4.0.2
- **painlessMesh Versions Affected**: v1.7.4, v1.7.5
- **Working Version**: v1.7.2

## Issue
Compilation fails with "error: '_task_request_t' was not declared" when building with v1.7.4 or v1.7.5.

## Error Output
```
.pio/libdeps/.../AlteriomPainlessMesh/src/painlessmesh/scheduler_queue.cpp:16:49: 
error: '_task_request_t' was not declared in this scope
     tsQueue = xQueueCreate(TS_QUEUE_LEN, sizeof(_task_request_t));
                                                 ^~~~~~~~~~~~~~~
```

## Root Cause
The `_task_request_t` type is not visible when compiling `scheduler_queue.cpp` because:
1. `_TASK_THREAD_SAFE` is defined in `painlessTaskOptions.h`
2. TaskScheduler headers are included before this definition takes effect
3. Type definition depends on macro being set before TaskScheduler inclusion

## Proposed Fix
Move `#define _TASK_THREAD_SAFE` to occur before including TaskScheduler headers, or include `TaskSchedulerDeclarations.h` after defining the macro in `scheduler_queue.hpp`.

## Workaround
Stay on v1.7.2, which compiles successfully (but has FreeRTOS crash bug).
```

---

## References

### painlessMesh Documentation
- v1.7.4 Release Notes: `docs/releases/PATCH_v1.7.4.md`
- FreeRTOS Fix Implementation: `docs/troubleshooting/FREERTOS_FIX_IMPLEMENTATION.md`
- Quick Fix Guide: `docs/troubleshooting/QUICK_FIX_FREERTOS.md`

### Related Commits
- **7391717**: "fix: Implement thread-safe scheduler for ESP32 FreeRTOS" (v1.7.4)
- **65afb16**: "fix: Increase semaphore timeout from 10 to 100 ticks" (v1.7.4)
- **499d597**: v1.7.5 release commit

### External Links
- [TaskScheduler GitHub](https://github.com/arkhipenko/TaskScheduler)
- [painlessMesh Repository](https://github.com/Alteriom/painlessMesh)
- [FreeRTOS Documentation](https://www.freertos.org/a00113.html)

---

## Document Status

**Last Updated**: October 19, 2025  
**Author**: Alteriom Development Team  
**Status**: Active Issue  
**Next Review**: When painlessMesh v1.7.6 is released

**Keywords**: painlessMesh, TaskScheduler, FreeRTOS, ESP32, compilation error, _task_request_t, thread-safe, v1.7.4, v1.7.5
