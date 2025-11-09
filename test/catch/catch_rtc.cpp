#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"
#include "Arduino.h"
#include "painlessmesh/rtc.hpp"

using namespace painlessmesh;
using namespace painlessmesh::rtc;

// Declare logger for test environment
painlessmesh::logger::LogClass Log;

/**
 * Mock RTC implementation for testing
 */
class MockRTC : public RTCInterface {
public:
  MockRTC() : initialized(false), available(true), currentTime(1609459200) {} // 2021-01-01 00:00:00
  
  bool begin() override {
    initialized = true;
    return true;
  }
  
  bool isAvailable() override {
    return available;
  }
  
  uint32_t getUnixTime() override {
    return currentTime;
  }
  
  bool setUnixTime(uint32_t timestamp) override {
    if (!available) return false;
    currentTime = timestamp;
    return true;
  }
  
  RTCType getType() override {
    return RTC_DS3231;
  }
  
  // Test helper methods
  void setAvailable(bool avail) { available = avail; }
  void simulateTimeAdvance(uint32_t seconds) { currentTime += seconds; }
  
private:
  bool initialized;
  bool available;
  uint32_t currentTime;
};

/**
 * Failing RTC implementation for testing error cases
 */
class FailingRTC : public RTCInterface {
public:
  bool begin() override { return false; }
  bool isAvailable() override { return false; }
  uint32_t getUnixTime() override { return 0; }
  bool setUnixTime(uint32_t timestamp) override { return false; }
  RTCType getType() override { return RTC_NONE; }
};

SCENARIO("RTCManager basic operations work correctly") {
  GIVEN("A fresh RTCManager") {
    RTCManager manager;
    
    REQUIRE(!manager.isEnabled());
    REQUIRE(manager.getType() == RTC_NONE);
    REQUIRE(manager.getTime() == 0);
    
    WHEN("Enabling RTC with valid interface") {
      MockRTC rtc;
      bool success = manager.enable(&rtc);
      
      THEN("RTC should be enabled and functional") {
        REQUIRE(success);
        REQUIRE(manager.isEnabled());
        REQUIRE(manager.getType() == RTC_DS3231);
        REQUIRE(manager.getTime() == 1609459200);
      }
    }
    
    WHEN("Enabling RTC with NULL interface") {
      bool success = manager.enable(nullptr);
      
      THEN("Enable should fail") {
        REQUIRE(!success);
        REQUIRE(!manager.isEnabled());
      }
    }
    
    WHEN("Enabling RTC that fails initialization") {
      FailingRTC rtc;
      bool success = manager.enable(&rtc);
      
      THEN("Enable should fail") {
        REQUIRE(!success);
        REQUIRE(!manager.isEnabled());
      }
    }
  }
}

SCENARIO("RTCManager time synchronization works correctly") {
  GIVEN("An enabled RTC") {
    RTCManager manager;
    MockRTC rtc;
    manager.enable(&rtc);
    
    REQUIRE(manager.isEnabled());
    REQUIRE(manager.getTime() == 1609459200);
    
    WHEN("Syncing from valid NTP timestamp") {
      uint32_t ntpTime = 1609545600; // 2021-01-02 00:00:00
      bool success = manager.syncFromNTP(ntpTime);
      
      THEN("RTC time should be updated") {
        REQUIRE(success);
        REQUIRE(manager.getTime() == ntpTime);
        REQUIRE(manager.getTimeSinceLastSync() >= 0);
      }
    }
    
    WHEN("Syncing with invalid timestamp (0)") {
      bool success = manager.syncFromNTP(0);
      
      THEN("Sync should fail and time unchanged") {
        REQUIRE(!success);
        REQUIRE(manager.getTime() == 1609459200);
      }
    }
    
    WHEN("RTC becomes unavailable during sync") {
      rtc.setAvailable(false);
      bool success = manager.syncFromNTP(1609545600);
      
      THEN("Sync should fail") {
        REQUIRE(!success);
      }
    }
  }
}

SCENARIO("RTCManager disable functionality works") {
  GIVEN("An enabled RTC") {
    RTCManager manager;
    MockRTC rtc;
    manager.enable(&rtc);
    
    REQUIRE(manager.isEnabled());
    
    WHEN("Disabling the RTC") {
      manager.disable();
      
      THEN("RTC should be disabled") {
        REQUIRE(!manager.isEnabled());
        REQUIRE(manager.getType() == RTC_NONE);
        REQUIRE(manager.getTime() == 0);
      }
    }
  }
}

SCENARIO("RTCManager handles multiple sync operations") {
  GIVEN("An enabled RTC") {
    RTCManager manager;
    MockRTC rtc;
    manager.enable(&rtc);
    
    WHEN("Syncing multiple times") {
      bool sync1 = manager.syncFromNTP(1609545600);
      bool sync2 = manager.syncFromNTP(1609632000); // Next day
      bool sync3 = manager.syncFromNTP(1609718400); // Another day
      
      THEN("All syncs should succeed and time should be correct") {
        REQUIRE(sync1);
        REQUIRE(sync2);
        REQUIRE(sync3);
        REQUIRE(manager.getTime() == 1609718400);
      }
    }
  }
}

SCENARIO("RTCInterface types are correctly identified") {
  GIVEN("RTCManagers with different RTC types") {
    class DS1307RTC : public RTCInterface {
    public:
      bool begin() override { return true; }
      bool isAvailable() override { return true; }
      uint32_t getUnixTime() override { return 1609459200; }
      bool setUnixTime(uint32_t timestamp) override { return true; }
      RTCType getType() override { return RTC_DS1307; }
    };
    
    class PCF8523RTC : public RTCInterface {
    public:
      bool begin() override { return true; }
      bool isAvailable() override { return true; }
      uint32_t getUnixTime() override { return 1609459200; }
      bool setUnixTime(uint32_t timestamp) override { return true; }
      RTCType getType() override { return RTC_PCF8523; }
    };
    
    RTCManager manager1, manager2, manager3;
    MockRTC ds3231;
    DS1307RTC ds1307;
    PCF8523RTC pcf8523;
    
    WHEN("Enabling different RTC types") {
      manager1.enable(&ds3231);
      manager2.enable(&ds1307);
      manager3.enable(&pcf8523);
      
      THEN("Types should be correctly identified") {
        REQUIRE(manager1.getType() == RTC_DS3231);
        REQUIRE(manager2.getType() == RTC_DS1307);
        REQUIRE(manager3.getType() == RTC_PCF8523);
      }
    }
  }
}

SCENARIO("RTCManager tracks time since last sync") {
  GIVEN("An enabled RTC that has never been synced") {
    RTCManager manager;
    MockRTC rtc;
    manager.enable(&rtc);
    
    WHEN("Checking time since last sync before any sync") {
      uint32_t timeSinceSync = manager.getTimeSinceLastSync();
      
      THEN("Should return 0") {
        REQUIRE(timeSinceSync == 0);
      }
    }
    
    WHEN("Syncing and checking time since sync") {
      manager.syncFromNTP(1609545600);
      uint32_t timeSinceSync = manager.getTimeSinceLastSync();
      
      THEN("Should return non-negative value") {
        REQUIRE(timeSinceSync >= 0);
      }
    }
  }
}
