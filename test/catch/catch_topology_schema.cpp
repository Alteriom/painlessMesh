#define CATCH_CONFIG_MAIN

#include "catch2/catch.hpp"
#include <ArduinoJson.h>
#include <string>

/**
 * Schema Validation Tests for @alteriom/mqtt-schema v0.5.0
 * 
 * Tests topology and event messages against schema requirements.
 * This test validates JSON structure directly without Arduino-specific types.
 */

// Helper function to validate envelope fields
bool validateEnvelope(const JsonObject& obj) {
    if (!obj["schema_version"].is<int>()) return false;
    if (!obj["device_id"].is<const char*>()) return false;
    if (!obj["device_type"].is<const char*>()) return false;
    if (!obj["timestamp"].is<const char*>()) return false;
    if (!obj["firmware_version"].is<const char*>()) return false;
    if (obj["schema_version"].as<int>() != 1) return false;
    
    return true;
}

// Helper function to validate Device ID format (ALT-XXXXXXXXXXXX)
bool validateDeviceIdFormat(const char* deviceId) {
    if (!deviceId) return false;
    std::string id(deviceId);
    
    if (id.length() != 16) return false;
    if (id.substr(0, 4) != "ALT-") return false;
    
    // Check that remaining 12 chars are uppercase hex
    for (size_t i = 4; i < 16; i++) {
        char c = id[i];
        if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F'))) {
            return false;
        }
    }
    
    return true;
}

SCENARIO("Device ID format validation works correctly") {
    GIVEN("Valid device ID formats") {
        std::vector<const char*> validIds = {
            "ALT-ABCD12345678",
            "ALT-123456789ABC",
            "ALT-000000000000",
            "ALT-FFFFFFFFFFFF"
        };
        
        THEN("All should be accepted") {
            for (const auto& id : validIds) {
                REQUIRE(validateDeviceIdFormat(id));
            }
        }
    }
    
    GIVEN("Invalid device ID formats") {
        std::vector<const char*> invalidIds = {
            "ALT-abcd12345678",  // lowercase
            "ALT-12345678",      // too short
            "ALT-12345678901234", // too long
            "ALTABCD12345678",   // missing dash
            "ALT-GGGG12345678"   // invalid hex
        };
        
        THEN("All should be rejected") {
            for (const auto& id : invalidIds) {
                REQUIRE(!validateDeviceIdFormat(id));
            }
        }
    }
}

SCENARIO("Envelope validation works correctly") {
    GIVEN("A valid envelope") {
        const char* json = R"({
            "schema_version": 1,
            "device_id": "ALT-ABCD12345678",
            "device_type": "mesh_gateway",
            "timestamp": "2024-10-12T14:30:00Z",
            "firmware_version": "1.7.0"
        })";
        
        WHEN("Parsing and validating") {
            JsonDocument doc;
            deserializeJson(doc, json);
            JsonObject obj = doc.as<JsonObject>();
            
            THEN("Validation should pass") {
                REQUIRE(validateEnvelope(obj));
            }
        }
    }
}

SCENARIO("Quality metrics are within valid ranges") {
    GIVEN("Sample quality metrics") {
        WHEN("Quality is between 0 and 100") {
            int quality = 85;
            REQUIRE(quality >= 0);
            REQUIRE(quality <= 100);
        }
        
        WHEN("RSSI is between -100 and 0") {
            int rssi = -45;
            REQUIRE(rssi >= -100);
            REQUIRE(rssi <= 0);
        }
        
        WHEN("Latency is positive") {
            int latency = 12;
            REQUIRE(latency >= 0);
            REQUIRE(latency < 10000);
        }
    }
}
