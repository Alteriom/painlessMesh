#ifndef _PAINLESS_MESH_RTC_HPP_
#define _PAINLESS_MESH_RTC_HPP_

#include "Arduino.h"
#include "painlessmesh/logger.hpp"

extern painlessmesh::logger::LogClass Log;

namespace painlessmesh {
namespace rtc {

/**
 * RTC module types supported by painlessMesh
 */
enum RTCType {
  RTC_NONE = 0,
  RTC_DS3231 = 1,
  RTC_DS1307 = 2,
  RTC_PCF8523 = 3,
  RTC_PCF8563 = 4,
  RTC_ESP32_INTERNAL = 5
};

/**
 * Abstract RTC interface for painlessMesh
 * 
 * Users should implement this interface for their specific RTC hardware
 * and pass an instance to mesh.enableRTC()
 */
class RTCInterface {
 public:
  virtual ~RTCInterface() {}
  
  /**
   * Initialize the RTC hardware
   * 
   * @return true if initialization successful, false otherwise
   */
  virtual bool begin() = 0;
  
  /**
   * Check if RTC is available and responding
   * 
   * @return true if RTC is working, false otherwise
   */
  virtual bool isAvailable() = 0;
  
  /**
   * Get current Unix timestamp from RTC
   * 
   * @return Unix timestamp (seconds since 1970-01-01 00:00:00 UTC)
   */
  virtual uint32_t getUnixTime() = 0;
  
  /**
   * Set RTC time from Unix timestamp
   * 
   * @param timestamp Unix timestamp to set
   * @return true if successful, false otherwise
   */
  virtual bool setUnixTime(uint32_t timestamp) = 0;
  
  /**
   * Get RTC type identifier
   * 
   * @return RTCType enum value
   */
  virtual RTCType getType() = 0;
};

/**
 * RTC manager class for mesh integration
 * 
 * Handles RTC time synchronization and fallback to mesh time
 */
class RTCManager {
 public:
  RTCManager() : rtcInterface(nullptr), rtcEnabled(false), lastSyncTime(0) {}
  
  /**
   * Enable RTC with a user-provided interface
   * 
   * @param interface Pointer to RTCInterface implementation
   * @return true if RTC initialized successfully, false otherwise
   */
  bool enable(RTCInterface* interface) {
    if (!interface) {
      Log(logger::ERROR, "RTCManager::enable() - NULL interface provided\n");
      return false;
    }
    
    rtcInterface = interface;
    
    if (!rtcInterface->begin()) {
      Log(logger::ERROR, "RTCManager::enable() - RTC initialization failed\n");
      rtcInterface = nullptr;
      return false;
    }
    
    if (!rtcInterface->isAvailable()) {
      Log(logger::ERROR, "RTCManager::enable() - RTC not available\n");
      rtcInterface = nullptr;
      return false;
    }
    
    rtcEnabled = true;
    Log(logger::GENERAL, "RTCManager::enable() - RTC type %d enabled successfully\n",
        rtcInterface->getType());
    return true;
  }
  
  /**
   * Disable RTC
   */
  void disable() {
    rtcEnabled = false;
    rtcInterface = nullptr;
    Log(logger::GENERAL, "RTCManager::disable() - RTC disabled\n");
  }
  
  /**
   * Check if RTC is enabled and available
   * 
   * @return true if RTC can be used, false otherwise
   */
  bool isEnabled() const {
    return rtcEnabled && rtcInterface != nullptr && rtcInterface->isAvailable();
  }
  
  /**
   * Get current time from RTC
   * 
   * @return Unix timestamp, or 0 if RTC unavailable
   */
  uint32_t getTime() {
    if (!isEnabled()) {
      return 0;
    }
    return rtcInterface->getUnixTime();
  }
  
  /**
   * Sync RTC time from NTP/Internet source
   * 
   * @param ntpTimestamp Unix timestamp from NTP source
   * @return true if sync successful, false otherwise
   */
  bool syncFromNTP(uint32_t ntpTimestamp) {
    if (!isEnabled()) {
      Log(logger::ERROR, "RTCManager::syncFromNTP() - RTC not enabled\n");
      return false;
    }
    
    if (ntpTimestamp == 0) {
      Log(logger::ERROR, "RTCManager::syncFromNTP() - Invalid timestamp\n");
      return false;
    }
    
    if (!rtcInterface->setUnixTime(ntpTimestamp)) {
      Log(logger::ERROR, "RTCManager::syncFromNTP() - Failed to set RTC time\n");
      return false;
    }
    
    lastSyncTime = millis();
    Log(logger::GENERAL, "RTCManager::syncFromNTP() - RTC synced to %u\n", 
        ntpTimestamp);
    return true;
  }
  
  /**
   * Get time since last RTC sync
   * 
   * @return Milliseconds since last sync, or 0 if never synced
   */
  uint32_t getTimeSinceLastSync() const {
    if (lastSyncTime == 0) {
      return 0;
    }
    return millis() - lastSyncTime;
  }
  
  /**
   * Get RTC type
   * 
   * @return RTCType enum, or RTC_NONE if disabled
   */
  RTCType getType() const {
    if (!isEnabled()) {
      return RTC_NONE;
    }
    return rtcInterface->getType();
  }
  
 private:
  RTCInterface* rtcInterface;
  bool rtcEnabled;
  uint32_t lastSyncTime;
};

}  // namespace rtc
}  // namespace painlessmesh

#endif  // _PAINLESS_MESH_RTC_HPP_
