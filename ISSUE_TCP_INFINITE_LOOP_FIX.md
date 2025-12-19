# TCP Connection Infinite Loop Fix - Summary

## Issue Description

Mesh nodes were getting stuck in an infinite connection retry loop when a bridge's WiFi AP was operational but its TCP server on port 5555 was unresponsive. This manifested as:

1. Node scans and finds bridge (WiFi working)
2. Node gets IP from bridge (WiFi association successful)
3. TCP connection attempts fail repeatedly with error -14 (ERR_CONN)
4. After 6 retry attempts (with exponential backoff: 1s, 2s, 4s, 8s, 8s), WiFi reconnection is triggered
5. Cycle repeats indefinitely because the node finds the same unresponsive bridge

### Root Cause

The code had a TCP retry mechanism with exponential backoff (added to handle timing issues), but no mechanism to track which specific nodes had failed. When all retries were exhausted, it would trigger a WiFi reconnection, which would scan for APs again. The same unresponsive bridge would be found and selected, starting the cycle over.

**Key insight**: The bridge's WiFi AP was working fine (DHCP, WiFi association all functional), but the mesh TCP server process on port 5555 was not accepting connections. This created a scenario where the node could connect to WiFi but not join the mesh.

## Solution: TCP Failure Blocklist

Implemented a temporary blocklist mechanism to track nodes where TCP connection attempts have been exhausted:

### Core Components

1. **Blocklist Storage** (`src/painlessMeshSTA.h`)
   - `std::map<uint32_t, uint32_t> tcpFailureBlocklist` - Maps nodeId to blockUntil timestamp
   - Each entry tracks when a node should become eligible for retry

2. **Blocklist Management** (`src/painlessMeshSTA.cpp`)
   - `blockNodeAfterTCPFailure(nodeId, duration)` - Add node to blocklist
   - `isNodeBlocked(nodeId)` - Check if node is currently blocked
   - `cleanupBlocklist()` - Remove expired entries automatically

3. **AP Filtering Integration** (`src/painlessMeshSTA.cpp`)
   - Modified `filterAPs()` to skip blocked nodes during AP selection
   - Cleanup is called before filtering to remove expired entries

4. **TCP Error Handler Integration** (`src/painlessmesh/tcp.hpp`)
   - After TCP retry exhaustion, decode nodeId from IP and add to blocklist
   - Block duration: 60 seconds (configurable via `TCP_FAILURE_BLOCK_DURATION_MS`)

5. **Mesh IP Helper** (`src/painlessmesh/tcp.hpp`)
   - `decodeNodeIdFromIP(ip)` - Extract nodeId from mesh IP (format: 10.x.x.1)
   - Validates full IP format to ensure it's a mesh address

## Behavior After Fix

### Normal Case (Single Bridge Fails)

1. Node scans and finds bridge A (nodeId: 3394043125)
2. Gets IP 10.252.245.x and tries TCP connection to 10.252.245.1:5555
3. TCP fails 6 times with error -14 (total ~23 seconds)
4. **NEW**: Bridge A is added to blocklist for 60 seconds
5. WiFi reconnection triggered (10 second delay)
6. Node scans again
7. **NEW**: Bridge A is filtered out (blocked)
8. Node either:
   - Connects to alternative bridge B if available
   - Waits and rescans (Bridge A becomes eligible after 60s)

### Multiple Bridges Available

- If Bridge A fails, node automatically tries Bridge B
- Provides seamless failover to working bridges
- No infinite loop even if one bridge is broken

### All Bridges Blocked (Rare)

- Node waits and rescans periodically
- After 60 seconds, oldest blocked bridge expires and becomes eligible
- Allows recovery if the TCP server issue was temporary

## Key Design Decisions

### Block Duration: 60 Seconds

- **Rationale**: Prevents rapid retry loops while allowing reasonable recovery time
- Longer than total retry sequence (23s) + reconnection delay (10s) = 33s
- Not too long to prevent recovery from temporary issues (e.g., server restart)
- Can be adjusted via `TCP_FAILURE_BLOCK_DURATION_MS` constant

### Automatic Expiration

- Blocklist entries automatically expire after configured duration
- No manual intervention needed
- Handles temporary TCP server issues gracefully

### millis() Rollover Safety

- Uses signed arithmetic to handle 49.7-day rollover
- Introduced `MILLIS_ROLLOVER_THRESHOLD` constant (2^30 = ~12 days)
- Any time difference larger than threshold is treated as expired
- Tested specifically for rollover scenarios

### IP Address Validation

- Full validation of mesh IP format: 10.(nodeId >> 8).(nodeId & 0xFF).1
- Only blocks valid mesh IPs (not router connections)
- Returns 0 (invalid) for non-mesh IPs

## Testing

### Automated Tests

Created comprehensive test suite (`test/catch/catch_tcp_blocklist.cpp`):

1. **Infinite Loop Prevention** - Validates blocklist prevents retry loops
2. **millis() Rollover Handling** - Tests time comparison across rollover
3. **IP Address Validation** - Verifies mesh IP format detection
4. **Exhaustion Delay** - Validates timing parameters
5. **Failover Scenarios** - Tests multiple bridge handling
6. **Blocklist Cleanup** - Validates expired entry removal

**Result**: All 28 assertions pass

### Integration Testing

- All existing tests continue to pass (no regressions)
- Build system validated on Linux/CMake/Ninja
- Compatible with ESP32/ESP8266 platforms (compile-time checks)

## Files Modified

1. **src/painlessMeshSTA.h**
   - Added blocklist data structure and method declarations
   - Added `MILLIS_ROLLOVER_THRESHOLD` constant

2. **src/painlessMeshSTA.cpp**
   - Implemented blocklist management functions
   - Modified `filterAPs()` to skip blocked nodes
   - Added logging for debugging

3. **src/painlessmesh/tcp.hpp**
   - Added `TCP_FAILURE_BLOCK_DURATION_MS` constant
   - Added `decodeNodeIdFromIP()` helper function
   - Modified TCP error handler to block nodes after exhaustion

4. **src/arduino/wifi.hpp**
   - Added public `blockNodeAfterTCPFailure()` method

5. **test/catch/catch_tcp_blocklist.cpp** (new)
   - Comprehensive test suite for blocklist functionality

## Backward Compatibility

- **Fully backward compatible** - No breaking changes to public API
- Existing mesh networks continue to work without modification
- Blocklist is optional and only activates on TCP failures
- No changes to mesh protocol or message formats

## Performance Impact

- **Minimal** - Only adds O(log n) map lookups during AP filtering
- Typical mesh has < 10 nodes, so overhead is negligible
- Cleanup only runs during AP scanning (infrequent)
- No impact on normal mesh operations

## Security Considerations

- **No security vulnerabilities introduced**
- Blocklist is local to each node (not shared)
- No new attack vectors created
- Validated by CodeQL security scanner

## Future Improvements

Potential enhancements (not required for this fix):

1. **Configurable block duration** - Allow user to set via API
2. **Persistent blocklist** - Survive node reboots (if needed)
3. **Metrics/monitoring** - Track how often nodes are blocked
4. **Adaptive block duration** - Increase duration for repeated failures

## Related Issues

This fix addresses the specific issue reported where:
- Bridge log shows: "Mesh connections active: NO"
- Node log shows: Repeated "error trying to connect -14" followed by WiFi reconnection
- Cycle continues indefinitely without human intervention

## Verification

To verify the fix is working:

1. Enable CONNECTION logging: `mesh.setDebugMsgTypes(ERROR | CONNECTION)`
2. Look for log messages:
   - "blockNodeAfterTCPFailure(): Blocking node X for Y ms"
   - "filterAPs(): Skipping blocked node X (TCP server unresponsive)"
   - "cleanupBlocklist(): Removing expired entry for node X"
3. Observe that node tries alternative bridges or waits for expiration

## Conclusion

This fix solves the infinite TCP connection retry loop by implementing a simple, effective blocklist mechanism. Nodes will no longer waste resources repeatedly trying to connect to bridges with unresponsive TCP servers. Instead, they will automatically fail over to working bridges or wait a reasonable time before retrying a problematic node.

The solution is minimal, well-tested, backward compatible, and addresses the root cause without introducing new complexity or security concerns.
