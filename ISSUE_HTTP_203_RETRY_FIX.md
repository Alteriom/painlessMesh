# HTTP 203 Retry Fix - "Permanent Response" Issue Resolution

## Issue Summary

When using `sendToInternet()` with APIs like Callmebot WhatsApp, users experienced a problem where HTTP 203 (Non-Authoritative Information) responses appeared "permanent" - requests would fail with HTTP 203 and never recover, even though the issue was likely temporary (cached/proxied responses).

## Problem Details

### Observed Behavior

**Node Log:**
```
00:35:40.035 ->    Message queued with ID: 2766733313
00:36:07.311 -> ❌ Failed to send WhatsApp: Ambiguous response - HTTP 203 may indicate cached/proxied response, not actual delivery (HTTP: 203)
```

**Bridge Log:**
- HTTP 203 received from Callmebot API
- Response correctly identified as failure
- BUT: Request immediately terminated (no retry)

### Root Cause

The system correctly identified HTTP 203 as a failure (not success), but treated it as a **terminal failure**. Once HTTP 203 was received:

1. ✅ Gateway correctly sent ACK with `success=false`
2. ✅ Node correctly displayed error message
3. ❌ **Request immediately removed from pending queue (no retry)**
4. ❌ **User had no automatic recovery mechanism**

This created the "permanent" problem - the cached response issue never cleared because retries weren't attempted.

## Solution

### Code Changes

Modified `handleGatewayAck()` in `src/painlessmesh/mesh.hpp` to implement intelligent retry logic:

```cpp
// Check if this is a success response
if (ack.success) {
  // Success - call callback and remove request
  if (request.callback) {
    request.callback(ack.success, ack.httpStatus, ack.error);
  }
  pendingInternetRequests.erase(it);
  return;
}

// Failure response - determine if retryable
bool isRetryable = false;

// HTTP 203 (Non-Authoritative Information) - cached/proxied response
// This is often temporary and retrying may succeed when cache expires
if (ack.httpStatus == 203) {
  isRetryable = true;
}
// HTTP 5xx server errors are typically transient
else if (ack.httpStatus >= 500 && ack.httpStatus < 600) {
  isRetryable = true;
}
// HTTP 429 (Too Many Requests) should be retried with backoff
else if (ack.httpStatus == 429) {
  isRetryable = true;
}
// Network errors (httpStatus == 0) are retryable
else if (ack.httpStatus == 0) {
  isRetryable = true;
}

// If retryable and have retries left, schedule retry
if (isRetryable && request.retryCount < request.maxRetries) {
  scheduleInternetRetry(ack.messageId);
} else {
  // Not retryable or max retries reached - call callback and remove
  if (request.callback) {
    request.callback(ack.success, ack.httpStatus, ack.error);
  }
  pendingInternetRequests.erase(it);
}
```

### Retry Categories

**Retryable Errors (automatic retry with exponential backoff):**
- ✅ HTTP 203 - Non-Authoritative Information (cached/proxied response)
- ✅ HTTP 5xx - Server errors (500, 502, 503, 504, etc.)
- ✅ HTTP 429 - Too Many Requests (rate limiting)
- ✅ HTTP 0 - Network errors (connection failed, timeout, etc.)

**Non-Retryable Errors (immediate callback, no retry):**
- ❌ HTTP 4xx (except 429) - Client errors (400, 401, 404, etc.)
- ❌ HTTP 3xx - Redirects (should be followed by HTTPClient)
- ❌ HTTP 2xx (except 203) - Success codes (200, 201, 202, 204)

### Exponential Backoff

Retries use exponential backoff to avoid overwhelming servers:

```
Attempt 1: Immediate send         → HTTP 203
Attempt 2: Wait 2 seconds         → HTTP 203
Attempt 3: Wait 4 seconds         → HTTP 203
Attempt 4: Wait 8 seconds         → HTTP 200 ✅ SUCCESS!
```

Default: 3 retries with 2-second base delay
Configurable via `sendToInternet()` parameters

## Test Coverage

Added comprehensive test scenarios in `test/catch/catch_http_status_codes.cpp`:

### Test Scenarios

1. **HTTP Status Classification**
   - ✅ 200, 201, 202, 204 = Success
   - ✅ 203, 205, 206 = Failure
   - ✅ 3xx, 4xx, 5xx = Various failure types

2. **Retry Behavior**
   - ✅ HTTP 203 triggers retry
   - ✅ HTTP 5xx triggers retry
   - ✅ HTTP 429 triggers retry
   - ✅ HTTP 0 (network error) triggers retry
   - ✅ HTTP 4xx (except 429) does NOT retry
   - ✅ HTTP 3xx does NOT retry
   - ✅ Success codes do NOT retry

3. **Issue-Specific Tests**
   - ✅ HTTP 203 "permanent response" issue resolved
   - ✅ Cache expiration scenario handled correctly
   - ✅ Exponential backoff behavior documented

### Test Results

```bash
$ ./bin/catch_http_status_codes
===============================================================================
All tests passed (50 assertions in 7 test cases)

$ ./bin/catch_gateway_ack_package
===============================================================================
All tests passed (40 assertions in 8 test cases)

$ ./bin/catch_gateway_data_package
===============================================================================
All tests passed (58 assertions in 10 test cases)

$ ./bin/catch_disconnected_mesh_internet
===============================================================================
All tests passed (12 assertions in 6 test cases)
```

**Total:** 160 assertions, 31 test cases - **ALL PASSING**

## Behavior Changes

### Before Fix

```
User sends WhatsApp message
  ↓
Bridge receives HTTP 203 from Callmebot
  ↓
ACK sent with success=false
  ↓
Node receives ACK
  ↓
❌ Callback called immediately
❌ Request removed from queue
❌ NO RETRY
  ↓
User sees: "❌ Failed to send WhatsApp: Ambiguous response..."
  ↓
PERMANENT FAILURE - User must manually retry
```

### After Fix

```
User sends WhatsApp message
  ↓
Bridge receives HTTP 203 from Callmebot
  ↓
ACK sent with success=false, httpStatus=203
  ↓
Node receives ACK
  ↓
✅ Identifies HTTP 203 as retryable
✅ Schedules retry with exponential backoff
  ↓
Retry 1 (2s delay) → HTTP 203
Retry 2 (4s delay) → HTTP 203
Retry 3 (8s delay) → HTTP 200 SUCCESS!
  ↓
✅ Callback called with success=true
✅ Request removed from queue
  ↓
User sees: "✅ WhatsApp message sent! HTTP Status: 200"
  ↓
AUTOMATIC RECOVERY - No manual intervention needed
```

## API Compatibility

### No Breaking Changes

The fix is **fully backward compatible**:

```cpp
// Existing code continues to work unchanged
mesh.sendToInternet(
  "https://api.callmebot.com/whatsapp.php?phone=+1234567890&apikey=KEY&text=Hello",
  "",
  [](bool success, uint16_t httpStatus, String error) {
    if (success) {
      Serial.printf("✅ Message sent! HTTP: %u\n", httpStatus);
    } else {
      Serial.printf("❌ Failed: %s (HTTP: %u)\n", error.c_str(), httpStatus);
    }
  }
);
```

**What's Different:**
- HTTP 203 failures now automatically retry (transparent to user)
- User callback still receives final success/failure status
- Existing error handling code works unchanged
- Optional: Can configure retry behavior via sendToInternet() parameters

## Usage Examples

### Default Behavior (Automatic Retry)

```cpp
// HTTP 203 will automatically retry up to 3 times
mesh.sendToInternet(url, payload, callback);
```

### Custom Retry Configuration

```cpp
// Configure retry behavior
mesh.sendToInternet(
  url,
  payload,
  callback,
  5,      // maxRetries = 5 attempts
  3000,   // retryDelayMs = 3 second base delay
  30000   // timeoutMs = 30 second timeout
);
```

### Handling Final Failure

```cpp
mesh.sendToInternet(url, payload, 
  [](bool success, uint16_t httpStatus, String error) {
    if (success) {
      Serial.println("✅ Message delivered successfully");
    } else if (httpStatus == 203) {
      Serial.println("❌ Cached response - max retries exhausted");
      Serial.println("   Consider increasing retry count or delay");
    } else {
      Serial.printf("❌ Failed: %s (HTTP: %u)\n", error.c_str(), httpStatus);
    }
  }
);
```

## Performance Impact

- **Minimal overhead**: Single boolean check per ACK
- **No additional memory**: Uses existing retry infrastructure
- **Network efficiency**: Exponential backoff prevents flooding
- **Battery friendly**: Longer delays between retries save power

## Related Issues

- Fixed: HTTP 203 "permanent response" issue
- Related: Bridge failover with sendToInternet (separate fix)
- Related: HTTP status code interpretation (previously fixed)

## Version Information

- **Fixed in**: painlessMesh v1.9.12+ (pending release)
- **Affects**: sendToInternet() API users
- **Breaking changes**: None
- **Migration required**: None

## References

- [sendToInternet() API Documentation](USER_GUIDE.md#sendtointernet)
- [Bridge Status Documentation](BRIDGE_TO_INTERNET.md)
- [HTTP Status Codes RFC](https://www.rfc-editor.org/rfc/rfc7231#section-6)
- [Callmebot WhatsApp API](https://www.callmebot.com/blog/free-api-whatsapp-messages/)

## Credits

- Issue reported by: User experiencing "permanent" HTTP 203 responses
- Root cause analysis: GitHub Copilot
- Fix implemented: 2024-12-18
- Testing: Comprehensive unit test suite added

## Appendix: HTTP 203 Explained

**HTTP 203 (Non-Authoritative Information)** means:
- Response came from a cache or proxy
- NOT from the origin server (e.g., Callmebot API)
- May be outdated or not reflect actual delivery

**For Callmebot WhatsApp API:**
- HTTP 203 = Message likely NOT delivered
- Message stuck in proxy cache
- Retrying allows cache to expire
- Fresh request reaches actual API server
- HTTP 200 = Message actually delivered

**Why This Matters:**
- User thinks message sent (saw "success" in old code)
- Message never actually sent
- Important notifications missed
- Fix ensures retries until genuine delivery or max retries
