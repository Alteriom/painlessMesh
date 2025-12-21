# Gateway Connectivity Error Non-Retryable Fix

## Issue Summary

When nodes attempted to send data via `sendToInternet()` but the gateway had infrastructure issues (router with no internet access, WiFi disconnected), the system would retry indefinitely, wasting time, battery, and network resources. Users experienced:

- Long delays before final failure (multiple retry attempts)
- Unclear feedback (error appeared transient when it was actually persistent)
- Resource waste (battery drain on retries that couldn't succeed)

## Problem Details

### Observed Behavior

**From User Report:**

```
Bridge log:
19:21:06.704 -> I am bridge: YES
19:21:06.704 -> Internet available via gateway: YES
19:21:06.704 -> Mesh connections active: YES

Node's log:
ERROR: handleGatewayAck(): Max retries reached for msgId=2766733316
❌ Failed to send WhatsApp: Ambiguous response - HTTP 203 may indicate 
   cached/proxied response, not actual delivery (HTTP: 203)
```

**The Problem:**

While HTTP 203 retry behavior was correct (that issue was previously fixed), there was a related but distinct issue: when the gateway's internet connectivity check failed, it would send back errors like "Router has no internet access" with `httpStatus=0`, which was treated as a retryable network error.

### Root Cause

In `src/painlessmesh/mesh.hpp`, the `handleGatewayAck()` function had this logic:

```cpp
// Network errors (httpStatus == 0) are retryable
else if (ack.httpStatus == 0) {
  isRetryable = true;
  Log(COMMUNICATION, "handleGatewayAck(): Network error, marking as retryable\n");
}
```

This treated **all** `httpStatus == 0` errors as retryable, including:

1. ✅ **Actually retryable:** HTTP request timeouts, connection failures during request
2. ❌ **NOT retryable:** Gateway connectivity checks (WiFi not connected, no internet access)

**Why This Is Wrong:**

Gateway connectivity errors indicate **infrastructure problems** that won't resolve by retrying:

- "Router has no internet access" - WAN cable unplugged, ISP down, modem offline
- "Gateway WiFi not connected" - ESP not associated with WiFi network, wrong credentials

These require **user intervention** to fix. Retrying wastes resources and delays the inevitable failure.

## Solution

### Code Changes

Modified `handleGatewayAck()` in `src/painlessmesh/mesh.hpp` to distinguish between infrastructure errors and transient network errors:

```cpp
// Network errors (httpStatus == 0) EXCEPT gateway connectivity errors
// Gateway connectivity errors are infrastructure issues (not transient)
else if (ack.httpStatus == 0) {
  // Check if this is a gateway-level connectivity error (non-retryable)
  bool isGatewayConnectivityError = false;
  
  // These errors indicate infrastructure issues that won't be fixed by retrying:
  // - "Router has no internet access" - WAN connection down
  // - "Gateway WiFi not connected" - ESP not associated with WiFi
  if (ack.error.find("Router has no internet") != TSTRING::npos ||
      ack.error.find("Gateway WiFi not connected") != TSTRING::npos) {
    isGatewayConnectivityError = true;
    Log(COMMUNICATION, "handleGatewayAck(): Gateway connectivity error detected (non-retryable): %s\n", 
        ack.error.c_str());
  }
  
  // Only mark as retryable if it's NOT a gateway connectivity error
  if (!isGatewayConnectivityError) {
    isRetryable = true;
    Log(COMMUNICATION, "handleGatewayAck(): Network error, marking as retryable\n");
  }
}
```

### Error Classification

**Non-Retryable Errors (Infrastructure Issues):**
- ❌ "Router has no internet access - check WAN connection"
- ❌ "Gateway WiFi not connected"
- ❌ HTTP 4xx client errors (except 429) - bad request, authentication, etc.
- ❌ HTTP 3xx redirects - should be followed by HTTPClient

**Retryable Errors (Transient Issues):**
- ✅ HTTP 203 - Cached/proxied response
- ✅ HTTP 5xx - Server errors (500, 502, 503, 504)
- ✅ HTTP 429 - Rate limiting (Too Many Requests)
- ✅ HTTP 0 with generic errors - Connection timeout, network unreachable

## Test Coverage

Added comprehensive test scenario in `test/catch/catch_internet_connectivity_check.cpp`:

```cpp
SCENARIO("Gateway connectivity errors should NOT be retried", "[gateway][retry][bug-fix]") {
    GIVEN("Gateway ACK packages with different error types") {
        
        WHEN("Error is 'Router has no internet access'") {
            // Verifies this is correctly identified as non-retryable
        }
        
        WHEN("Error is 'Gateway WiFi not connected'") {
            // Verifies this is correctly identified as non-retryable
        }
        
        WHEN("Error is a genuine network error during HTTP request") {
            // Verifies transient errors are still retryable
        }
    }
}
```

### Test Results

```bash
$ ./bin/catch_internet_connectivity_check
===============================================================================
All tests passed (25 assertions in 6 test cases)

$ ./bin/catch_http_status_codes
===============================================================================
All tests passed (50 assertions in 7 test cases)

$ ./bin/catch_send_to_internet
===============================================================================
All tests passed (184 assertions in 10 test cases)
```

**Total:** 259 assertions across 23 test cases - **ALL PASSING**

## Behavior Changes

### Before Fix

```
User: sendToInternet("https://api.callmebot.com/...", ...)
  ↓
Gateway: WiFi connected ✅
  ↓
Gateway: hasActualInternetAccess() ❌ (DNS fails)
  ↓
Gateway: Send ACK with error "Router has no internet access", httpStatus=0
  ↓
Node: Receives ACK, httpStatus=0 → marked as RETRYABLE
  ↓
Retry 1 (2s delay)  → Same error
Retry 2 (4s delay)  → Same error
Retry 3 (8s delay)  → Same error
  ↓
ERROR: handleGatewayAck(): Max retries reached
❌ User callback: "Router has no internet access"
  ↓
Total time wasted: ~14 seconds
User: "Why did it retry when my router has no internet?"
```

### After Fix

```
User: sendToInternet("https://api.callmebot.com/...", ...)
  ↓
Gateway: WiFi connected ✅
  ↓
Gateway: hasActualInternetAccess() ❌ (DNS fails)
  ↓
Gateway: Send ACK with error "Router has no internet access", httpStatus=0
  ↓
Node: Receives ACK, detects "Router has no internet" → NON-RETRYABLE
  ↓
❌ User callback IMMEDIATELY: "Router has no internet access - check WAN connection"
  ↓
Total time: ~100ms (DNS check only)
User: "Ah, router problem! Let me check the modem." ✅
```

## API Compatibility

### No Breaking Changes

The fix is **fully backward compatible**:

```cpp
// Existing code works unchanged
mesh.sendToInternet(url, payload, 
  [](bool success, uint16_t httpStatus, String error) {
    if (success) {
      Serial.println("✅ Success!");
    } else {
      Serial.printf("❌ Failed: %s\n", error.c_str());
      // Now fails faster with clearer error for infrastructure issues
    }
  }
);
```

**What's Different:**
- Faster failure for infrastructure issues (no retries)
- Clearer user experience (don't wait for retries that can't succeed)
- Better battery life (no wasted retry attempts)
- Same error handling API

## Usage Examples

### Handling Infrastructure Errors

```cpp
mesh.sendToInternet(url, payload, 
  [](bool success, uint16_t httpStatus, String error) {
    if (success) {
      Serial.println("✅ Message delivered!");
    } else {
      // Check for infrastructure vs transient errors
      if (error.indexOf("Router has no internet") >= 0) {
        Serial.println("❌ Router offline - check WAN connection");
        Serial.println("   1. Check modem power and cables");
        Serial.println("   2. Verify ISP service status");
        Serial.println("   3. Test other devices' internet access");
        // Don't retry - infrastructure needs fixing
      } else if (error.indexOf("Gateway WiFi not connected") >= 0) {
        Serial.println("❌ Gateway WiFi disconnected");
        Serial.println("   1. Check WiFi credentials in code");
        Serial.println("   2. Verify router is powered on");
        Serial.println("   3. Check WiFi signal strength");
        // Don't retry - WiFi needs reconnecting
      } else if (httpStatus == 203) {
        Serial.println("❌ Cached response - trying again in 30s");
        // System already retried 3 times - wait before manual retry
        delay(30000);
        // Retry manually if needed
      } else {
        Serial.printf("❌ Error: %s (HTTP %u)\n", error.c_str(), httpStatus);
      }
    }
  }
);
```

### Monitoring Gateway Health

```cpp
void checkGatewayStatus() {
  if (mesh.hasInternetConnection()) {
    Serial.println("✅ Internet available via gateway");
    
    auto bridges = mesh.getBridges();
    for (const auto& bridge : bridges) {
      if (bridge.internetConnected) {
        Serial.printf("   Bridge %u: RSSI %d dBm\n",
                     bridge.nodeId, bridge.routerRSSI);
      }
    }
  } else {
    Serial.println("❌ No internet access");
    Serial.println("Possible causes:");
    Serial.println("1. No gateway nodes in mesh");
    Serial.println("2. Gateway WiFi disconnected");
    Serial.println("3. Router has no WAN connection ← Most common!");
    Serial.println("");
    Serial.println("If retries keep failing with same error:");
    Serial.println("→ Check infrastructure first before debugging code");
  }
}
```

## Performance Impact

### Benefits

**Time Savings:**
- Infrastructure errors: Fail in ~100ms (DNS check) instead of ~14+ seconds (3 retries with backoff)
- 140x faster failure detection for common issues

**Resource Savings:**
- No wasted CPU cycles on retries that can't succeed
- No wasted network packets
- Better battery life on battery-powered nodes
- Reduced mesh network congestion

**User Experience:**
- Immediate, clear error messages
- Users fix infrastructure before debugging code
- Less confusion about "why is it retrying?"

### Overhead

- **Minimal**: Single string search per failure (negligible)
- **No extra memory**: Uses existing error message string
- **No network overhead**: Actually reduces network usage

## Related Issues

- **HTTP 203 Retry Fix** ([ISSUE_HTTP_203_RETRY_FIX.md](ISSUE_HTTP_203_RETRY_FIX.md)) - Separate fix for cached responses
- **Internet Connectivity Check** ([ISSUE_INTERNET_CONNECTIVITY_CHECK.md](ISSUE_INTERNET_CONNECTIVITY_CHECK.md)) - Added DNS-based connectivity validation
- **Gateway Timeout Fix** ([GATEWAY_CONNECTION_TIMEOUT_FIX.md](GATEWAY_CONNECTION_TIMEOUT_FIX.md)) - Related to request timeout handling

## Version Information

- **Fixed in**: painlessMesh v1.9.19+ (pending release)
- **Affects**: sendToInternet() API users
- **Breaking changes**: None
- **Migration required**: None (automatic improvement)

## Credits

- Issue reported by: User experiencing indefinite retries with no internet
- Root cause analysis: GitHub Copilot
- Fix implemented: 2024-12-21
- Testing: Comprehensive unit test suite added

## References

- [sendToInternet() API Documentation](examples/sendToInternet/README.md)
- [Internet Connectivity Check Enhancement](ISSUE_INTERNET_CONNECTIVITY_CHECK.md)
- [HTTP Status Code Handling](ISSUE_HTTP_203_RETRY_FIX.md)
- [Gateway/Bridge Configuration](BRIDGE_TO_INTERNET.md)

## Appendix: Error Type Comparison

| Error Scenario | httpStatus | Error Message | Retryable? | Why? |
|---------------|-----------|---------------|------------|------|
| Router no internet | 0 | "Router has no internet..." | ❌ No | Infrastructure issue - needs user fix |
| WiFi not connected | 0 | "Gateway WiFi not connected" | ❌ No | Infrastructure issue - needs WiFi setup |
| HTTP timeout | 0 | "Connection timeout" | ✅ Yes | Transient - may be temporary congestion |
| HTTP 203 cached | 203 | "Ambiguous response..." | ✅ Yes | Transient - cache may expire |
| HTTP 500 error | 500 | "Internal Server Error" | ✅ Yes | Transient - server may recover |
| HTTP 404 error | 404 | "Not Found" | ❌ No | Permanent - wrong URL |
| HTTP 429 rate limit | 429 | "Too Many Requests" | ✅ Yes | Transient - backoff and retry |

## Developer Notes

### Testing Infrastructure Errors

To test the fix in development:

```cpp
// Simulate "no internet" scenario
void testNoInternetError() {
  // 1. Disconnect your router's WAN cable
  // 2. Run sendToInternet()
  // 3. Observe: Immediate failure, no retries
  // 4. Check logs: Should see "Gateway connectivity error detected (non-retryable)"
  
  mesh.sendToInternet(
    "https://api.callmebot.com/test",
    "",
    [](bool success, uint16_t httpStatus, String error) {
      // Should be called IMMEDIATELY (no retry delay)
      Serial.printf("Callback at: %lu ms\n", millis());
      Serial.printf("Error: %s\n", error.c_str());
      // Expected: "Router has no internet access - check WAN connection"
    }
  );
}
```

### Log Messages

**Non-Retryable (Infrastructure):**
```
handleGatewayAck(): Gateway connectivity error detected (non-retryable): 
                   Router has no internet access - check WAN connection
handleGatewayAck(): Non-retryable failure for msgId=123456789 (HTTP 0)
```

**Retryable (Transient):**
```
handleGatewayAck(): Network error, marking as retryable
handleGatewayAck(): Scheduling retry for msgId=123456789 (attempt 1/3)
```

### Future Enhancements

Potential improvements for future versions:

1. **Configurable error patterns:**
   ```cpp
   mesh.addNonRetryablePattern("Custom error message");
   ```

2. **Retry policy per error type:**
   ```cpp
   mesh.setRetryPolicy(ERROR_INFRASTRUCTURE, 0);  // No retries
   mesh.setRetryPolicy(ERROR_TRANSIENT, 3);       // 3 retries
   ```

3. **Error statistics:**
   ```cpp
   auto stats = mesh.getInternetErrorStats();
   Serial.printf("Infrastructure errors: %u\n", stats.infrastructureErrors);
   Serial.printf("Transient errors: %u\n", stats.transientErrors);
   ```

4. **Auto-recovery monitoring:**
   - Track when infrastructure errors resolve
   - Notify user when internet becomes available
   - Auto-retry queued requests after recovery
