# Sensor Node Connection Crash - Action Plan

## ✅ STATUS: FIXED (October 19, 2025)

**Fix Implemented:** Commit [7391717](https://github.com/Alteriom/painlessMesh/commit/7391717)  
**Released:** painlessMesh v1.7.4  
**Implementation:** See [FREERTOS_FIX_IMPLEMENTATION.md](FREERTOS_FIX_IMPLEMENTATION.md)

The complete dual-approach fix (Option A + Option B) is now available in v1.7.4. Update to the latest version to automatically apply the fix when building for ESP32 platforms.

**Installation:**

- **PlatformIO:** `pio pkg update sparck75/AlteriomPainlessMesh`
- **NPM:** `npm install @alteriom/painlessmesh@1.7.4`
- **Arduino IDE:** Library Manager → Search "painlessMesh" → Update to 1.7.4
- **GitHub:** [Release v1.7.4](https://github.com/Alteriom/painlessMesh/releases/tag/v1.7.4)

---

## Issue Summary

**Symptom:** FreeRTOS assertion failure when sensor nodes connect to mesh  
**Error:** `assert failed: vTaskPriorityDisinheritAfterTimeout`  
**Platform:** ESP32 only (ESP8266 unaffected)  
**Scope:** painlessMesh library issue - NOT related to Build 8015 or Phase 2 changes  

**Important:** This is a known painlessMesh + AsyncTCP + FreeRTOS interaction issue that requires mesh library fixes, separate from your application code.

## Immediate Action Plan

### Step 1: Apply Quick Fix (Choose ONE)

#### Option A: Increase Semaphore Timeout ⭐ RECOMMENDED FOR TESTING

**Fastest fix - 5 minutes:**

1. **Edit:** `src/painlessmesh/mesh.hpp` line 544
2. **Change:** `(TickType_t)10` → `(TickType_t)100`
3. **Rebuild & Upload:** `pio run -t upload`
4. **Test:** Connect sensor nodes and monitor for crashes

**Rationale:** Gives more time for semaphore acquisition, prevents timeout-related assertions.

#### Option B: Enable Thread-Safe Scheduler ⭐ RECOMMENDED FOR PRODUCTION

**Better long-term solution - 10 minutes:**

1. **Create:** `src/painlessTaskOptions.h` (if doesn't exist)
   ```cpp
   #ifndef _PAINLESS_TASK_OPTIONS_H_
   #define _PAINLESS_TASK_OPTIONS_H_
   
   #ifdef ESP32
   #define _TASK_THREAD_SAFE    // FreeRTOS queue-based task control
   #define _TASK_PRIORITY       // Priority scheduling support
   #endif
   
   #endif
   ```

2. **Modify:** Your main sketch BEFORE `#include <painlessMesh.h>`:
   ```cpp
   #include "painlessTaskOptions.h"
   #include <painlessMesh.h>
   ```

3. **Rebuild & Upload**

**Rationale:** Uses FreeRTOS queue for thread-safe task management, eliminates race conditions.

### Step 2: Monitor & Verify

Add this monitoring code to track connection health:

```cpp
void setup() {
    Serial.begin(115200);
    
    // Enable mesh debugging
    mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
    
    // Connection monitoring
    mesh.onNewConnection([](uint32_t nodeId) {
        Serial.printf("[MONITOR] Node connected: %u\n", nodeId);
        Serial.printf("  Free heap: %d bytes\n", ESP.getFreeHeap());
        Serial.printf("  Min heap: %d bytes\n", ESP.getMinFreeHeap());
        Serial.printf("  Stack HWM: %d bytes\n", uxTaskGetStackHighWaterMark(NULL));
        Serial.printf("  Total nodes: %d\n", mesh.getNodeList().size() + 1);
    });
    
    mesh.onDroppedConnection([](uint32_t nodeId, bool isStation) {
        Serial.printf("[MONITOR] Node dropped: %u (station=%d)\n", nodeId, isStation);
    });
}

void loop() {
    mesh.update();
    
    // Periodic health check every 30 seconds
    static uint32_t lastCheck = 0;
    if (millis() - lastCheck > 30000) {
        lastCheck = millis();
        Serial.printf("[HEALTH] Heap: %d, Min: %d, Stack: %d, Nodes: %d\n",
                     ESP.getFreeHeap(),
                     ESP.getMinFreeHeap(),
                     uxTaskGetStackHighWaterMark(NULL),
                     mesh.getNodeList().size() + 1);
    }
}
```

### Step 3: Test Scenarios

**Critical Test Cases:**

1. **Single Node Connection**
   - Start gateway
   - Connect 1 sensor node
   - Verify: No crash, stable connection

2. **Multiple Simultaneous Connections**
   - Start gateway
   - Power on 3-5 sensor nodes simultaneously
   - Verify: All connect without crashes

3. **Rapid Connect/Disconnect**
   - Connect sensor node
   - Wait 10 seconds
   - Power cycle sensor
   - Repeat 10 times
   - Verify: No cumulative issues or crashes

4. **Sustained Operation**
   - Run mesh network for 1+ hour
   - Monitor free heap and stack
   - Verify: No memory leaks or gradual degradation

### Step 4: Collect Diagnostic Data

If crashes persist after applying fixes, collect:

```cpp
// Enable detailed FreeRTOS logging
#define CORE_DEBUG_LEVEL 5  // Verbose

// In platformio.ini
build_flags = 
    -DCORE_DEBUG_LEVEL=5
    -DCONFIG_FREERTOS_ASSERT_FAIL_ABORT=0  // Don't abort on assert
    -DCONFIG_FREERTOS_WATCHPOINT_END_OF_STACK=1
```

**Capture:**
- Full serial output during crash
- Heap/stack watermarks before crash
- Number of connected nodes at crash time
- WiFi RSSI values
- Mesh topology (node IDs and connections)

## Root Cause Analysis

**Why This Happens:**

1. **AsyncTCP** uses FreeRTOS mutexes internally for WiFi callbacks
2. **painlessMesh** uses its own mutex (`xSemaphore`) for mesh operations
3. **TaskScheduler** runs cooperative multitasking on top of FreeRTOS
4. When sensor connects:
   - WiFi callback fires (high priority FreeRTOS task)
   - Mesh code tries to acquire semaphore
   - 10ms timeout too short for WiFi operations
   - FreeRTOS tries to restore task priority after timeout
   - Priority inheritance state is inconsistent → ASSERTION FAILURE

**Key Insight:** This is a timing/synchronization issue in the mesh library itself, not your sensor/gateway code.

## Long-Term Solutions (For Next Release)

These require mesh library changes and should be tracked separately from Build 8015:

### 1. Implement Thread-Safe TaskScheduler (HIGH PRIORITY)

**Status:** ⚠️ Requires painlessMesh library modification  
**Effort:** 2-4 hours  
**Impact:** Eliminates root cause

**Implementation:**
- Enable `_TASK_THREAD_SAFE` by default for ESP32 builds
- Implement FreeRTOS queue for task control requests
- Add proper mutex guards around critical sections

**Files to modify:**
- `src/painlessmesh/configuration.hpp` - Add `#define _TASK_THREAD_SAFE` for ESP32
- `src/painlessmesh/mesh.hpp` - Update semaphore timeout to 100ms
- Test with full suite

### 2. Replace Mutex with Binary Semaphore (MEDIUM PRIORITY)

**Status:** ⚠️ Alternative approach  
**Effort:** 1 hour  
**Impact:** Reduces priority inheritance issues

**Trade-off:** Loses priority inheritance protection (acceptable for mesh use case)

### 3. Add Connection Rate Limiting (LOW PRIORITY)

**Status:** Workaround  
**Effort:** 30 minutes  
**Impact:** Prevents simultaneous connection storm

```cpp
// In mesh.hpp - connection handling
static uint32_t lastConnectionTime = 0;
if (millis() - lastConnectionTime < 1000) {
    delay(100);  // Rate limit connections
}
lastConnectionTime = millis();
```

## Integration with Build 8015

**Important Separation:**

| Aspect | Build 8015 (Phase 2) | Mesh Connection Crash |
|--------|---------------------|----------------------|
| **Scope** | MQTT command bridge, topology reporting | painlessMesh + FreeRTOS timing |
| **Root Cause** | Application code | Library interaction issue |
| **Testing** | Functional tests for commands | Stress tests for connections |
| **Priority** | HIGH (feature delivery) | HIGH (stability) |
| **Timeline** | Current sprint | Parallel investigation |
| **Dependencies** | None on mesh fix | None on Build 8015 |

**Recommendation:** Apply Quick Fix (Option A or B) immediately to unblock Build 8015 testing. Schedule library fix as separate work item.

## Success Criteria

After applying fixes, you should observe:

✅ **Zero crashes** during sensor node connections  
✅ **Stable memory** - no heap degradation over time  
✅ **Fast connections** - nodes join within 5-10 seconds  
✅ **No disconnects** - sustained connections for hours  
✅ **Scales reliably** - supports 4-10 nodes depending on ESP32 model

## Related Documentation

- [FREERTOS_ASSERTION_FAILURE.md](./FREERTOS_ASSERTION_FAILURE.md) - Complete technical analysis
- [QUICK_FIX_FREERTOS.md](./QUICK_FIX_FREERTOS.md) - Emergency fixes reference
- [TaskScheduler Thread Safety Example](../../test/TaskScheduler/examples/Scheduler_example30_THREAD_SAFE/) - Reference implementation

## Decision Log

**Date:** 2025-10-19  
**Decision:** Apply Option A (timeout increase) for immediate testing  
**Rationale:** Fastest unblock for Build 8015 validation  
**Next Step:** Evaluate Option B for production deployment  
**Owner:** [Your Name]  
**Status:** 🟡 In Progress

---

**Last Updated:** 2025-10-19  
**Severity:** HIGH (blocks production deployment)  
**Workaround Available:** YES (Option A/B)  
**Permanent Fix Required:** YES (library modification)
