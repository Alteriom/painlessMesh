//************************************************************
// RTC Integration Example
//
// This example demonstrates how to use RTC (Real-Time Clock)
// modules with painlessMesh for accurate offline timekeeping.
//
// When Internet/bridge is unavailable, nodes can still maintain
// accurate timestamps using a local RTC module.
//
// Supported RTC modules:
// - DS3231, DS1307 (I2C)
// - PCF8523, PCF8563 (I2C)
// - ESP32 internal RTC
//
// This example uses DS3231 with the RTClib library.
// Install: https://github.com/adafruit/RTClib
//************************************************************

#include "painlessMesh.h"
#include <RTClib.h>  // Adafruit RTClib for DS3231

#define MESH_PREFIX     "AlteriomMesh"
#define MESH_PASSWORD   "somethingSneaky"
#define MESH_PORT       5555

Scheduler userScheduler;
painlessMesh mesh;

// DS3231 RTC instance
RTC_DS3231 rtc;

// RTC Interface implementation for DS3231
class DS3231Interface : public painlessmesh::rtc::RTCInterface {
private:
  RTC_DS3231* rtcDevice;
  
public:
  DS3231Interface(RTC_DS3231* device) : rtcDevice(device) {}
  
  bool begin() override {
    if (!rtcDevice->begin()) {
      Serial.println("Couldn't find RTC");
      return false;
    }
    
    // Check if RTC lost power and needs time set
    if (rtcDevice->lostPower()) {
      Serial.println("RTC lost power, time needs to be set!");
      // Note: Time will be set via NTP when Internet available
    }
    
    return true;
  }
  
  bool isAvailable() override {
    return !rtcDevice->lostPower();
  }
  
  uint32_t getUnixTime() override {
    DateTime now = rtcDevice->now();
    return now.unixtime();
  }
  
  bool setUnixTime(uint32_t timestamp) override {
    rtcDevice->adjust(DateTime(timestamp));
    Serial.printf("RTC time set to: %u\n", timestamp);
    return true;
  }
  
  painlessmesh::rtc::RTCType getType() override {
    return painlessmesh::rtc::RTC_DS3231;
  }
};

// Create RTC interface instance
DS3231Interface rtcInterface(&rtc);

// Task to send timestamped sensor data
void sendSensorData();
Task taskSendData(30000, TASK_FOREVER, &sendSensorData);

// Flag to track if we need NTP sync
bool needsNTPSync = true;

void setup() {
  Serial.begin(115200);
  
  // Initialize mesh
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  
  // Set up mesh callbacks
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onBridgeStatusChanged(&bridgeStatusCallback);
  mesh.onRTCSyncComplete(&rtcSyncCompleteCallback);
  
  // Enable RTC
  if (mesh.enableRTC(&rtcInterface)) {
    Serial.println("RTC enabled successfully!");
    Serial.printf("RTC Type: %d\n", mesh.getRTCType());
    
    // Print current RTC time
    uint32_t rtcTime = mesh.getAccurateTime();
    Serial.printf("Current RTC time: %u\n", rtcTime);
  } else {
    Serial.println("Failed to enable RTC - will use mesh time only");
  }
  
  // Add sensor data task
  userScheduler.addTask(taskSendData);
  taskSendData.enable();
  
  Serial.println("Setup complete");
}

void loop() {
  mesh.update();
}

void sendSensorData() {
  // Get accurate timestamp (RTC if available, mesh time otherwise)
  uint32_t timestamp = mesh.getAccurateTime();
  
  // Simulate sensor reading
  float temperature = 25.5 + random(-50, 50) / 10.0;
  float humidity = 60.0 + random(-100, 100) / 10.0;
  
  // Create message with timestamp
  String msg = "{\"type\":\"sensor\",";
  msg += "\"nodeId\":" + String(mesh.getNodeId()) + ",";
  msg += "\"timestamp\":" + String(timestamp) + ",";
  msg += "\"temperature\":" + String(temperature, 1) + ",";
  msg += "\"humidity\":" + String(humidity, 1) + ",";
  msg += "\"hasRTC\":" + String(mesh.hasRTC() ? "true" : "false") + ",";
  msg += "\"rtcType\":" + String(mesh.getRTCType());
  msg += "}";
  
  mesh.sendBroadcast(msg);
  
  Serial.printf("Sent sensor data with timestamp: %u\n", timestamp);
  Serial.printf("Time since last RTC sync: %u ms\n", mesh.getTimeSinceRTCSync());
}

void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("Received from %u: %s\n", from, msg.c_str());
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("New Connection: %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
}

void bridgeStatusCallback(uint32_t bridgeNodeId, bool hasInternet) {
  Serial.printf("Bridge %u - Internet: %s\n", 
                bridgeNodeId, 
                hasInternet ? "Connected" : "Disconnected");
  
  // If THIS node is the bridge, set time authority based on Internet availability
  if (bridgeNodeId == mesh.getNodeId()) {
    if (hasInternet) {
      Serial.println("This node has Internet - setting time authority");
      mesh.setTimeAuthority(true);
    } else {
      // Lost Internet - remove time authority if no RTC
      if (!mesh.hasRTC()) {
        Serial.println("Lost Internet and no RTC - removing time authority");
        mesh.setTimeAuthority(false);
      }
    }
  }
  
  if (hasInternet && needsNTPSync && mesh.hasRTC()) {
    // Internet is available and we need to sync RTC
    // In a real application, you would get NTP time here
    // For this example, we'll use a placeholder
    Serial.println("Internet available - would sync RTC from NTP now");
    
    // Example: Get NTP time (you need to implement this)
    // uint32_t ntpTime = getNTPTime();
    // if (mesh.syncRTCFromNTP(ntpTime)) {
    //   Serial.println("RTC synced successfully!");
    //   needsNTPSync = false;
    // }
    
    // For demonstration purposes, let's sync to a known time
    uint32_t demoTime = 1704067200; // 2024-01-01 00:00:00 UTC
    if (mesh.syncRTCFromNTP(demoTime)) {
      Serial.println("RTC synced to demo time!");
      needsNTPSync = false;
    }
  } else if (!hasInternet) {
    // Internet lost - RTC will keep accurate time offline
    Serial.println("Internet offline - using RTC for timestamps");
  }
}

void rtcSyncCompleteCallback(uint32_t timestamp) {
  Serial.printf("RTC sync completed! New time: %u\n", timestamp);
  
  // Convert timestamp to human-readable format
  DateTime dt(timestamp);
  Serial.printf("Synced to: %d-%02d-%02d %02d:%02d:%02d\n",
                dt.year(), dt.month(), dt.day(),
                dt.hour(), dt.minute(), dt.second());
}
