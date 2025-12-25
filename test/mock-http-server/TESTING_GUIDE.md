# Using Mock HTTP Server in Tests

This guide shows how to use the mock HTTP server in your painlessMesh tests and examples.

## Quick Start

### 1. Start the Mock Server

```bash
# Terminal 1: Start mock server
cd test/mock-http-server
python3 server.py

# Or with Docker
docker-compose up mock-http-server
```

### 2. Configure Tests to Use Mock Server

In your test or example code:

```cpp
// Instead of real Internet endpoint
// #define API_ENDPOINT "https://api.callmebot.com/whatsapp.php"

// Use mock server
#define API_ENDPOINT "http://192.168.1.100:8080/whatsapp"
//                     ^^^^^^^^^^^^^^^^^^^
//                     Replace with your computer's IP address
```

### 3. Run Your Tests

```bash
# Upload to ESP32/ESP8266
pio run -t upload -t monitor

# Watch the mock server logs in Terminal 1
```

## Example: Testing sendToInternet()

### Setup Bridge Node

```cpp
#include "painlessMesh.h"

#define MESH_PREFIX     "TestMesh"
#define MESH_PASSWORD   "testpass"
#define ROUTER_SSID     "YourWiFi"
#define ROUTER_PASSWORD "wifipass"

// Point to mock server instead of real API
#define MOCK_SERVER_IP  "192.168.1.100"  // Your computer's IP
#define MOCK_SERVER_PORT 8080

painlessMesh mesh;
Scheduler userScheduler;

void setup() {
    Serial.begin(115200);
    
    // Initialize as bridge
    mesh.initAsBridge(MESH_PREFIX, MESH_PASSWORD,
                      ROUTER_SSID, ROUTER_PASSWORD,
                      &userScheduler, 5555);
    
    mesh.enableSendToInternet();
    
    // Test various scenarios
    testSuccessCase();
    testErrorCases();
    testTimeoutCase();
}

void testSuccessCase() {
    Serial.println("\n=== Testing Success Case ===");
    
    String url = String("http://") + MOCK_SERVER_IP + ":" + 
                 String(MOCK_SERVER_PORT) + "/status/200";
    
    mesh.sendToInternet(url, "", [](bool success, uint16_t status, String error) {
        if (success && status == 200) {
            Serial.println("✓ Success case PASSED");
        } else {
            Serial.printf("✗ Success case FAILED: %s (HTTP %d)\n", 
                         error.c_str(), status);
        }
    });
}

void testErrorCases() {
    Serial.println("\n=== Testing Error Cases ===");
    
    // Test HTTP 404
    String url404 = String("http://") + MOCK_SERVER_IP + ":" + 
                    String(MOCK_SERVER_PORT) + "/status/404";
    
    mesh.sendToInternet(url404, "", [](bool success, uint16_t status, String error) {
        if (!success && status == 404) {
            Serial.println("✓ 404 error case PASSED");
        } else {
            Serial.println("✗ 404 error case FAILED");
        }
    });
    
    // Test HTTP 203 (Non-Authoritative - should be treated as failure)
    String url203 = String("http://") + MOCK_SERVER_IP + ":" + 
                    String(MOCK_SERVER_PORT) + "/status/203";
    
    mesh.sendToInternet(url203, "", [](bool success, uint16_t status, String error) {
        if (!success && status == 203) {
            Serial.println("✓ HTTP 203 treated as failure PASSED");
        } else {
            Serial.println("✗ HTTP 203 case FAILED");
        }
    });
    
    // Test HTTP 500
    String url500 = String("http://") + MOCK_SERVER_IP + ":" + 
                    String(MOCK_SERVER_PORT) + "/status/500";
    
    mesh.sendToInternet(url500, "", [](bool success, uint16_t status, String error) {
        if (!success && status == 500) {
            Serial.println("✓ 500 error case PASSED");
        } else {
            Serial.println("✗ 500 error case FAILED");
        }
    });
}

void testTimeoutCase() {
    Serial.println("\n=== Testing Timeout Case ===");
    
    // This endpoint never responds - tests timeout handling
    String urlTimeout = String("http://") + MOCK_SERVER_IP + ":" + 
                        String(MOCK_SERVER_PORT) + "/timeout";
    
    mesh.sendToInternet(urlTimeout, "", [](bool success, uint16_t status, String error) {
        if (!success && error.indexOf("timeout") >= 0) {
            Serial.println("✓ Timeout case PASSED");
        } else {
            Serial.printf("✗ Timeout case FAILED: %s\n", error.c_str());
        }
    });
}

void loop() {
    mesh.update();
}
```

## Testing Specific Scenarios

### Test WhatsApp API Integration

```cpp
void testWhatsAppAPI() {
    String url = String("http://") + MOCK_SERVER_IP + ":" + 
                 String(MOCK_SERVER_PORT) + 
                 "/whatsapp?phone=%2B1234567890&apikey=test&text=Hello";
    
    mesh.sendToInternet(url, "", [](bool success, uint16_t status, String error) {
        if (success && status == 200) {
            Serial.println("✓ WhatsApp API simulation PASSED");
        } else {
            Serial.printf("✗ WhatsApp API simulation FAILED: %s\n", error.c_str());
        }
    });
}
```

### Test Delayed Responses

```cpp
void testDelayedResponse() {
    // Server will delay 5 seconds before responding
    String url = String("http://") + MOCK_SERVER_IP + ":" + 
                 String(MOCK_SERVER_PORT) + "/delay/5";
    
    unsigned long startTime = millis();
    
    mesh.sendToInternet(url, "", [startTime](bool success, uint16_t status, String error) {
        unsigned long elapsed = millis() - startTime;
        
        if (success && elapsed >= 5000) {
            Serial.printf("✓ Delayed response PASSED (waited %lums)\n", elapsed);
        } else {
            Serial.printf("✗ Delayed response FAILED (only waited %lums)\n", elapsed);
        }
    });
}
```

### Test Echo Endpoint

```cpp
void testEchoEndpoint() {
    String url = String("http://") + MOCK_SERVER_IP + ":" + 
                 String(MOCK_SERVER_PORT) + "/echo";
    
    String payload = "{\"sensor\":\"temp\",\"value\":25.5}";
    
    mesh.sendToInternet(url, payload, [payload](bool success, uint16_t status, String error) {
        if (success && status == 200) {
            Serial.println("✓ Echo endpoint PASSED");
            Serial.println("   Server echoed back our request");
        } else {
            Serial.println("✗ Echo endpoint FAILED");
        }
    });
}
```

## Finding Your Computer's IP Address

### Linux/Mac

```bash
ifconfig | grep "inet " | grep -v 127.0.0.1
```

### Windows

```cmd
ipconfig | findstr IPv4
```

### Important

- Make sure your ESP32/ESP8266 and computer are on the same network
- The mock server must be accessible from the ESP device
- Firewall may need to allow connections on port 8080

## Docker Setup

If running tests inside Docker containers:

```yaml
# docker-compose.yml
services:
  esp-simulator:
    build: .
    environment:
      - MOCK_HTTP_ENDPOINT=http://mock-http-server:8080
    depends_on:
      - mock-http-server
  
  mock-http-server:
    build: ./test/mock-http-server
    ports:
      - "8080:8080"
```

Then in your code:

```cpp
#ifdef DOCKER_ENV
  #define API_ENDPOINT "http://mock-http-server:8080"
#else
  #define API_ENDPOINT "http://192.168.1.100:8080"
#endif
```

## Automated Testing

### Using PlatformIO Test Framework

```cpp
// test/test_sendtointernet.cpp
#include <Arduino.h>
#include <unity.h>
#include "painlessMesh.h"

painlessMesh mesh;
Scheduler userScheduler;

void setUp(void) {
    mesh.init("TestMesh", "testpass", &userScheduler, 5555);
    mesh.enableSendToInternet();
}

void tearDown(void) {
    // Clean up
}

void test_http_200_success(void) {
    bool callbackCalled = false;
    bool testPassed = false;
    
    String url = String(TEST_HTTP_ENDPOINT) + "/status/200";
    
    mesh.sendToInternet(url, "", [&](bool success, uint16_t status, String error) {
        callbackCalled = true;
        testPassed = (success && status == 200);
    });
    
    // Wait for callback
    unsigned long start = millis();
    while (!callbackCalled && (millis() - start) < 10000) {
        mesh.update();
        delay(10);
    }
    
    TEST_ASSERT_TRUE(callbackCalled);
    TEST_ASSERT_TRUE(testPassed);
}

void test_http_404_failure(void) {
    bool callbackCalled = false;
    bool testPassed = false;
    
    String url = String(TEST_HTTP_ENDPOINT) + "/status/404";
    
    mesh.sendToInternet(url, "", [&](bool success, uint16_t status, String error) {
        callbackCalled = true;
        testPassed = (!success && status == 404);
    });
    
    unsigned long start = millis();
    while (!callbackCalled && (millis() - start) < 10000) {
        mesh.update();
        delay(10);
    }
    
    TEST_ASSERT_TRUE(callbackCalled);
    TEST_ASSERT_TRUE(testPassed);
}

void setup() {
    delay(2000);
    UNITY_BEGIN();
    RUN_TEST(test_http_200_success);
    RUN_TEST(test_http_404_failure);
    UNITY_END();
}

void loop() {
    // Tests run once in setup()
}
```

### Run Tests

```bash
# Set environment variable
export TEST_HTTP_ENDPOINT=http://192.168.1.100:8080

# Upload and run tests
pio test -e esp32
```

## Troubleshooting

### "Connection refused" Error

**Problem:** ESP can't reach mock server

**Solutions:**
1. Check your computer's IP address
2. Verify server is running: `curl http://localhost:8080/health`
3. Check firewall settings
4. Ensure ESP and computer are on same network

### "No route to host" Error

**Problem:** Network routing issue

**Solutions:**
1. Ping from ESP network: `ping 192.168.1.100`
2. Disable VPN if active
3. Check WiFi network isolation settings

### Tests Timing Out

**Problem:** Requests take too long

**Solutions:**
1. Check network latency
2. Increase timeout in sendToInternet() call
3. Use `/delay/0` endpoint to eliminate server delay
4. Check server logs for errors

## Best Practices

### 1. Use Environment-Specific Configuration

```cpp
#ifdef TESTING
  #define API_ENDPOINT "http://192.168.1.100:8080"
#else
  #define API_ENDPOINT "https://api.callmebot.com"
#endif
```

### 2. Log All Requests

```cpp
mesh.sendToInternet(url, payload, [url](bool success, uint16_t status, String error) {
    Serial.printf("Request to %s: %s (HTTP %d)\n", 
                  url.c_str(), 
                  success ? "SUCCESS" : "FAILED",
                  status);
});
```

### 3. Test Both Success and Failure Cases

Always test:
- ✅ Success cases (200, 201, 202, 204)
- ❌ Client errors (400, 404)
- ❌ Server errors (500, 503)
- ⏱️ Timeouts
- 🔄 Retries

### 4. Use Descriptive Test Names

```cpp
void test_whatsapp_api_valid_params_returns_200() { ... }
void test_whatsapp_api_missing_apikey_returns_400() { ... }
void test_whatsapp_api_timeout_triggers_retry() { ... }
```

## See Also

- [Mock Server README](README.md) - Server documentation
- [sendToInternet Example](../../examples/sendToInternet/) - Full example
- [Bridge Documentation](../../BRIDGE_TO_INTERNET.md) - Bridge setup guide
