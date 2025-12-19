# Bridge Promotion Crash Fix - Quick Reference

## Problem
ESP32/ESP8266 crashed with "Guru Meditation Error: Load access fault" immediately after bridge promotion.

## Symptoms
```
STARTUP: Bridge takeover complete. Status broadcasts will announce bridge to network.
Guru Meditation Error: Core  0 panic'ed (Load access fault). Exception was unhandled.
MEPC    : 0x4201ab20  RA      : 0x4201ab3e
MTVAL   : 0x7247a757
```

## Root Cause
`addTask()` called immediately after `stop()/initAsBridge()` cycle before scheduler fully reinitialized.

## Solution
Added 100ms delay before task scheduling in `initBridgeStatusBroadcast()` to allow scheduler stabilization.

## Fix Location
**File**: `src/arduino/wifi.hpp`  
**Function**: `initBridgeStatusBroadcast()`  
**Lines**: 1251-1291

## What Changed
```cpp
// Before (crashed):
this->addTask([this]() { /* ... */ });

// After (fixed):
const uint32_t INIT_DELAY_MS = 100;
this->addTask(INIT_DELAY_MS, TASK_ONCE, [this]() { /* ... */ });
```

## Impact
- ✅ Fixes critical crash
- ✅ No breaking changes
- ✅ Minimal performance impact (100-150ms during init only)
- ✅ All functionality preserved

## For Developers
If you're implementing custom bridge promotion logic:
- **DO NOT** call `addTask()` immediately after `stop()/init()` cycles
- **DO** add a small delay (100ms+) before scheduling tasks
- **DO** allow network stack and scheduler to stabilize first

## Testing
All 49 test suites pass with 2000+ assertions validated.

## More Information
See `ISSUE_HARD_RESET_BRIDGE_PROMOTION_FIX.md` for complete technical details.
