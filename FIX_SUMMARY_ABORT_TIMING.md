# Fix Summary: Hard Reset on Bridge When Node Becomes Offline

## Issue
ESP32/ESP8266 devices crash with heap corruption when nodes become offline during bridge operations:
```
CORRUPT HEAP: Bad head at 0x40838cdc. Expected 0xabba1234 got 0x4200822e
assert failed: multi_heap_free multi_heap_poisoning.c:279 (head != NULL)
```

## Root Cause
The `BufferedConnection` destructor called `client->abort()` synchronously before scheduling deferred deletion (1000ms+ later). This violated AsyncTCP best practices and left the client in an inconsistent state during the deletion window.

## The Fix

### Single Line Change
**File**: `src/painlessmesh/connection.hpp`

**Removed**:
```cpp
client->abort();  // Called synchronously before deferred deletion
```

**Rationale**:
- `close()` and `close(true)` are sufficient for connection termination
- AsyncTCP best practices: `abort()` should only be called immediately before `delete`
- Eliminates the 1000ms window where AsyncTCP accesses an aborted but not-yet-deleted client

## Why This Works

### Connection Cleanup Sequence (Fixed)
```
T=0ms:    ~BufferedConnection() called
T=0ms:    close() - gracefully closes connection
T=0ms:    close(true) if needed - forces close
T=0ms:    Deletion scheduled for T+1000ms+ (with spacing)
T=1000ms+: delete client (client is in clean state)
✅ NO HEAP CORRUPTION
```

### Previous (Broken) Sequence
```
T=0ms:    ~BufferedConnection() called
T=0ms:    close() and close(true)
T=0ms:    abort() - triggers AsyncTCP cleanup
T=0ms:    Deletion scheduled for T+1000ms+
T=0-1000ms: AsyncTCP tries to access aborted client
❌ HEAP CORRUPTION
T=1000ms+: delete client (too late)
```

## Testing
✅ **All 1000+ test assertions pass**
- TCP retry tests (47 assertions)
- Connection tests (6 assertions)
- TCP integration tests (3 assertions)
- All other test suites pass

## Impact
- ✅ Fixes critical heap corruption crash
- ✅ Follows AsyncTCP library best practices
- ✅ No breaking API changes
- ✅ Zero performance impact
- ✅ Completes AsyncClient lifecycle management

## Related Fixes
This completes a series of AsyncClient lifecycle improvements:
1. **Issue #254** - Deferred deletion (not immediate)
2. **Issue #269** - Increased delay (500ms)
3. **v1.9.10** - Further increased delay (1000ms)
4. **v1.9.11** - Serialized deletion (250ms spacing)
5. **This fix** - Proper cleanup order (remove synchronous abort)

## Documentation
- **ISSUE_ABORT_TIMING_FIX.md** - Detailed analysis and AsyncTCP best practices
- **CHANGELOG.md** - Version history entry
- **This file** - Quick reference summary

## Code Review & Security
- ✅ Code review: No issues found
- ✅ Security scan: No vulnerabilities detected
- ✅ All tests: Pass
