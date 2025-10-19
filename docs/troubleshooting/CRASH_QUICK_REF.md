# 🚨 MESH CONNECTION CRASH - QUICK REFERENCE

**Issue:** `assert failed: vTaskPriorityDisinheritAfterTimeout`  
**When:** Sensor nodes connect to mesh  
**Platform:** ESP32 only  
**Cause:** painlessMesh library (NOT Build 8015)

---

## ⚡ FASTEST FIX (5 min)

**File:** `src/painlessmesh/mesh.hpp` **Line 544**

```cpp
// CHANGE THIS:
return xSemaphoreTake(xSemaphore, (TickType_t)10) == pdTRUE;

// TO THIS:
return xSemaphoreTake(xSemaphore, (TickType_t)100) == pdTRUE;
```

**Then:** `pio run -t upload`

---

## 🛡️ PRODUCTION FIX (10 min)

**1. Create:** `src/painlessTaskOptions.h`
```cpp
#ifndef _PAINLESS_TASK_OPTIONS_H_
#define _PAINLESS_TASK_OPTIONS_H_

#ifdef ESP32
#define _TASK_THREAD_SAFE
#define _TASK_PRIORITY
#endif

#endif
```

**2. In your sketch BEFORE** `#include <painlessMesh.h>`:
```cpp
#include "painlessTaskOptions.h"
```

**3. Rebuild:** `pio run -t upload`

---

## 📊 MONITORING CODE

Add to `setup()`:
```cpp
mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);

mesh.onNewConnection([](uint32_t nodeId) {
    Serial.printf("Node %u: Heap=%d Stack=%d\n", 
                 nodeId, ESP.getFreeHeap(), 
                 uxTaskGetStackHighWaterMark(NULL));
});
```

---

## ✅ TEST CHECKLIST

- [ ] Single node connects without crash
- [ ] 5 nodes connect simultaneously
- [ ] Rapid connect/disconnect cycles (10x)
- [ ] 1+ hour sustained operation
- [ ] Heap remains stable (check every 30 sec)

---

## 📚 FULL DOCS

- `docs/troubleshooting/SENSOR_NODE_CONNECTION_CRASH.md` - Action plan
- `docs/troubleshooting/QUICK_FIX_FREERTOS.md` - Emergency fixes
- `docs/troubleshooting/FREERTOS_ASSERTION_FAILURE.md` - Technical details

---

## 🎯 KEY POINTS

✅ **Apply fix BEFORE testing Build 8015**  
✅ **This is separate from Phase 2 changes**  
✅ **Known library issue - not your code**  
✅ **Choose Option A for speed, Option B for production**

---

**Last Updated:** 2025-10-19  
**Status:** 🟡 Workaround available, library fix pending
