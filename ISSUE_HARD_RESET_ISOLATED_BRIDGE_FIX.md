# Hard Reset Crash in Isolated Bridge Promotion - Root Cause Analysis and Fix

## Issue Summary

**Symptom:** Hard reset crash (Guru Meditation Error: Load access fault) occurring immediately after `bridgeRoleChangedCallback` returns in `attemptIsolatedBridgePromotion()`.

**Location:** `src/arduino/wifi.hpp` line 1943 (approx.) in `attemptIsolatedBridgePromotion()` method

**Crash Details:**
```
20:46:14.079 -> ✓ Isolated bridge promotion complete on channel 10
20:46:14.079 -> 🎯 PROMOTED TO BRIDGE: Isolated node promoted to bridge
20:46:14.079 -> This node is now the primary bridge!
20:46:14.079 -> Guru Meditation Error: Core  0 panic'ed (Load access fault). Exception was unhandled.
MEPC: 0x42013754  RA: 0x42013750
A4: 0xbaad5678 (freed memory marker)
MTVAL: 0xbaad59d4 (A4 + 0x35C offset - trying to access freed memory)
```

The `0xbaad5678` marker indicates attempting to access memory that has been freed.

## Root Cause

The crash was caused by attempting to schedule a new task (`addTask`) immediately after a `stop()/initAsBridge()` cycle within the same function execution context.

### Sequence of Events Leading to Crash:

1. **Initial State:** A periodic isolated bridge retry task is executing (scheduled at mesh init, line 233-310 in wifi.hpp)

2. **Stop Called:** Inside `attemptIsolatedBridgePromotion()`, `this->stop()` is called (line 1901)
   - `stop()` calls `painlessmesh::Mesh<Connection>::stop()` which:
     - Closes all connections
     - Calls `plugin::PackageHandler<T>::stop()` which **disables all tasks and clears the task list**
     - If scheduler is not external, deletes the scheduler (but in this case it is external, so it's preserved)

3. **Reinit Called:** `initAsBridge()` is called (line 1905-1907)
   - Creates new mesh state
   - Reinitializes scheduler with external scheduler reference
   - Calls `initBridgeStatusBroadcast()` which sets up periodic status broadcasts

4. **Success Path:** Bridge initialization succeeds
   - Callback is invoked successfully (line 1943-1945) - **this works fine**
   - Attempts to schedule new task via `this->addTask()` (line 1952) - **CRASHES HERE**

5. **Crash Occurs:** When `addTask` is called, it accesses internal mesh state or scheduler state that is in an inconsistent state immediately after the stop/reinit cycle, causing an access fault on freed memory.

### Why the Callback Succeeded But addTask Crashed

The callback execution was successful because it only performed console output and didn't access mesh internal state. However, `addTask()` needs to:
- Access `mScheduler` pointer
- Add task to `taskList` (managed by PackageHandler)
- Access other internal mesh structures

Immediately after `stop()` has cleared tasks and connections, and `initAsBridge()` has only partially reconstructed state, these structures may not be fully stable.

## The Fix

**Solution:** Remove the redundant task scheduling since `initBridgeStatusBroadcast()` (called by `initAsBridge()`) already handles bridge status announcements.

### Code Changes

**Before (Problematic):**
```cpp
// Notify via callback
if (bridgeRoleChangedCallback) {
  bridgeRoleChangedCallback(true, TSTRING("Isolated node promoted to bridge"));
}

// Send bridge status announcement to attract other nodes
this->addTask(3000, TASK_ONCE, [this]() {
  Log(STARTUP, "Sending bridge status announcement on channel %d\n",
      _meshChannel);
  this->sendBridgeStatus();
});
```

**After (Fixed):**
```cpp
// Notify via callback
if (bridgeRoleChangedCallback) {
  TSTRING reason = "Isolated node promoted to bridge";
  bridgeRoleChangedCallback(true, reason);
}

// Note: Bridge status announcement will be sent automatically by
// initBridgeStatusBroadcast() which is called by initAsBridge().
// The immediate broadcast is scheduled there at line 1277-1280, so we don't
// need to schedule another one here. This avoids potential crashes from
// scheduling tasks immediately after stop()/reinit cycle.
Log(STARTUP,
    "Bridge status announcement will be sent by bridge status broadcast "
    "system\n");
```

### Why This Fix Works

1. **Eliminates Unsafe addTask Call:** No longer calls `addTask()` immediately after `stop()/initAsBridge()` cycle

2. **Relies on Existing Infrastructure:** The `initBridgeStatusBroadcast()` function (lines 1239-1344) already:
   - Schedules immediate bridge status broadcast (line 1277-1280)
   - Sets up periodic broadcasts (line 1272-1273)
   - Handles status updates when nodes connect (line 1286-1340)

3. **Maintains Functionality:** Bridge nodes still announce their status to attract other nodes, just via the existing broadcast system rather than an ad-hoc scheduled task

4. **Safer Execution Context:** The broadcast system's tasks are scheduled during `initAsBridge()` when the mesh state is being properly reconstructed, not from within a task that existed in the pre-stop state

## Related Code Sections

- **initBridgeStatusBroadcast():** Lines 1239-1344 in wifi.hpp
  - Line 1277-1280: Immediate broadcast after bridge initialization
  - Line 1272-1273: Periodic broadcast task setup
  
- **initAsBridge():** Lines 383-519 in wifi.hpp
  - Line 496: Calls `initBridgeStatusBroadcast()`
  - Line 499: Calls `initGatewayInternetHandler()`

- **attemptIsolatedBridgePromotion():** Lines 1850-1960 in wifi.hpp
  - Line 1901: Calls `this->stop()`
  - Line 1905: Calls `this->initAsBridge()`

## Testing Recommendations

1. **Isolated Node Scenario:** Test a single node with router credentials that successfully promotes to bridge
2. **Bridge Failover:** Test existing bridge disconnecting and node promotion
3. **Multiple Retry Attempts:** Test the retry mechanism over multiple cycles
4. **Status Broadcast Reception:** Verify that other nodes successfully receive and process bridge status announcements
5. **Callback Execution:** Ensure `bridgeRoleChangedCallback` executes correctly without crashes

## Prevention Guidelines

To prevent similar issues in the future:

1. **Avoid Scheduling Tasks After stop():** Never call `addTask()` immediately after `stop()` within the same execution context
2. **Trust Existing Infrastructure:** Check if functionality already exists before adding ad-hoc task scheduling
3. **Document Lifecycle Dependencies:** Methods like `stop()` and `init()` should clearly document what state they leave the object in
4. **Use Deferred Execution:** If tasks must be scheduled after reinit, consider using the main loop or existing periodic tasks
5. **Test Stop/Reinit Cycles:** Any code path that calls `stop()` followed by `init*()` should be thoroughly tested

## Version Information

- **Issue First Reported:** v1.9.10 (supposedly fixed by changing callback signature to const reference)
- **Actual Root Cause:** Task scheduling after stop/reinit cycle
- **Fix Applied:** v1.9.11 (pending)

## Additional Notes

The CHANGELOG mentioned this was fixed in v1.9.10 by changing the callback signature from pass-by-value to `const TSTRING&`. While that change was correct for other reasons (avoiding unnecessary string copies), it did not address the root cause of this crash.

The actual issue was architectural: attempting to schedule tasks on a mesh object that had just undergone a stop/reinit cycle while still executing within a task from the pre-stop state. This creates a complex lifecycle scenario where internal state consistency cannot be guaranteed.

The fix properly delegates responsibility to the existing bridge status broadcast infrastructure, which is designed to handle initialization-time task scheduling safely.
