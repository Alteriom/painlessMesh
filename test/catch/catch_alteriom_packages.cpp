#define CATCH_CONFIG_MAIN

#include "catch2/catch.hpp"

#include "Arduino.h"

#include "catch_utils.hpp"

#include "painlessmesh/plugin.hpp"
#include "../../examples/alteriom/alteriom_sensor_package.hpp"

using namespace painlessmesh;
using namespace alteriom;

logger::LogClass Log;

SCENARIO("Alteriom SensorPackage serialization works correctly") {
    GIVEN("A SensorPackage with test data") {
        auto pkg = SensorPackage();
        pkg.from = 12345;
        pkg.temperature = 23.5;
        pkg.humidity = 67.2;
        pkg.pressure = 1013.25;
        pkg.sensorId = 42;
        pkg.timestamp = 1609459200; // 2021-01-01 00:00:00 UTC
        pkg.batteryLevel = 85;
        
        REQUIRE(pkg.routing == router::BROADCAST);
        REQUIRE(pkg.type == 200);
        
        WHEN("Converting it to and from Variant") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<SensorPackage>();
            
            THEN("Should result in the same values") {
                REQUIRE(pkg2.from == pkg.from);
                REQUIRE(pkg2.temperature == pkg.temperature);
                REQUIRE(pkg2.humidity == pkg.humidity);
                REQUIRE(pkg2.pressure == pkg.pressure);
                REQUIRE(pkg2.sensorId == pkg.sensorId);
                REQUIRE(pkg2.timestamp == pkg.timestamp);
                REQUIRE(pkg2.batteryLevel == pkg.batteryLevel);
                REQUIRE(pkg2.routing == pkg.routing);
                REQUIRE(pkg2.type == pkg.type);
            }
        }
    }
}

SCENARIO("Alteriom CommandPackage serialization works correctly") {
    GIVEN("A CommandPackage with test data") {
        auto pkg = CommandPackage();
        pkg.from = 12345;
        pkg.dest = 67890;
        pkg.command = 5;
        pkg.targetDevice = 42;
        pkg.parameters = "{\"param1\":\"value1\",\"param2\":123}";
        pkg.commandId = 9876;
        
        REQUIRE(pkg.routing == router::SINGLE);
        REQUIRE(pkg.type == 400);
        
        WHEN("Converting it to and from Variant") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<CommandPackage>();
            
            THEN("Should result in the same values") {
                REQUIRE(pkg2.from == pkg.from);
                REQUIRE(pkg2.dest == pkg.dest);
                REQUIRE(pkg2.command == pkg.command);
                REQUIRE(pkg2.targetDevice == pkg.targetDevice);
                REQUIRE(pkg2.parameters == pkg.parameters);
                REQUIRE(pkg2.commandId == pkg.commandId);
                REQUIRE(pkg2.routing == pkg.routing);
                REQUIRE(pkg2.type == pkg.type);
            }
        }
    }
}

SCENARIO("Alteriom StatusPackage serialization works correctly") {
    GIVEN("A StatusPackage with test data") {
        auto pkg = StatusPackage();
        pkg.from = 12345;
        pkg.deviceStatus = 0x07; // Multiple status flags
        pkg.uptime = 86400; // 1 day in seconds
        pkg.freeMemory = 128; // 128KB
        pkg.wifiStrength = 75;
        pkg.firmwareVersion = "1.2.3-alteriom";
        
        REQUIRE(pkg.routing == router::BROADCAST);
        REQUIRE(pkg.type == 202);
        
        WHEN("Converting it to and from Variant") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<StatusPackage>();
            
            THEN("Should result in the same values") {
                REQUIRE(pkg2.from == pkg.from);
                REQUIRE(pkg2.deviceStatus == pkg.deviceStatus);
                REQUIRE(pkg2.uptime == pkg.uptime);
                REQUIRE(pkg2.freeMemory == pkg.freeMemory);
                REQUIRE(pkg2.wifiStrength == pkg.wifiStrength);
                REQUIRE(pkg2.firmwareVersion == pkg.firmwareVersion);
                REQUIRE(pkg2.routing == pkg.routing);
                REQUIRE(pkg2.type == pkg.type);
            }
        }
    }
}

SCENARIO("Alteriom StatusPackage organization fields serialize correctly") {
    GIVEN("A StatusPackage with organization metadata") {
        auto pkg = StatusPackage();
        pkg.from = 12345;
        pkg.deviceStatus = 0x03;
        pkg.uptime = 3600;
        pkg.freeMemory = 256;
        pkg.wifiStrength = 85;
        pkg.firmwareVersion = "1.8.0";
        
        // Set organization fields
        pkg.organizationId = "org123";
        pkg.customerId = "cust456";
        pkg.deviceGroup = "sensors";
        pkg.deviceName = "Test Sensor";
        pkg.deviceLocation = "Building A";
        pkg.deviceSecretSet = true;
        
        REQUIRE(pkg.routing == router::BROADCAST);
        REQUIRE(pkg.type == 202);
        
        WHEN("Converting it to and from Variant") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<StatusPackage>();
            
            THEN("Should result in the same values including organization fields") {
                REQUIRE(pkg2.from == pkg.from);
                REQUIRE(pkg2.deviceStatus == pkg.deviceStatus);
                REQUIRE(pkg2.uptime == pkg.uptime);
                REQUIRE(pkg2.freeMemory == pkg.freeMemory);
                REQUIRE(pkg2.wifiStrength == pkg.wifiStrength);
                REQUIRE(pkg2.firmwareVersion == pkg.firmwareVersion);
                
                // Verify organization fields
                REQUIRE(pkg2.organizationId == pkg.organizationId);
                REQUIRE(pkg2.customerId == pkg.customerId);
                REQUIRE(pkg2.deviceGroup == pkg.deviceGroup);
                REQUIRE(pkg2.deviceName == pkg.deviceName);
                REQUIRE(pkg2.deviceLocation == pkg.deviceLocation);
                REQUIRE(pkg2.deviceSecretSet == pkg.deviceSecretSet);
                
                REQUIRE(pkg2.routing == pkg.routing);
                REQUIRE(pkg2.type == pkg.type);
            }
        }
        
        WHEN("Only some organization fields are set") {
            auto pkg3 = StatusPackage();
            pkg3.from = 99999;
            pkg3.deviceName = "Partial Device";
            pkg3.deviceSecretSet = true;
            // Leave organizationId, customerId, deviceGroup, deviceLocation empty
            
            auto var3 = protocol::Variant(&pkg3);
            auto pkg4 = var3.to<StatusPackage>();
            
            THEN("Organization object should still be created and fields preserved") {
                REQUIRE(pkg4.deviceName == "Partial Device");
                REQUIRE(pkg4.deviceSecretSet == true);
                REQUIRE(pkg4.organizationId == "");
                REQUIRE(pkg4.customerId == "");
                REQUIRE(pkg4.deviceGroup == "");
                REQUIRE(pkg4.deviceLocation == "");
            }
        }
    }
}

SCENARIO("Alteriom packages can be used with PackageHandler") {
    GIVEN("A mock connection and package handler") {
        class MockConnection : public layout::Neighbour {
        public:
            bool addMessage(TSTRING msg) { return true; }
        };
        
        auto handler = plugin::PackageHandler<MockConnection>();
        bool sensorPackageReceived = false;
        bool commandPackageReceived = false;
        bool statusPackageReceived = false;
        
        // Setup handlers for each package type
        handler.onPackage(200, [&sensorPackageReceived](protocol::Variant& variant) {
            auto pkg = variant.to<SensorPackage>();
            sensorPackageReceived = true;
            REQUIRE(pkg.type == 200);
            return true;
        });
        
        handler.onPackage(400, [&commandPackageReceived](protocol::Variant& variant) {
            auto pkg = variant.to<CommandPackage>();
            commandPackageReceived = true;
            REQUIRE(pkg.type == 400);
            return true;
        });
        
        handler.onPackage(202, [&statusPackageReceived](protocol::Variant& variant) {
            auto pkg = variant.to<StatusPackage>();
            statusPackageReceived = true;
            REQUIRE(pkg.type == 202);
            return true;
        });
        
        WHEN("Creating different Alteriom packages") {
            // Create packages
            auto sensorPkg = SensorPackage();
            sensorPkg.temperature = 25.0;
            
            auto commandPkg = CommandPackage();
            commandPkg.command = 1;
            
            auto statusPkg = StatusPackage();
            statusPkg.uptime = 3600;
            
            // Test that sendPackage works without errors
            THEN("Packages can be sent without errors") {
                REQUIRE_NOTHROW(handler.sendPackage(&sensorPkg));
                REQUIRE_NOTHROW(handler.sendPackage(&commandPkg));  
                REQUIRE_NOTHROW(handler.sendPackage(&statusPkg));
            }
        }
    }
}

SCENARIO("Alteriom EnhancedStatusPackage serialization works correctly") {
    GIVEN("An EnhancedStatusPackage with comprehensive test data") {
        auto pkg = EnhancedStatusPackage();
        pkg.from = 12345;
        
        // Device Health
        pkg.deviceStatus = 0x07;
        pkg.uptime = 86400; // 1 day
        pkg.freeMemory = 128; // 128KB
        pkg.wifiStrength = 75;
        pkg.firmwareVersion = "1.2.3-alteriom";
        pkg.firmwareMD5 = "abc123def456";
        
        // Mesh Statistics
        pkg.nodeCount = 15;
        pkg.connectionCount = 4;
        pkg.messagesReceived = 1234;
        pkg.messagesSent = 5678;
        pkg.messagesDropped = 12;
        
        // Performance Metrics
        pkg.avgLatency = 45; // 45ms
        pkg.packetLossRate = 2; // 2%
        pkg.throughput = 1024; // 1KB/s
        
        // Alerts
        pkg.alertFlags = 0x03; // Some alerts active
        pkg.lastError = "Connection timeout";
        
        REQUIRE(pkg.routing == router::BROADCAST);
        REQUIRE(pkg.type == 604);  // Updated to 604 (MESH_STATUS) per mqtt-schema v0.7.2
        
        WHEN("Converting it to and from Variant") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<EnhancedStatusPackage>();
            
            THEN("Should result in the same values") {
                REQUIRE(pkg2.from == pkg.from);
                
                // Device Health
                REQUIRE(pkg2.deviceStatus == pkg.deviceStatus);
                REQUIRE(pkg2.uptime == pkg.uptime);
                REQUIRE(pkg2.freeMemory == pkg.freeMemory);
                REQUIRE(pkg2.wifiStrength == pkg.wifiStrength);
                REQUIRE(pkg2.firmwareVersion == pkg.firmwareVersion);
                REQUIRE(pkg2.firmwareMD5 == pkg.firmwareMD5);
                
                // Mesh Statistics
                REQUIRE(pkg2.nodeCount == pkg.nodeCount);
                REQUIRE(pkg2.connectionCount == pkg.connectionCount);
                REQUIRE(pkg2.messagesReceived == pkg.messagesReceived);
                REQUIRE(pkg2.messagesSent == pkg.messagesSent);
                REQUIRE(pkg2.messagesDropped == pkg.messagesDropped);
                
                // Performance Metrics
                REQUIRE(pkg2.avgLatency == pkg.avgLatency);
                REQUIRE(pkg2.packetLossRate == pkg.packetLossRate);
                REQUIRE(pkg2.throughput == pkg.throughput);
                
                // Alerts
                REQUIRE(pkg2.alertFlags == pkg.alertFlags);
                REQUIRE(pkg2.lastError == pkg.lastError);
                
                REQUIRE(pkg2.routing == pkg.routing);
                REQUIRE(pkg2.type == pkg.type);
            }
        }
    }
}

SCENARIO("Alteriom EnhancedStatusPackage with minimal data") {
    GIVEN("An EnhancedStatusPackage with only required fields") {
        auto pkg = EnhancedStatusPackage();
        pkg.from = 99999;
        pkg.uptime = 3600;
        pkg.freeMemory = 64;
        // Leave optional fields with default values
        
        WHEN("Converting it to and from Variant") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<EnhancedStatusPackage>();
            
            THEN("Default values should be preserved") {
                REQUIRE(pkg2.from == pkg.from);
                REQUIRE(pkg2.uptime == pkg.uptime);
                REQUIRE(pkg2.freeMemory == pkg.freeMemory);
                REQUIRE(pkg2.messagesReceived == 0);
                REQUIRE(pkg2.messagesSent == 0);
                REQUIRE(pkg2.messagesDropped == 0);
                REQUIRE(pkg2.avgLatency == 0);
                REQUIRE(pkg2.packetLossRate == 0);
                REQUIRE(pkg2.throughput == 0);
                REQUIRE(pkg2.alertFlags == 0);
                REQUIRE(pkg2.lastError == "");
                REQUIRE(pkg2.firmwareMD5 == "");
            }
        }
    }
}

SCENARIO("Alteriom packages handle edge cases correctly") {
    GIVEN("Packages with extreme values") {
        WHEN("SensorPackage has extreme temperature values") {
            auto pkg = SensorPackage();
            pkg.temperature = -40.0; // Extreme cold
            pkg.humidity = 100.0;    // Maximum humidity
            pkg.pressure = 0.0;      // No pressure (space?)
            
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<SensorPackage>();
            
            THEN("Values should be preserved correctly") {
                REQUIRE(pkg2.temperature == -40.0);
                REQUIRE(pkg2.humidity == 100.0);
                REQUIRE(pkg2.pressure == 0.0);
            }
        }
        
        WHEN("CommandPackage has empty parameters") {
            auto pkg = CommandPackage();
            pkg.parameters = "";
            
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<CommandPackage>();
            
            THEN("Empty string should be preserved") {
                REQUIRE(pkg2.parameters == "");
            }
        }
        
        WHEN("StatusPackage has maximum values") {
            auto pkg = StatusPackage();
            pkg.uptime = 0xFFFFFFFF;    // Maximum uptime
            pkg.freeMemory = 0xFFFF;    // Maximum memory
            pkg.deviceStatus = 0xFF;    // All status flags set
            
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<StatusPackage>();
            
            THEN("Maximum values should be preserved") {
                REQUIRE(pkg2.uptime == 0xFFFFFFFF);
                REQUIRE(pkg2.freeMemory == 0xFFFF);
                REQUIRE(pkg2.deviceStatus == 0xFF);
            }
        }
        
        WHEN("EnhancedStatusPackage has maximum metric values") {
            auto pkg = EnhancedStatusPackage();
            pkg.nodeCount = 0xFFFF;
            pkg.messagesReceived = 0xFFFFFFFF;
            pkg.messagesSent = 0xFFFFFFFF;
            pkg.messagesDropped = 0xFFFFFFFF;
            pkg.avgLatency = 0xFFFF;
            pkg.packetLossRate = 100; // 100%
            
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<EnhancedStatusPackage>();
            
            THEN("Maximum values should be preserved") {
                REQUIRE(pkg2.nodeCount == 0xFFFF);
                REQUIRE(pkg2.messagesReceived == 0xFFFFFFFF);
                REQUIRE(pkg2.messagesSent == 0xFFFFFFFF);
                REQUIRE(pkg2.messagesDropped == 0xFFFFFFFF);
                REQUIRE(pkg2.avgLatency == 0xFFFF);
                REQUIRE(pkg2.packetLossRate == 100);
            }
        }
    }
}

SCENARIO("StatusPackage sensor configuration serialization works correctly") {
    GIVEN("A StatusPackage with sensor configuration data") {
        auto pkg = StatusPackage();
        pkg.from = 12345;
        pkg.deviceStatus = 0x03;
        pkg.uptime = 3600;
        pkg.freeMemory = 256;
        pkg.wifiStrength = 85;
        pkg.firmwareVersion = "1.8.0";
        
        // Set sensor configuration
        pkg.sensorReadInterval = 30000;      // 30 seconds
        pkg.transmissionInterval = 60000;    // 60 seconds
        pkg.tempOffset = 0.5;
        pkg.humidityOffset = -1.0;
        pkg.pressureOffset = 2.0;
        
        REQUIRE(pkg.routing == router::BROADCAST);
        REQUIRE(pkg.type == 202);
        
        WHEN("Converting it to and from Variant") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<StatusPackage>();
            
            THEN("Should result in the same sensor configuration values") {
                REQUIRE(pkg2.from == pkg.from);
                REQUIRE(pkg2.deviceStatus == pkg.deviceStatus);
                REQUIRE(pkg2.sensorReadInterval == 30000);
                REQUIRE(pkg2.transmissionInterval == 60000);
                REQUIRE(pkg2.tempOffset == 0.5);
                REQUIRE(pkg2.humidityOffset == -1.0);
                REQUIRE(pkg2.pressureOffset == 2.0);
            }
        }
    }
}

SCENARIO("StatusPackage sensor inventory serialization works correctly") {
    GIVEN("A StatusPackage with sensor inventory data") {
        auto pkg = StatusPackage();
        pkg.from = 12345;
        pkg.deviceStatus = 0x03;
        pkg.uptime = 3600;
        
        // Set sensor inventory
        pkg.sensorCount = 3;
        pkg.sensorTypeMask = 7;  // 0b111 - three sensor types
        
        REQUIRE(pkg.routing == router::BROADCAST);
        REQUIRE(pkg.type == 202);
        
        WHEN("Converting it to and from Variant") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<StatusPackage>();
            
            THEN("Should result in the same sensor inventory values") {
                REQUIRE(pkg2.from == pkg.from);
                REQUIRE(pkg2.sensorCount == 3);
                REQUIRE(pkg2.sensorTypeMask == 7);
            }
        }
    }
}

SCENARIO("StatusPackage sensor config and inventory no collision") {
    GIVEN("A StatusPackage with both sensor config and inventory") {
        auto pkg = StatusPackage();
        pkg.from = 12345;
        pkg.deviceStatus = 0x03;
        pkg.uptime = 3600;
        pkg.freeMemory = 256;
        pkg.wifiStrength = 85;
        pkg.firmwareVersion = "1.8.0";
        
        // Set sensor configuration
        pkg.sensorReadInterval = 30000;
        pkg.transmissionInterval = 60000;
        pkg.tempOffset = 0.5;
        pkg.humidityOffset = -1.0;
        pkg.pressureOffset = 2.0;
        
        // Set sensor inventory
        pkg.sensorCount = 3;
        pkg.sensorTypeMask = 7;
        
        WHEN("Serializing to JSON") {
            // Create JSON document and serialize package
#if ARDUINOJSON_VERSION_MAJOR == 7
            JsonDocument jsonDoc;
#else
            DynamicJsonDocument jsonDoc(2048);
#endif
            JsonObject obj = jsonDoc.to<JsonObject>();
            pkg.addTo(std::move(obj));
            
            THEN("Should have separate 'sensors' and 'sensor_inventory' keys") {
                REQUIRE(obj["sensors"].is<JsonObject>());
                REQUIRE(obj["sensor_inventory"].is<JsonObject>());
                
                // Verify sensors config structure
                JsonObject sensors = obj["sensors"];
                REQUIRE(sensors["read_interval_ms"] == 30000);
                REQUIRE(sensors["read_interval_s"] == 30);
                REQUIRE(sensors["transmission_interval_ms"] == 60000);
                REQUIRE(sensors["transmission_interval_s"] == 60);
                REQUIRE(sensors["calibration"].is<JsonObject>());
                
                JsonObject calibration = sensors["calibration"];
                REQUIRE(calibration["temperature_offset"] == 0.5);
                REQUIRE(calibration["humidity_offset"] == -1.0);
                REQUIRE(calibration["pressure_offset"] == 2.0);
                
                // Verify sensor_inventory structure
                JsonObject sensorInventory = obj["sensor_inventory"];
                REQUIRE(sensorInventory["count"] == 3);
                REQUIRE(sensorInventory["type_mask"] == 7);
                
                // Verify no collision - sensors object should NOT have count/type_mask
                REQUIRE_FALSE(sensors["count"].is<uint8_t>());
                REQUIRE_FALSE(sensors["type_mask"].is<uint8_t>());
            }
        }
        
        WHEN("Converting it to and from Variant") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<StatusPackage>();
            
            THEN("Round-trip should preserve all values correctly") {
                // Verify sensor configuration preserved
                REQUIRE(pkg2.sensorReadInterval == 30000);
                REQUIRE(pkg2.transmissionInterval == 60000);
                REQUIRE(pkg2.tempOffset == 0.5);
                REQUIRE(pkg2.humidityOffset == -1.0);
                REQUIRE(pkg2.pressureOffset == 2.0);
                
                // Verify sensor inventory preserved
                REQUIRE(pkg2.sensorCount == 3);
                REQUIRE(pkg2.sensorTypeMask == 7);
                
                // Verify other fields preserved
                REQUIRE(pkg2.from == pkg.from);
                REQUIRE(pkg2.deviceStatus == pkg.deviceStatus);
                REQUIRE(pkg2.uptime == pkg.uptime);
            }
        }
    }
}

SCENARIO("StatusPackage sensor config without calibration") {
    GIVEN("A StatusPackage with intervals but no calibration offsets") {
        auto pkg = StatusPackage();
        pkg.from = 12345;
        
        // Set only intervals, leave offsets at 0
        pkg.sensorReadInterval = 30000;
        pkg.transmissionInterval = 60000;
        
        WHEN("Serializing to JSON") {
#if ARDUINOJSON_VERSION_MAJOR == 7
            JsonDocument jsonDoc;
#else
            DynamicJsonDocument jsonDoc(2048);
#endif
            JsonObject obj = jsonDoc.to<JsonObject>();
            pkg.addTo(std::move(obj));
            
            THEN("Should create sensors object but calibration should be minimal") {
                REQUIRE(obj["sensors"].is<JsonObject>());
                JsonObject sensors = obj["sensors"];
                REQUIRE(sensors["read_interval_ms"] == 30000);
                REQUIRE(sensors["transmission_interval_ms"] == 60000);
                
                // Calibration object should not be created when all offsets are 0
                REQUIRE_FALSE(sensors["calibration"].is<JsonObject>());
            }
        }
        
        WHEN("Converting it to and from Variant") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<StatusPackage>();
            
            THEN("Round-trip should work correctly") {
                REQUIRE(pkg2.sensorReadInterval == 30000);
                REQUIRE(pkg2.transmissionInterval == 60000);
                REQUIRE(pkg2.tempOffset == 0.0);
                REQUIRE(pkg2.humidityOffset == 0.0);
                REQUIRE(pkg2.pressureOffset == 0.0);
            }
        }
    }
}

SCENARIO("StatusPackage with only calibration offsets") {
    GIVEN("A StatusPackage with calibration but no intervals") {
        auto pkg = StatusPackage();
        pkg.from = 12345;
        
        // Set only calibration offsets, leave intervals at 0
        pkg.tempOffset = 1.5;
        pkg.humidityOffset = -2.5;
        pkg.pressureOffset = 0.3;
        
        WHEN("Serializing to JSON") {
#if ARDUINOJSON_VERSION_MAJOR == 7
            JsonDocument jsonDoc;
#else
            DynamicJsonDocument jsonDoc(2048);
#endif
            JsonObject obj = jsonDoc.to<JsonObject>();
            pkg.addTo(std::move(obj));
            
            THEN("Should create sensors object with calibration") {
                REQUIRE(obj["sensors"].is<JsonObject>());
                JsonObject sensors = obj["sensors"];
                REQUIRE(sensors["calibration"].is<JsonObject>());
                
                JsonObject calibration = sensors["calibration"];
                REQUIRE(calibration["temperature_offset"] == 1.5);
                REQUIRE(calibration["humidity_offset"] == -2.5);
                REQUIRE(calibration["pressure_offset"] == 0.3);
            }
        }
        
        WHEN("Converting it to and from Variant") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<StatusPackage>();
            
            THEN("Round-trip should work correctly") {
                REQUIRE(pkg2.tempOffset == 1.5);
                REQUIRE(pkg2.humidityOffset == -2.5);
                REQUIRE(pkg2.pressureOffset == 0.3);
                REQUIRE(pkg2.sensorReadInterval == 0);
                REQUIRE(pkg2.transmissionInterval == 0);
            }
        }
    }
}

SCENARIO("StatusPackage with no sensor data") {
    GIVEN("A StatusPackage with no sensor config or inventory") {
        auto pkg = StatusPackage();
        pkg.from = 12345;
        pkg.deviceStatus = 0x03;
        pkg.uptime = 3600;
        // Leave all sensor fields at default 0
        
        WHEN("Serializing to JSON") {
#if ARDUINOJSON_VERSION_MAJOR == 7
            JsonDocument jsonDoc;
#else
            DynamicJsonDocument jsonDoc(2048);
#endif
            JsonObject obj = jsonDoc.to<JsonObject>();
            pkg.addTo(std::move(obj));
            
            THEN("Should not create sensors or sensor_inventory objects") {
                REQUIRE_FALSE(obj["sensors"].is<JsonObject>());
                REQUIRE_FALSE(obj["sensor_inventory"].is<JsonObject>());
            }
        }
        
        WHEN("Converting it to and from Variant") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<StatusPackage>();
            
            THEN("Round-trip should work correctly with defaults") {
                REQUIRE(pkg2.sensorReadInterval == 0);
                REQUIRE(pkg2.transmissionInterval == 0);
                REQUIRE(pkg2.tempOffset == 0.0);
                REQUIRE(pkg2.humidityOffset == 0.0);
                REQUIRE(pkg2.pressureOffset == 0.0);
                REQUIRE(pkg2.sensorCount == 0);
                REQUIRE(pkg2.sensorTypeMask == 0);
            }
        }
    }
}

SCENARIO("StatusPackage time field naming convention consistency") {
    GIVEN("A StatusPackage with time-based configuration values") {
        auto pkg = StatusPackage();
        pkg.from = 12345;
        pkg.deviceStatus = 0x03;
        pkg.uptime = 3600;
        
        // Set time-based fields with values that are easily converted
        pkg.sensorReadInterval = 30000;      // 30 seconds in milliseconds
        pkg.transmissionInterval = 60000;    // 60 seconds in milliseconds
        
        WHEN("Serializing to JSON") {
#if ARDUINOJSON_VERSION_MAJOR == 7
            JsonDocument jsonDoc;
#else
            DynamicJsonDocument jsonDoc(2048);
#endif
            JsonObject obj = jsonDoc.to<JsonObject>();
            pkg.addTo(std::move(obj));
            
            THEN("All time fields should provide both _ms and _s variants") {
                REQUIRE(obj["sensors"].is<JsonObject>());
                JsonObject sensors = obj["sensors"];
                
                // Verify read_interval has both variants
                REQUIRE(sensors["read_interval_ms"].is<uint32_t>());
                REQUIRE(sensors["read_interval_s"].is<uint32_t>());
                REQUIRE(sensors["read_interval_ms"] == 30000);
                REQUIRE(sensors["read_interval_s"] == 30);
                
                // Verify transmission_interval has both variants
                REQUIRE(sensors["transmission_interval_ms"].is<uint32_t>());
                REQUIRE(sensors["transmission_interval_s"].is<uint32_t>());
                REQUIRE(sensors["transmission_interval_ms"] == 60000);
                REQUIRE(sensors["transmission_interval_s"] == 60);
            }
            
            THEN("Seconds variants should be correctly calculated from milliseconds") {
                JsonObject sensors = obj["sensors"];
                
                // Verify the conversion is correct (milliseconds / 1000 = seconds)
                uint32_t read_ms = sensors["read_interval_ms"];
                uint32_t read_s = sensors["read_interval_s"];
                REQUIRE(read_s == read_ms / 1000);
                
                uint32_t trans_ms = sensors["transmission_interval_ms"];
                uint32_t trans_s = sensors["transmission_interval_s"];
                REQUIRE(trans_s == trans_ms / 1000);
            }
        }
    }
}

SCENARIO("StatusPackage time field naming convention with various values") {
    GIVEN("Time fields with different magnitudes") {
        auto pkg = StatusPackage();
        pkg.from = 12345;
        
        WHEN("Using sub-second precision values") {
            pkg.sensorReadInterval = 500;        // 500ms (0.5 seconds)
            pkg.transmissionInterval = 250;      // 250ms (0.25 seconds)
            
#if ARDUINOJSON_VERSION_MAJOR == 7
            JsonDocument jsonDoc;
#else
            DynamicJsonDocument jsonDoc(2048);
#endif
            JsonObject obj = jsonDoc.to<JsonObject>();
            pkg.addTo(std::move(obj));
            
            THEN("Millisecond precision is preserved while seconds are truncated") {
                JsonObject sensors = obj["sensors"];
                REQUIRE(sensors["read_interval_ms"] == 500);
                REQUIRE(sensors["read_interval_s"] == 0);  // 500ms truncates to 0s
                REQUIRE(sensors["transmission_interval_ms"] == 250);
                REQUIRE(sensors["transmission_interval_s"] == 0);  // 250ms truncates to 0s
            }
        }
        
        WHEN("Using large time values") {
            pkg.sensorReadInterval = 3600000;    // 1 hour = 3600 seconds
            pkg.transmissionInterval = 86400000; // 1 day = 86400 seconds
            
#if ARDUINOJSON_VERSION_MAJOR == 7
            JsonDocument jsonDoc;
#else
            DynamicJsonDocument jsonDoc(2048);
#endif
            JsonObject obj = jsonDoc.to<JsonObject>();
            pkg.addTo(std::move(obj));
            
            THEN("Both milliseconds and seconds are correctly represented") {
                JsonObject sensors = obj["sensors"];
                REQUIRE(sensors["read_interval_ms"] == 3600000);
                REQUIRE(sensors["read_interval_s"] == 3600);
                REQUIRE(sensors["transmission_interval_ms"] == 86400000);
                REQUIRE(sensors["transmission_interval_s"] == 86400);
            }
        }
        
        WHEN("Using edge case values") {
            pkg.sensorReadInterval = 999;        // Just under 1 second
            pkg.transmissionInterval = 1000;     // Exactly 1 second
            
#if ARDUINOJSON_VERSION_MAJOR == 7
            JsonDocument jsonDoc;
#else
            DynamicJsonDocument jsonDoc(2048);
#endif
            JsonObject obj = jsonDoc.to<JsonObject>();
            pkg.addTo(std::move(obj));
            
            THEN("Edge case conversions are handled correctly") {
                JsonObject sensors = obj["sensors"];
                REQUIRE(sensors["read_interval_ms"] == 999);
                REQUIRE(sensors["read_interval_s"] == 0);    // 999ms truncates to 0s
                REQUIRE(sensors["transmission_interval_ms"] == 1000);
                REQUIRE(sensors["transmission_interval_s"] == 1);    // Exactly 1 second
            }
        }
    }
}

SCENARIO("StatusPackage time field deserialization backward compatibility") {
    GIVEN("JSON with time fields") {
        WHEN("JSON contains only _ms variants") {
#if ARDUINOJSON_VERSION_MAJOR == 7
            JsonDocument jsonDoc;
#else
            DynamicJsonDocument jsonDoc(2048);
#endif
            JsonObject obj = jsonDoc.to<JsonObject>();
            obj["from"] = 12345;
            obj["type"] = 202;
            
            JsonObject sensors = obj["sensors"].to<JsonObject>();
            sensors["read_interval_ms"] = 30000;
            sensors["transmission_interval_ms"] = 60000;
            // Intentionally omit _s variants
            
            auto pkg = StatusPackage(obj);
            
            THEN("Package should deserialize correctly using _ms values") {
                REQUIRE(pkg.from == 12345);
                REQUIRE(pkg.sensorReadInterval == 30000);
                REQUIRE(pkg.transmissionInterval == 60000);
            }
        }
        
        WHEN("JSON contains both _ms and _s variants") {
#if ARDUINOJSON_VERSION_MAJOR == 7
            JsonDocument jsonDoc;
#else
            DynamicJsonDocument jsonDoc(2048);
#endif
            JsonObject obj = jsonDoc.to<JsonObject>();
            obj["from"] = 12345;
            obj["type"] = 202;
            
            JsonObject sensors = obj["sensors"].to<JsonObject>();
            sensors["read_interval_ms"] = 30000;
            sensors["read_interval_s"] = 30;
            sensors["transmission_interval_ms"] = 60000;
            sensors["transmission_interval_s"] = 60;
            
            auto pkg = StatusPackage(obj);
            
            THEN("Package should deserialize using _ms values (milliseconds are source of truth)") {
                REQUIRE(pkg.from == 12345);
                REQUIRE(pkg.sensorReadInterval == 30000);
                REQUIRE(pkg.transmissionInterval == 60000);
            }
        }
    }
}

SCENARIO("StatusPackage time field naming convention documentation") {
    GIVEN("The StatusPackage class") {
        THEN("Time-based fields follow consistent naming pattern") {
            // This test serves as living documentation of the naming convention:
            //
            // CONVENTION FOR TIME-BASED FIELDS:
            // ---------------------------------
            // 1. Internal storage: Always use milliseconds (uint32_t)
            // 2. Field naming: Use descriptive names without unit suffix
            //    Examples: sensorReadInterval, transmissionInterval
            //
            // 3. JSON serialization: ALWAYS provide BOTH variants:
            //    - {fieldname}_ms: The value in milliseconds
            //    - {fieldname}_s: The value in seconds (milliseconds / 1000)
            //
            // 4. JSON deserialization: Read from _ms variant
            //    (This makes milliseconds the source of truth)
            //
            // BENEFITS:
            // - Consumer convenience: No mental overhead for unit conversion
            // - Flexibility: Consumers choose the unit that makes sense for their context
            // - Self-documenting: Field names clearly indicate available units
            // - Precision: Milliseconds preserved, seconds provided for readability
            //
            // EXAMPLE:
            // Internal: uint32_t sensorReadInterval = 30000;
            // JSON out: {"read_interval_ms": 30000, "read_interval_s": 30}
            // JSON in:  Reads from "read_interval_ms"
            
            // Verify the convention is followed in the existing code
            auto pkg = StatusPackage();
            pkg.sensorReadInterval = 45000;
            pkg.transmissionInterval = 90000;
            
#if ARDUINOJSON_VERSION_MAJOR == 7
            JsonDocument jsonDoc;
#else
            DynamicJsonDocument jsonDoc(2048);
#endif
            JsonObject obj = jsonDoc.to<JsonObject>();
            pkg.addTo(std::move(obj));
            
            JsonObject sensors = obj["sensors"];
            
            // Convention is correctly implemented
            REQUIRE(sensors["read_interval_ms"].is<uint32_t>());
            REQUIRE(sensors["read_interval_s"].is<uint32_t>());
            REQUIRE(sensors["transmission_interval_ms"].is<uint32_t>());
            REQUIRE(sensors["transmission_interval_s"].is<uint32_t>());
            
            // Values match the convention
            REQUIRE(sensors["read_interval_ms"] == 45000);
            REQUIRE(sensors["read_interval_s"] == 45);
            REQUIRE(sensors["transmission_interval_ms"] == 90000);
            REQUIRE(sensors["transmission_interval_s"] == 90);
        }
    }
}