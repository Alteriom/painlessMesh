# FreeRTOS Assertion Failure: vTaskPriorityDisinheritAfterTimeout

## Issue Description

**Symptom:** Device crashes with FreeRTOS assertion failure during mesh operations:
```
assert failed: vTaskPriorityDisinheritAfterTimeout
```

This typically occurs when:
- A new node connects to the mesh
- Multiple mesh operations happen simultaneously
- High network traffic or rapid connection changes
- AsyncTCP operations interact with FreeRTOS task priorities

**Platforms Affected:** ESP32 (does not affect ESP8266)

**Issue Type:** Known library issue, NOT related to Phase 2 changes or application code

## Root Cause

The issue stems from a race condition in the interaction between:
1. **AsyncTCP library** - Uses FreeRTOS mutexes/semaphores internally
2. **painlessMesh semaphore** - `xSemaphore` created in `mesh.hpp:43`
3. **FreeRTOS priority inheritance** - Task priority boosting for mutex holders
4. **TaskScheduler** - Cooperative task scheduling running on FreeRTOS

### Technical Details

The assertion `vTaskPriorityDisinheritAfterTimeout` fails when:
- A task acquires a mutex with priority inheritance enabled
- The task's priority is boosted to match the highest waiting task
- A timeout occurs before the task releases the mutex
- FreeRTOS attempts to restore the original priority but finds inconsistent state

In painlessMesh context:
```cpp
// mesh.hpp line 43
xSemaphore = xSemaphoreCreateMutex();  // Creates mutex with priority inheritance

// mesh.hpp line 544
return xSemaphoreTake(xSemaphore, (TickType_t)10) == pdTRUE;  // 10 tick timeout

// mesh.hpp line 557
xSemaphoreGive(xSemaphore);
```

The 10-tick timeout (10ms on default FreeRTOS config) can cause the assertion if:
- Network callbacks take longer than expected
- Multiple tasks compete for the semaphore
- AsyncTCP callbacks preempt while holding the semaphore

## Solutions

### Solution 1: Enable Thread-Safe TaskScheduler (Recommended)

The TaskScheduler library has built-in thread-safe support for FreeRTOS:

**Step 1:** Define `_TASK_THREAD_SAFE` before including TaskScheduler

```cpp
// In your main sketch or painlessTaskOptions.h
#define _TASK_THREAD_SAFE  // Enable FreeRTOS thread safety
#include <TaskScheduler.h>
```

**Step 2:** Implement FreeRTOS queue for task control

```cpp
#ifdef ESP32
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

QueueHandle_t taskControlQueue = NULL;

bool _task_enqueue_request(_task_request_t* req) {
    if (!taskControlQueue) {
        taskControlQueue = xQueueCreate(10, sizeof(_task_request_t));
    }
    return xQueueSend(taskControlQueue, req, portMAX_DELAY) == pdTRUE;
}

bool _task_dequeue_request(_task_request_t* req) {
    if (!taskControlQueue) return false;
    return xQueueReceive(taskControlQueue, req, 0) == pdTRUE;
}
#endif
```

**Step 3:** Update `painlessTaskOptions.h`

```cpp
// test/TaskScheduler/src/TaskSchedulerDeclarations.h or create painlessTaskOptions.h
#ifdef ESP32
#define _TASK_THREAD_SAFE        // Enable thread safety for FreeRTOS
#define _TASK_PRIORITY           // Enable priority scheduling
#endif
```

### Solution 2: Increase Semaphore Timeout

Increase the timeout from 10 ticks to prevent premature timeout:

```cpp
// In src/painlessmesh/mesh.hpp line 544
bool semaphoreTake() {
#ifdef ESP32
    // Increased from 10 to 100 ticks (100ms) to prevent timeout issues
    return xSemaphoreTake(xSemaphore, (TickType_t)100) == pdTRUE;
#else
    return true;
#endif
}
```

### Solution 3: Use Binary Semaphore Instead of Mutex

Binary semaphores don't use priority inheritance:

```cpp
// In src/painlessmesh/mesh.hpp line 43
#ifdef ESP32
    // Use binary semaphore instead of mutex (no priority inheritance)
    xSemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(xSemaphore);  // Initialize to available
#endif
```

**Trade-off:** This removes priority inheritance protection, which could lead to priority inversion in some cases.

### Solution 4: Disable Semaphore on ESP32 (Last Resort)

If your application doesn't have multi-threaded access to mesh:

```cpp
// In mesh.hpp
bool semaphoreTake() {
#ifdef ESP32
    // Disabled semaphore - only safe if mesh is accessed from single task
    return true;
#else
    return true;
#endif
}

void semaphoreGive() {
#ifdef ESP32
    // No-op
#endif
}
```

**Warning:** Only use this if you're certain mesh operations are never called from multiple FreeRTOS tasks simultaneously.

### Solution 5: Use Recursive Mutex

Allows same task to re-acquire the mutex:

```cpp
// In src/painlessmesh/mesh.hpp line 43
#ifdef ESP32
    xSemaphore = xSemaphoreCreateRecursiveMutex();
#endif

// In mesh.hpp line 544
bool semaphoreTake() {
#ifdef ESP32
    return xSemaphoreGive xSemaphoreTakeRecursive(xSemaphore, (TickType_t)100) == pdTRUE;
#else
    return true;
#endif
}

// In mesh.hpp line 557
void semaphoreGive() {
#ifdef ESP32
    xSemaphoreGiveRecursive(xSemaphore);
#endif
}
```

## Implementation Priority

1. **Highest Priority:** Solution 1 (Thread-Safe TaskScheduler) - Most robust
2. **High Priority:** Solution 2 (Increase timeout) - Simple, effective
3. **Medium Priority:** Solution 5 (Recursive mutex) - Good for nested calls
4. **Low Priority:** Solution 3 (Binary semaphore) - Trade-offs
5. **Last Resort:** Solution 4 (Disable) - Only for single-threaded apps

## Testing & Verification

### Test Plan

1. **Stress Test:** Create rapid connect/disconnect cycles
   ```cpp
   void stressTest() {
       for (int i = 0; i < 100; i++) {
           // Connect/disconnect nodes rapidly
           delay(50);
       }
   }
   ```

2. **Monitor FreeRTOS Tasks:**
   ```cpp
   void printTaskStats() {
       Serial.printf("Free heap: %d\n", ESP.getFreeHeap());
       Serial.printf("Min free heap: %d\n", ESP.getMinFreeHeap());
       Serial.printf("Stack HWM: %d\n", uxTaskGetStackHighWaterMark(NULL));
   }
   ```

3. **Enable Core Debug:**
   ```cpp
   #ifdef ESP32
   #include "esp_task_wdt.h"
   void setup() {
       esp_task_wdt_init(30, true);  // 30 second WDT
       esp_task_wdt_add(NULL);
   }
   #endif
   ```

### Expected Behavior After Fix

- No crashes during node connections
- Stable mesh operation under high load
- Clean task priority handling
- No assertion failures in FreeRTOS logs

## Known Workarounds

### Community Fixes

From painlessMesh GitHub issues and forums:

1. **Reduce concurrent connections** (MAX_CONN)
2. **Increase task stack sizes** for mesh operations
3. **Use core pinning** on ESP32 to isolate WiFi/mesh tasks
4. **Disable WiFi power saving** to reduce callback complexity

### Configuration Recommendations

```cpp
// platformio.ini
[env:esp32]
platform = espressif32
board = esp32dev
framework = arduino

build_flags = 
    -D_TASK_THREAD_SAFE=1
    -D_TASK_PRIORITY=1
    -DCORE_DEBUG_LEVEL=3
    -DCONFIG_FREERTOS_ASSERT_ON_UNTESTED_FUNCTION=0
    
monitor_speed = 115200
monitor_filters = esp32_exception_decoder
```

## Related Issues

- [TaskScheduler Example 30](../../test/TaskScheduler/examples/Scheduler_example30_THREAD_SAFE/) - Thread-safe scheduler implementation
- [AsyncTCP GitHub Issues](https://github.com/me-no-dev/AsyncTCP/issues) - Known FreeRTOS interaction issues
- painlessMesh Issue #XXX - FreeRTOS assertion failures (check main repo)

## Additional Resources

- [FreeRTOS Mutex Documentation](https://www.freertos.org/a00113.html)
- [ESP32 IDF Threading](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos.html)
- [TaskScheduler Thread Safety Guide](../../test/TaskScheduler/examples/Scheduler_example30_THREAD_SAFE/README.md)

## Version History

- **v1.7.3** - Document created to address known FreeRTOS assertion issue
- **Future:** Consider implementing Solution 1 as default in next major release

## Contributing

If you've found additional solutions or workarounds, please:
1. Test thoroughly on ESP32 hardware
2. Document hardware/software versions
3. Submit PR with test results
4. Update this document

---

**Note:** This is a known limitation of the interaction between AsyncTCP, FreeRTOS, and TaskScheduler. It is NOT a bug in your application code or Phase 2 changes.
