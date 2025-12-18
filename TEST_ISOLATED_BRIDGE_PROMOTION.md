# Test Plan: Isolated Bridge Promotion Fix

## Objective
Verify that the fix for the hard reset crash in `attemptIsolatedBridgePromotion()` resolves the issue without introducing regressions.

## Test Environment

### Hardware
- ESP32 or ESP8266 development boards (minimum 2 devices)
- WiFi router with known SSID/password
- Serial monitor connection for crash detection

### Software
- PlatformIO or Arduino IDE
- painlessMesh library with fix applied
- bridge_failover example sketch

## Test Cases

### Test Case 1: Isolated Node Promotion (Primary Test)

**Objective:** Verify that an isolated node can successfully promote to bridge without crashing

**Setup:**
1. Flash ONE node with bridge_failover example
2. Set `INITIAL_BRIDGE = false`
3. Configure correct `ROUTER_SSID` and `ROUTER_PASSWORD`
4. Ensure router is reachable
5. Power on the node

**Expected Behavior:**
1. Node starts in regular mode
2. After ~60 seconds (electionStartupDelayMs), node detects isolation
3. Node detects router availability (via scan)
4. Node attempts isolated bridge promotion
5. Node successfully initializes as bridge
6. Callback fires: "🎯 PROMOTED TO BRIDGE: Isolated node promoted to bridge"
7. **NO CRASH** - Node continues running
8. Bridge status broadcasts are sent
9. Serial log shows: "Bridge status announcement will be sent by bridge status broadcast system"

**Success Criteria:**
- ✅ No Guru Meditation Error or hard reset
- ✅ Node successfully operates as bridge
- ✅ Bridge status broadcasts visible in logs
- ✅ Node maintains router connection
- ✅ Free heap remains stable

**Failure Indicators:**
- ❌ Guru Meditation Error with `0xbaad5678` memory marker
- ❌ Unexpected resets
- ❌ No bridge status broadcasts
- ❌ Node stuck in crash loop

### Test Case 2: Multi-Node Mesh with Isolated Promotion

**Objective:** Verify isolated promotion works when a node temporarily loses mesh connectivity

**Setup:**
1. Flash two nodes with bridge_failover example
2. Node A: `INITIAL_BRIDGE = true`
3. Node B: `INITIAL_BRIDGE = false`
4. Both have router credentials
5. Start both nodes, let them form mesh
6. Move Node B out of range or power off Node A

**Expected Behavior:**
1. Node B detects loss of bridge/mesh
2. After timeout + random delay, Node B starts isolated promotion
3. Node B successfully promotes to bridge without crash
4. Node B broadcasts bridge status

**Success Criteria:**
- ✅ Node B promotes without crash
- ✅ Node B maintains router connection
- ✅ If Node A returns, mesh reforms with Node B as bridge

### Test Case 3: Router Unreachable Scenario

**Objective:** Verify graceful handling when router is not available

**Setup:**
1. Flash one node with bridge_failover example
2. Set `INITIAL_BRIDGE = false`
3. Configure INCORRECT router credentials or disable router
4. Power on node

**Expected Behavior:**
1. Node attempts isolated promotion
2. `initAsBridge()` fails (router unreachable)
3. Node reverts to regular mode
4. Callback fires: "Isolated bridge promotion failed - router unreachable"
5. **NO CRASH** - Node continues running
6. Node continues retry attempts

**Success Criteria:**
- ✅ No crashes despite failed promotion
- ✅ Error messages in logs indicating router unreachable
- ✅ Node continues to retry periodically
- ✅ Free heap remains stable

### Test Case 4: Rapid Retry Attempts

**Objective:** Verify the retry mechanism doesn't cause crashes

**Setup:**
1. Flash one node with bridge_failover example
2. Configure correct router credentials
3. Simulate intermittent router availability (power cycle router)

**Expected Behavior:**
1. Node attempts promotion when router visible
2. If router disappears, promotion fails gracefully
3. Node retries up to MAX_ISOLATED_BRIDGE_RETRY_ATTEMPTS (5)
4. After max attempts, waits for reset interval
5. **NO CRASHES** during any retry attempts

**Success Criteria:**
- ✅ Retry counter increments correctly
- ✅ No crashes during retry cycle
- ✅ Reset timeout works correctly
- ✅ Eventually succeeds when router stable

### Test Case 5: Bridge Status Broadcast Verification

**Objective:** Verify bridge status broadcasts are sent after successful promotion

**Setup:**
1. Flash one bridge node (from Test Case 1)
2. Flash one regular node listening for bridge status
3. Power on both nodes sequentially

**Expected Behavior:**
1. Bridge node promotes successfully
2. Bridge sends immediate status broadcast (from initBridgeStatusBroadcast)
3. Regular node receives bridge status
4. Periodic broadcasts continue every 30 seconds

**Success Criteria:**
- ✅ Regular node receives bridge status within 5 seconds
- ✅ Bridge status includes correct information:
  - internetConnected: true
  - routerRSSI: valid value
  - routerChannel: correct channel
- ✅ Periodic broadcasts continue
- ✅ No duplicate broadcasts from the removed addTask call

### Test Case 6: Callback Execution Safety

**Objective:** Verify callback can perform various operations safely

**Setup:**
1. Modify bridge_failover example to add operations in callback:
```cpp
void bridgeRoleCallback(bool isBridge, const String& reason) {
  if (isBridge) {
    Serial.printf("🎯 PROMOTED TO BRIDGE: %s\n", reason.c_str());
    Serial.println("This node is now the primary bridge!");
    
    // Test various operations
    Serial.printf("Free heap: %u\n", ESP.getFreeHeap());
    Serial.printf("Node ID: %u\n", mesh.getNodeId());
    Serial.printf("Mesh time: %u\n", mesh.getNodeTime());
  }
}
```

**Expected Behavior:**
1. Callback executes successfully
2. All operations in callback complete without error
3. **NO CRASH** after callback returns
4. Bridge continues normal operation

**Success Criteria:**
- ✅ Callback completes all operations
- ✅ No crashes during or after callback
- ✅ All print statements visible in logs

## Test Execution Log Template

```
Test Case: _______________
Date: _______________
Tester: _______________
Hardware: ESP32 / ESP8266
Library Version: _______________

Setup Steps Completed: ☐ Yes ☐ No
Notes: _______________________________________________

Test Results:
☐ PASS ☐ FAIL ☐ PARTIAL

Observations:
_______________________________________________
_______________________________________________

Serial Log Excerpt:
```
_______________________________________________
_______________________________________________
```

Issues Found:
_______________________________________________
_______________________________________________

Free Heap Before: _______ bytes
Free Heap After: _______ bytes
Heap Delta: _______ bytes

Additional Notes:
_______________________________________________
_______________________________________________
```

## Automated Testing (Optional)

Create a test harness that:
1. Monitors serial output for crash signatures
2. Checks for expected log messages
3. Measures heap usage
4. Simulates router availability cycles
5. Validates bridge status broadcasts

Example test assertions:
```cpp
void test_isolated_bridge_promotion() {
  // Wait for promotion attempt
  waitForLogMessage("Attempting direct bridge promotion");
  
  // Verify success
  assertLogContains("✓ Isolated bridge promotion complete");
  
  // Verify no crash
  assertNoLogContains("Guru Meditation Error");
  assertNoLogContains("0xbaad5678");
  
  // Verify bridge status
  assertLogContains("Bridge status announcement will be sent");
  
  // Verify stability
  delay(10000);
  assertHeapStable();
  assertNodeResponsive();
}
```

## Regression Testing

After fix verification, run full painlessMesh test suite:
1. Basic mesh formation tests
2. Bridge failover tests
3. Election mechanism tests
4. Message routing tests
5. Connection stability tests
6. Memory leak tests

## Performance Metrics

Track the following metrics before and after fix:
- **Memory Usage:** Free heap during promotion
- **Promotion Time:** Duration from attempt to success
- **Broadcast Latency:** Time from promotion to first broadcast
- **CPU Usage:** Impact on other tasks during promotion
- **Network Traffic:** Number of broadcasts sent

## Success Criteria Summary

The fix is considered successful if:
1. ✅ Zero crashes in Test Cases 1-6
2. ✅ All expected behaviors occur
3. ✅ No regressions in existing functionality
4. ✅ Performance metrics remain acceptable
5. ✅ Code passes code review
6. ✅ Documentation is complete

## Failure Response

If any test fails:
1. Document the failure mode in detail
2. Collect complete serial logs
3. Check for related issues in other test cases
4. Review the fix implementation
5. Consider alternative approaches
6. Re-test after fix adjustment

## Sign-Off

```
Developer: _______________  Date: _______________
Tester: _______________     Date: _______________
Reviewer: _______________   Date: _______________
```
