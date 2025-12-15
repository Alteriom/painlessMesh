# Fix for TCP Connection Retry Immediate Execution Bug

## Problem

ESP32/ESP8266 devices were experiencing TCP connection failures (error -14 ERR_CONN) where retry attempts were happening immediately instead of with the intended exponential backoff delays. This defeated the purpose of the retry mechanism and prevented nodes from successfully establishing mesh connections.

## Symptoms

From user logs:
```
09:40:02.140 -> CONNECTION: tcp_err(): error trying to connect -14 (attempt 1/6)
09:40:02.175 -> CONNECTION: tcp_err(): Scheduling retry in 1000 ms (backoff x1)
09:40:02.175 -> CONNECTION: tcp_err(): Cleaning up failed AsyncClient (retry path)
09:40:02.175 -> CONNECTION: tcp_err(): Retrying TCP connection...
09:40:02.175 -> CONNECTION: tcp::connect(): Attempting connection to port 5555 (attempt 2/6)
```

Notice all messages appear at the same millisecond (09:40:02.175), meaning the retry happened **immediately** instead of 1 second later as intended.

Expected behavior:
```
09:40:02.140 -> tcp_err(): error -14 (attempt 1/6)
09:40:02.175 -> tcp_err(): Scheduling retry in 1000 ms
09:40:03.175 -> tcp_err(): Retrying TCP connection...  [1 second later]
09:40:03.175 -> tcp::connect(): Attempting connection (attempt 2/6)
```

## Root Cause

The TCP retry mechanism in `src/painlessmesh/tcp.hpp` schedules delayed retries using:

```cpp
mesh.addTask([&mesh, ip, port, retryCount]() {
  Log(CONNECTION, "tcp_err(): Retrying TCP connection...\n");
  AsyncClient *pRetryConn = new AsyncClient();
  connect<T, M>((*pRetryConn), ip, port, mesh, retryCount + 1);
}, retryDelay);
```

This calls `mesh.addTask(callback, delayMs)` which is defined in `mesh.hpp` as:

```cpp
inline std::shared_ptr<Task> addTask(std::function<void()> aCallback,
                                     unsigned long delayMs) {
  return plugin::PackageHandler<T>::addTask((*this->mScheduler), delayMs,
                                            TASK_ONCE, aCallback);
}
```

The problem was in `PackageHandler::addTask()` in `src/painlessmesh/plugin.hpp`:

```cpp
std::shared_ptr<Task> addTask(Scheduler& scheduler, unsigned long aInterval,
                              long aIterations,
                              std::function<void()> aCallback) {
  // ... task reuse logic ...
  
  std::shared_ptr<Task> task =
      std::make_shared<Task>(aInterval, aIterations, aCallback);
  scheduler.addTask((*task));
  task->enable();  // BUG: This causes immediate execution!
  taskList.push_front(task);
  return task;
}
```

### TaskScheduler Behavior

The TaskScheduler library has two methods for enabling tasks:

1. **`enable()`**: Activates the task immediately
   - For TASK_ONCE: executes immediately, then stops
   - For TASK_FOREVER: executes immediately, waits interval, repeats

2. **`enableDelayed()`**: Waits for the interval before first execution
   - For TASK_ONCE: waits interval, executes once, stops
   - For TASK_FOREVER: waits interval, executes, waits interval, repeats

For a one-shot delayed task (TASK_ONCE with interval > 0), we need `enableDelayed()` not `enable()`.

## Solution

Modified `PackageHandler::addTask()` to use `enableDelayed()` for one-shot tasks with intervals:

```cpp
std::shared_ptr<Task> addTask(Scheduler& scheduler, unsigned long aInterval,
                              long aIterations,
                              std::function<void()> aCallback) {
  // ... task reuse logic with same fix ...
  
  std::shared_ptr<Task> task =
      std::make_shared<Task>(aInterval, aIterations, aCallback);
  scheduler.addTask((*task));
  
  // Use enableDelayed() for delayed one-shot tasks to prevent immediate execution
  // This ensures tasks with intervals execute after the delay, not immediately
  if (aInterval > 0 && aIterations == TASK_ONCE) {
    task->enableDelayed();
  } else {
    task->enable();
  }
  
  taskList.push_front(task);
  return task;
}
```

### Why This Fixes the Issue

1. **TCP retries now wait properly**: When a retry is scheduled with `addTask(callback, 1000)`, the task waits 1000ms before executing instead of executing immediately

2. **Exponential backoff works**: The retry delays (1s, 2s, 4s, 8s, 8s) are now properly enforced, giving the network stack and TCP server time to stabilize

3. **No breaking changes**: Recurring tasks and zero-interval tasks still use `enable()` for immediate execution, preserving existing behavior

## TCP Retry Mechanism Details

### Configuration Constants (from `tcp.hpp`)
```cpp
static const uint8_t TCP_CONNECT_MAX_RETRIES = 5;              // 5 retries (6 total attempts)
static const uint32_t TCP_CONNECT_RETRY_DELAY_MS = 1000;       // Base delay: 1 second
static const uint32_t TCP_CONNECT_STABILIZATION_DELAY_MS = 500; // 500ms stabilization
static const uint32_t TCP_CLIENT_CLEANUP_DELAY_MS = 500;       // 500ms cleanup delay
static const uint32_t TCP_EXHAUSTION_RECONNECT_DELAY_MS = 10000; // 10s before reconnection
```

### Exponential Backoff Formula
```cpp
uint8_t backoffMultiplier = (retryCount < 3) ? (1U << retryCount) : 8;
uint32_t retryDelay = TCP_CONNECT_RETRY_DELAY_MS * backoffMultiplier;
```

### Retry Sequence
| Attempt | retryCount | Multiplier | Delay | Cumulative |
|---------|-----------|------------|-------|------------|
| 1 (initial) | 0 | N/A | 0ms | 0s |
| 2 (retry 1) | 0 | 1 | 1000ms | 1s |
| 3 (retry 2) | 1 | 2 | 2000ms | 3s |
| 4 (retry 3) | 2 | 4 | 4000ms | 7s |
| 5 (retry 4) | 3 | 8 | 8000ms | 15s |
| 6 (retry 5) | 4 | 8 | 8000ms | 23s |
| WiFi reconnect | N/A | N/A | 10000ms | 33s |

**Total time before WiFi reconnection**: ~33 seconds (23s retries + 10s exhaustion delay)

## Impact

✅ **Fixes critical connection failures**: Nodes can now successfully establish mesh connections  
✅ **Enables proper retry logic**: Exponential backoff reduces network congestion  
✅ **No breaking changes**: Fully backward compatible with existing code  
✅ **Well-tested**: All 650+ existing assertions pass plus new validation tests  
✅ **Documented**: Code comments, test documentation, and troubleshooting guide updated  

## Testing

### New Test File
Created `test/catch/catch_delayed_task_execution.cpp` to document and validate:
- Task scheduling behavior differences between `enable()` and `enableDelayed()`
- Delayed one-shot task execution patterns
- TCP retry mechanism task scheduling
- Exponential backoff calculation

### Test Results
```
catch_delayed_task_execution: 11 assertions passed ✅
catch_tcp_retry: 31 assertions passed ✅
All tests: 650+ assertions passed ✅
```

## Files Modified

1. **`src/painlessmesh/plugin.hpp`** - Fixed `PackageHandler::addTask()` to use `enableDelayed()`
2. **`docs/troubleshooting/common-issues.md`** - Updated TCP retry documentation
3. **`test/catch/catch_delayed_task_execution.cpp`** - Added validation tests

## Related Issues and Fixes

This fix builds upon previous TCP connection improvements:

### Issue #231 (v1.9.6)
- Introduced TCP retry mechanism with exponential backoff
- Increased retries from 3 to 5 attempts
- Added stabilization delay after IP acquisition

### Issue #254 (v1.9.8)
- Deferred AsyncClient deletion to prevent heap corruption
- Changed from synchronous `delete` to deferred cleanup

### Issue #269 (v1.9.9)
- Increased AsyncClient cleanup delay from 0ms to 500ms
- Prevented crashes during retry sequence

### Current Fix
- Fixed task scheduling to properly delay retry attempts
- Completes the TCP retry mechanism implementation
- Ensures exponential backoff actually works as designed

## Prevention

To prevent similar issues in the future:

1. **When scheduling delayed tasks**: Use `enableDelayed()` for one-shot tasks that should execute after a delay
2. **When scheduling periodic tasks**: Use `enable()` for immediate execution or `enableDelayed()` for delayed start
3. **Test timing-sensitive code**: Verify task execution happens at the expected time, not immediately

## See Also

- `src/painlessmesh/tcp.hpp` - TCP connection handling with retry logic
- `src/painlessmesh/plugin.hpp` - Task scheduling implementation
- `ISSUE_231_FIX_SUMMARY.md` - Previous TCP retry improvements
- `ASYNCCLIENT_CLEANUP_FIX.md` - AsyncClient lifecycle management
- `docs/troubleshooting/common-issues.md` - User-facing troubleshooting guide
