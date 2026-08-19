//************************************************************
// Mock HTTP Server Integration Example
//
// This example demonstrates how to use the mock HTTP server
// for testing sendToInternet() functionality without requiring
// actual Internet connectivity.
//
// Setup:
// 1. Start the mock server on your development machine:
//    cd test/mock-http-server
//    python3 server.py
//
// 2. Update MOCK_SERVER_IP below with your computer's IP address
//
// 3. Upload this sketch to your ESP32/ESP8266
//
// 4. Watch the serial output and mock server logs
//
// The sketch will automatically run through various test scenarios
// and report pass/fail results for each test case.
//************************************************************

#include "painlessMesh.h"

// ============================================
// Configuration
// ============================================
#define MESH_PREFIX     "TestMesh"
#define MESH_PASSWORD   "testpass123"
#define MESH_PORT       5555

// Router credentials (for bridge node)
#define ROUTER_SSID     "YourRouterSSID"
#define ROUTER_PASSWORD "YourRouterPassword"

// Mock HTTP Server Configuration
// Replace with your computer's IP address where mock server is running
#define MOCK_SERVER_IP   "192.168.1.100"  // ← CHANGE THIS
#define MOCK_SERVER_PORT 8080

// Set to true if this node should be the bridge
#define IS_BRIDGE true

// ============================================
// Global Objects
// ============================================
Scheduler userScheduler;
painlessMesh mesh;

// Test tracking
int totalTests = 0;
int passedTests = 0;
int failedTests = 0;

// ============================================
// Helper Functions
// ============================================

String getMockServerUrl(const String& endpoint) {
    return String("http://") + MOCK_SERVER_IP + ":" + String(MOCK_SERVER_PORT) + endpoint;
}

void recordTestResult(const String& testName, bool passed) {
    totalTests++;
    if (passed) {
        passedTests++;
        Serial.printf("✓ PASS: %s\n", testName.c_str());
    } else {
        failedTests++;
        Serial.printf("✗ FAIL: %s\n", testName.c_str());
    }
}

void printTestSummary() {
    Serial.println("\n========================================");
    Serial.println("Test Summary");
    Serial.println("========================================");
    Serial.printf("Total Tests:  %d\n", totalTests);
    Serial.printf("Passed:       %d\n", passedTests);
    Serial.printf("Failed:       %d\n", failedTests);
    Serial.printf("Success Rate: %.1f%%\n", (totalTests > 0) ? (100.0 * passedTests / totalTests) : 0.0);
    Serial.println("========================================\n");
}

// ============================================
// Test Cases
// ============================================

/**
 * Test Case 1: HTTP 200 Success
 * Expected: success=true, status=200
 */
void testHttp200Success() {
    Serial.println("\n[Test 1] HTTP 200 Success Response");
    
    String url = getMockServerUrl("/status/200");
    Serial.printf("URL: %s\n", url.c_str());
    
    mesh.sendToInternet(url, "", [](bool success, uint16_t status, String error) {
        bool testPassed = (success && status == 200);
        recordTestResult("HTTP 200 Success", testPassed);
        
        if (!testPassed) {
            Serial.printf("  Expected: success=true, status=200\n");
            Serial.printf("  Got:      success=%s, status=%d, error=%s\n",
                         success ? "true" : "false", status, error.c_str());
        }
    });
}

/**
 * Test Case 2: HTTP 203 Non-Authoritative
 * Expected: success=false (per Issue #336 fix)
 */
void testHttp203Failure() {
    Serial.println("\n[Test 2] HTTP 203 Non-Authoritative (should fail)");
    
    String url = getMockServerUrl("/status/203");
    Serial.printf("URL: %s\n", url.c_str());
    
    mesh.sendToInternet(url, "", [](bool success, uint16_t status, String error) {
        // HTTP 203 should be treated as failure (cached/proxied response)
        bool testPassed = (!success && status == 203);
        recordTestResult("HTTP 203 treated as failure", testPassed);
        
        if (!testPassed) {
            Serial.printf("  Expected: success=false, status=203\n");
            Serial.printf("  Got:      success=%s, status=%d\n",
                         success ? "true" : "false", status);
        }
    });
}

/**
 * Test Case 3: HTTP 404 Not Found
 * Expected: success=false, status=404
 */
void testHttp404NotFound() {
    Serial.println("\n[Test 3] HTTP 404 Not Found");
    
    String url = getMockServerUrl("/status/404");
    Serial.printf("URL: %s\n", url.c_str());
    
    mesh.sendToInternet(url, "", [](bool success, uint16_t status, String error) {
        bool testPassed = (!success && status == 404);
        recordTestResult("HTTP 404 Not Found", testPassed);
        
        if (!testPassed) {
            Serial.printf("  Expected: success=false, status=404\n");
            Serial.printf("  Got:      success=%s, status=%d\n",
                         success ? "true" : "false", status);
        }
    });
}

/**
 * Test Case 4: HTTP 500 Internal Server Error
 * Expected: success=false, status=500
 */
void testHttp500ServerError() {
    Serial.println("\n[Test 4] HTTP 500 Internal Server Error");
    
    String url = getMockServerUrl("/status/500");
    Serial.printf("URL: %s\n", url.c_str());
    
    mesh.sendToInternet(url, "", [](bool success, uint16_t status, String error) {
        bool testPassed = (!success && status == 500);
        recordTestResult("HTTP 500 Server Error", testPassed);
        
        if (!testPassed) {
            Serial.printf("  Expected: success=false, status=500\n");
            Serial.printf("  Got:      success=%s, status=%d\n",
                         success ? "true" : "false", status);
        }
    });
}

/**
 * Test Case 5: WhatsApp API Simulation
 * Expected: success=true, status=200
 */
void testWhatsAppApiSimulation() {
    Serial.println("\n[Test 5] WhatsApp API Simulation");
    
    String url = getMockServerUrl("/whatsapp");
    url += "?phone=%2B1234567890";
    url += "&apikey=test_key";
    url += "&text=Test%20message%20from%20ESP";
    
    Serial.printf("URL: %s\n", url.c_str());
    
    mesh.sendToInternet(url, "", [](bool success, uint16_t status, String error) {
        bool testPassed = (success && status == 200);
        recordTestResult("WhatsApp API Simulation", testPassed);
        
        if (!testPassed) {
            Serial.printf("  Expected: success=true, status=200\n");
            Serial.printf("  Got:      success=%s, status=%d, error=%s\n",
                         success ? "true" : "false", status, error.c_str());
        }
    });
}

/**
 * Test Case 6: Delayed Response (2 seconds)
 * Expected: success=true after 2+ seconds
 */
void testDelayedResponse() {
    Serial.println("\n[Test 6] Delayed Response (2 seconds)");
    
    String url = getMockServerUrl("/delay/2");
    Serial.printf("URL: %s\n", url.c_str());
    
    unsigned long startTime = millis();
    
    mesh.sendToInternet(url, "", [startTime](bool success, uint16_t status, String error) {
        unsigned long elapsed = millis() - startTime;
        bool testPassed = (success && status == 200 && elapsed >= 2000);
        recordTestResult("Delayed Response (2s)", testPassed);
        
        Serial.printf("  Response time: %lu ms\n", elapsed);
        
        if (!testPassed) {
            Serial.printf("  Expected: success=true, elapsed >= 2000ms\n");
            Serial.printf("  Got:      success=%s, elapsed=%lu ms\n",
                         success ? "true" : "false", elapsed);
        }
    });
}

/**
 * Test Case 7: Echo Endpoint (POST with payload)
 * Expected: success=true, server echoes back our data
 */
void testEchoEndpoint() {
    Serial.println("\n[Test 7] Echo Endpoint with JSON Payload");
    
    String url = getMockServerUrl("/echo");
    String payload = "{\"sensor\":\"temperature\",\"value\":25.5,\"unit\":\"celsius\"}";
    
    Serial.printf("URL: %s\n", url.c_str());
    Serial.printf("Payload: %s\n", payload.c_str());
    
    mesh.sendToInternet(url, payload, [](bool success, uint16_t status, String error) {
        bool testPassed = (success && status == 200);
        recordTestResult("Echo Endpoint", testPassed);
        
        if (!testPassed) {
            Serial.printf("  Expected: success=true, status=200\n");
            Serial.printf("  Got:      success=%s, status=%d\n",
                         success ? "true" : "false", status);
        }
    });
}

// ============================================
// Task to Run All Tests
// ============================================

void runAllTests() {
    Serial.println("\n\n");
    Serial.println("========================================");
    Serial.println("Starting Mock HTTP Server Test Suite");
    Serial.println("========================================");
    Serial.printf("Mock Server: http://%s:%d\n", MOCK_SERVER_IP, MOCK_SERVER_PORT);
    Serial.println("========================================\n");
    
    // Wait a bit for mesh to stabilize
    delay(5000);
    
    // Run all test cases with delays between them
    testHttp200Success();
    delay(2000);
    
    testHttp203Failure();
    delay(2000);
    
    testHttp404NotFound();
    delay(2000);
    
    testHttp500ServerError();
    delay(2000);
    
    testWhatsAppApiSimulation();
    delay(2000);
    
    testDelayedResponse();
    delay(4000);  // Extra delay for the delayed response test
    
    testEchoEndpoint();
    delay(2000);
    
    // Print summary after all tests complete
    delay(5000);
    printTestSummary();
}

Task taskRunTests(TASK_ONCE, TASK_IMMEDIATE, &runAllTests, &userScheduler);

// ============================================
// Setup Function
// ============================================

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n\n");
    Serial.println("================================================");
    Serial.println("   Mock HTTP Server Integration Example");
    Serial.println("   painlessMesh sendToInternet Testing");
    Serial.println("================================================\n");
    
    // Configure mesh
    mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
    
    bool success = false;
    
#if IS_BRIDGE
    // Initialize as bridge with router connection
    Serial.println("Mode: BRIDGE NODE\n");
    success = mesh.initAsBridge(
        MESH_PREFIX, MESH_PASSWORD,
        ROUTER_SSID, ROUTER_PASSWORD,
        &userScheduler, MESH_PORT
    );
    
    if (success) {
        Serial.println("✓ Bridge initialized successfully");
    } else {
        Serial.println("✗ Bridge init failed - falling back to regular mesh");
        mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
    }
#else
    // Initialize as regular mesh node
    Serial.println("Mode: REGULAR NODE\n");
    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
    success = true;
#endif
    
    // Enable sendToInternet API
    mesh.enableSendToInternet();
    
    Serial.println("================================================");
    Serial.printf("Node ID: %u\n", mesh.getNodeId());
    Serial.printf("Is Bridge: %s\n", mesh.isBridge() ? "YES" : "NO");
    Serial.println("================================================\n");
    
    // Schedule test run
    Serial.println("Tests will start in 10 seconds...\n");
    taskRunTests.setInterval(10000);  // Start after 10 seconds
    taskRunTests.enable();
}

// ============================================
// Main Loop
// ============================================

void loop() {
    mesh.update();
}
