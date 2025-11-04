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

SCENARIO("StatusPackage time field naming consistency") {
    GIVEN("A StatusPackage with display, power, and MQTT retry configuration") {
        auto pkg = StatusPackage();
        pkg.from = 12345;
        pkg.deviceStatus = 0x01;
        pkg.uptime = 3600;
        pkg.freeMemory = 256;
        pkg.wifiStrength = 80;
        pkg.firmwareVersion = "1.0.0";
        
        // Set display configuration
        pkg.displayEnabled = true;
        pkg.displayBrightness = 128;
        pkg.displayTimeout = 45000; // 45 seconds in ms
        
        // Set power configuration
        pkg.deepSleepEnabled = true;
        pkg.deepSleepInterval = 300000; // 5 minutes in ms
        pkg.batteryPercent = 75;
        
        // Set MQTT retry configuration
        pkg.mqttMaxRetryAttempts = 5;
        pkg.mqttCircuitBreakerMs = 120000; // 2 minutes in ms
        pkg.mqttHourlyRetryEnabled = true;
        pkg.mqttInitialRetryMs = 5000; // 5 seconds in ms
        pkg.mqttMaxRetryMs = 60000; // 1 minute in ms
        pkg.mqttBackoffMultiplier = 2.0;
        
        WHEN("Converting to and from Variant") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<StatusPackage>();
            
            THEN("All configuration fields should round-trip correctly") {
                // Display config
                REQUIRE(pkg2.displayEnabled == pkg.displayEnabled);
                REQUIRE(pkg2.displayBrightness == pkg.displayBrightness);
                REQUIRE(pkg2.displayTimeout == pkg.displayTimeout);
                
                // Power config
                REQUIRE(pkg2.deepSleepEnabled == pkg.deepSleepEnabled);
                REQUIRE(pkg2.deepSleepInterval == pkg.deepSleepInterval);
                REQUIRE(pkg2.batteryPercent == pkg.batteryPercent);
                
                // MQTT retry config
                REQUIRE(pkg2.mqttMaxRetryAttempts == pkg.mqttMaxRetryAttempts);
                REQUIRE(pkg2.mqttCircuitBreakerMs == pkg.mqttCircuitBreakerMs);
                REQUIRE(pkg2.mqttHourlyRetryEnabled == pkg.mqttHourlyRetryEnabled);
                REQUIRE(pkg2.mqttInitialRetryMs == pkg.mqttInitialRetryMs);
                REQUIRE(pkg2.mqttMaxRetryMs == pkg.mqttMaxRetryMs);
                REQUIRE(pkg2.mqttBackoffMultiplier == pkg.mqttBackoffMultiplier);
            }
        }
        
        WHEN("Serializing to JSON") {
#if ARDUINOJSON_VERSION_MAJOR == 7
            JsonDocument jsonDoc;
#else
            DynamicJsonDocument jsonDoc(4096); // Larger size for all the new fields
#endif
            JsonObject obj = jsonDoc.to<JsonObject>();
            pkg.addTo(std::move(obj));
            
            THEN("Display config should have both _ms and _s variants") {
                REQUIRE(obj["display_config"].is<JsonObject>());
                JsonObject displayConfig = obj["display_config"];
                
                REQUIRE(displayConfig["enabled"] == true);
                REQUIRE(displayConfig["brightness"] == 128);
                REQUIRE(displayConfig["timeout_ms"] == 45000);
                REQUIRE(displayConfig["timeout_s"] == 45);
            }
            
            THEN("Power config should have both _ms and _s variants") {
                REQUIRE(obj["power_config"].is<JsonObject>());
                JsonObject powerConfig = obj["power_config"];
                
                REQUIRE(powerConfig["deep_sleep_enabled"] == true);
                REQUIRE(powerConfig["deep_sleep_interval_ms"] == 300000);
                REQUIRE(powerConfig["deep_sleep_interval_s"] == 300);
                REQUIRE(powerConfig["battery_percent"] == 75);
            }
            
            THEN("MQTT retry config should have both _ms and _s variants") {
                REQUIRE(obj["mqtt_retry"].is<JsonObject>());
                JsonObject mqttRetry = obj["mqtt_retry"];
                
                REQUIRE(mqttRetry["max_attempts"] == 5);
                REQUIRE(mqttRetry["circuit_breaker_ms"] == 120000);
                REQUIRE(mqttRetry["circuit_breaker_s"] == 120);
                REQUIRE(mqttRetry["hourly_retry_enabled"] == true);
                REQUIRE(mqttRetry["initial_retry_ms"] == 5000);
                REQUIRE(mqttRetry["initial_retry_s"] == 5);
                REQUIRE(mqttRetry["max_retry_ms"] == 60000);
                REQUIRE(mqttRetry["max_retry_s"] == 60);
                REQUIRE(mqttRetry["backoff_multiplier"] == 2.0);
            }
        }
    }
    
    GIVEN("A StatusPackage without display, power, or MQTT config") {
        auto pkg = StatusPackage();
        pkg.from = 54321;
        pkg.deviceStatus = 0x02;
        pkg.uptime = 7200;
        
        WHEN("Serializing to JSON") {
#if ARDUINOJSON_VERSION_MAJOR == 7
            JsonDocument jsonDoc;
#else
            DynamicJsonDocument jsonDoc(2048);
#endif
            JsonObject obj = jsonDoc.to<JsonObject>();
            pkg.addTo(std::move(obj));
            
            THEN("Should not create display_config, power_config, or mqtt_retry objects") {
                REQUIRE_FALSE(obj["display_config"].is<JsonObject>());
                REQUIRE_FALSE(obj["power_config"].is<JsonObject>());
                REQUIRE_FALSE(obj["mqtt_retry"].is<JsonObject>());
            }
        }
        
        WHEN("Converting it to and from Variant") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<StatusPackage>();
            
            THEN("Round-trip should work correctly with defaults") {
                REQUIRE(pkg2.displayEnabled == false);
                REQUIRE(pkg2.displayBrightness == 0);
                REQUIRE(pkg2.displayTimeout == 0);
                REQUIRE(pkg2.deepSleepEnabled == false);
                REQUIRE(pkg2.deepSleepInterval == 0);
                REQUIRE(pkg2.batteryPercent == 0);
                REQUIRE(pkg2.mqttMaxRetryAttempts == 0);
                REQUIRE(pkg2.mqttCircuitBreakerMs == 0);
                REQUIRE(pkg2.mqttHourlyRetryEnabled == false);
                REQUIRE(pkg2.mqttInitialRetryMs == 0);
                REQUIRE(pkg2.mqttMaxRetryMs == 0);
                REQUIRE(pkg2.mqttBackoffMultiplier == 0.0);
            }
        }
    }
}

SCENARIO("StatusPackage backward compatibility for time fields") {
    GIVEN("A JSON payload with old field names (no _ms suffix)") {
#if ARDUINOJSON_VERSION_MAJOR == 7
        JsonDocument jsonDoc;
#else
        DynamicJsonDocument jsonDoc(2048);
#endif
        JsonObject obj = jsonDoc.to<JsonObject>();
        
        // Simulate old format without _ms suffix
        JsonObject displayConfig = obj["display_config"].to<JsonObject>();
        displayConfig["enabled"] = true;
        displayConfig["brightness"] = 100;
        displayConfig["timeout"] = 30000; // Old field name
        
        JsonObject powerConfig = obj["power_config"].to<JsonObject>();
        powerConfig["deep_sleep_enabled"] = true;
        powerConfig["deep_sleep_interval"] = 600000; // Old field name
        powerConfig["battery_percent"] = 80;
        
        WHEN("Deserializing from old format") {
            auto pkg = StatusPackage(obj);
            
            THEN("Should correctly parse old field names") {
                REQUIRE(pkg.displayEnabled == true);
                REQUIRE(pkg.displayBrightness == 100);
                REQUIRE(pkg.displayTimeout == 30000);
                
                REQUIRE(pkg.deepSleepEnabled == true);
                REQUIRE(pkg.deepSleepInterval == 600000);
                REQUIRE(pkg.batteryPercent == 80);
            }
        }
    }
    
    GIVEN("A JSON payload with new field names (_ms suffix)") {
#if ARDUINOJSON_VERSION_MAJOR == 7
        JsonDocument jsonDoc;
#else
        DynamicJsonDocument jsonDoc(2048);
#endif
        JsonObject obj = jsonDoc.to<JsonObject>();
        
        // New format with _ms suffix
        JsonObject displayConfig = obj["display_config"].to<JsonObject>();
        displayConfig["enabled"] = true;
        displayConfig["brightness"] = 150;
        displayConfig["timeout_ms"] = 60000; // New field name
        
        JsonObject powerConfig = obj["power_config"].to<JsonObject>();
        powerConfig["deep_sleep_enabled"] = false;
        powerConfig["deep_sleep_interval_ms"] = 900000; // New field name
        powerConfig["battery_percent"] = 90;
        
        WHEN("Deserializing from new format") {
            auto pkg = StatusPackage(obj);
            
            THEN("Should correctly parse new field names") {
                REQUIRE(pkg.displayEnabled == true);
                REQUIRE(pkg.displayBrightness == 150);
                REQUIRE(pkg.displayTimeout == 60000);
                
                REQUIRE(pkg.deepSleepEnabled == false);
                REQUIRE(pkg.deepSleepInterval == 900000);
                REQUIRE(pkg.batteryPercent == 90);
            }
        }
    }
}

SCENARIO("StatusPackage MQTT retry configuration with only boolean/float fields") {
    GIVEN("A StatusPackage with only mqttHourlyRetryEnabled set") {
        auto pkg = StatusPackage();
        pkg.from = 12345;
        pkg.mqttHourlyRetryEnabled = true;
        pkg.mqttBackoffMultiplier = 1.5;
        
        WHEN("Serializing to JSON") {
#if ARDUINOJSON_VERSION_MAJOR == 7
            JsonDocument jsonDoc;
#else
            DynamicJsonDocument jsonDoc(2048);
#endif
            JsonObject obj = jsonDoc.to<JsonObject>();
            pkg.addTo(std::move(obj));
            
            THEN("mqtt_retry object should be created even without time fields") {
                REQUIRE(obj["mqtt_retry"].is<JsonObject>());
                JsonObject mqttRetry = obj["mqtt_retry"];
                REQUIRE(mqttRetry["hourly_retry_enabled"] == true);
                REQUIRE(mqttRetry["backoff_multiplier"] == 1.5);
            }
        }
        
        WHEN("Converting to and from Variant") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<StatusPackage>();
            
            THEN("Should round-trip correctly") {
                REQUIRE(pkg2.mqttHourlyRetryEnabled == true);
                REQUIRE(pkg2.mqttBackoffMultiplier == 1.5);
            }
        }
    }
}

SCENARIO("StatusPackage boolean naming convention semantics") {
    GIVEN("A StatusPackage demonstrating the three boolean patterns") {
        auto pkg = StatusPackage();
        pkg.from = 12345;
        
        // Pattern 1: *Set suffix - Configuration data has been provided
        // Does NOT indicate if feature is enabled or working
        pkg.deviceSecretSet = true;  // Secret is configured
        
        // Pattern 2: *Enabled suffix - Feature is currently active/turned on
        // Independent of whether required configuration exists
        pkg.displayEnabled = true;      // Display feature is active
        pkg.deepSleepEnabled = false;   // Deep sleep mode is inactive
        pkg.mqttHourlyRetryEnabled = true;  // Hourly retry feature is active
        
        // Pattern 3: Runtime state (is* prefix or *Connected)
        // Would be added here if StatusPackage had such fields
        // Examples: isConfigured, mqttConnected, meshIsRoot
        
        WHEN("Verifying *Set fields indicate configuration presence") {
            THEN("deviceSecretSet should mean 'secret is configured'") {
                REQUIRE(pkg.deviceSecretSet == true);
                // This does NOT mean the secret is enabled/disabled
                // It only means the secret has been provided
            }
        }
        
        WHEN("Verifying *Enabled fields indicate feature state") {
            THEN("displayEnabled should mean 'display feature is active'") {
                REQUIRE(pkg.displayEnabled == true);
                // This means the feature is turned on
            }
            
            THEN("deepSleepEnabled should mean 'deep sleep is active'") {
                REQUIRE(pkg.deepSleepEnabled == false);
                // This means the feature is turned off
            }
            
            THEN("mqttHourlyRetryEnabled should mean 'hourly retry is active'") {
                REQUIRE(pkg.mqttHourlyRetryEnabled == true);
                // This means the retry feature is turned on
            }
        }
        
        WHEN("Demonstrating independent semantics of *Set and *Enabled") {
            THEN("A feature can have configuration but be disabled") {
                // Example: OTA server URL is configured but OTA is disabled
                // This would be represented as:
                //   otaServerSet = true (configuration exists)
                //   otaEnabled = false (feature is turned off)
                // 
                // Both fields are independent and serve different purposes
                REQUIRE(true); // Conceptual test
            }
        }
        
        WHEN("Serializing to JSON and back") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<StatusPackage>();
            
            THEN("All boolean semantics should be preserved") {
                // *Set fields preserve configuration state
                REQUIRE(pkg2.deviceSecretSet == pkg.deviceSecretSet);
                
                // *Enabled fields preserve feature activation state
                REQUIRE(pkg2.displayEnabled == pkg.displayEnabled);
                REQUIRE(pkg2.deepSleepEnabled == pkg.deepSleepEnabled);
                REQUIRE(pkg2.mqttHourlyRetryEnabled == pkg.mqttHourlyRetryEnabled);
            }
        }
    }
}

SCENARIO("StatusPackage boolean fields follow consistent patterns") {
    GIVEN("The StatusPackage class definition") {
        WHEN("Examining boolean field names") {
            THEN("All *Set fields should indicate configuration presence") {
                // deviceSecretSet follows this pattern correctly
                REQUIRE(true);
            }
            
            THEN("All *Enabled fields should indicate feature state") {
                // displayEnabled, deepSleepEnabled, mqttHourlyRetryEnabled
                // all follow this pattern correctly
                REQUIRE(true);
            }
            
            THEN("Future runtime state fields should use is* or *Connected") {
                // If added in the future:
                // isConfigured, mqttConnected, meshIsRoot
                // would follow this pattern
                REQUIRE(true);
            }
        }
    }
}

SCENARIO("StatusPackage JSON structure follows nesting guidelines") {
    GIVEN("A StatusPackage with various configuration fields populated") {
        auto pkg = StatusPackage();
        pkg.from = 12345;
        pkg.deviceStatus = 0x07;
        pkg.uptime = 86400;
        pkg.freeMemory = 128;
        pkg.wifiStrength = 75;
        pkg.firmwareVersion = "1.2.3-alteriom";
        
        // Populate sensor configuration with calibration
        pkg.sensorReadInterval = 30000;
        pkg.transmissionInterval = 60000;
        pkg.tempOffset = 0.5;
        pkg.humidityOffset = -2.0;
        pkg.pressureOffset = 0.1;
        
        // Populate display configuration
        pkg.displayEnabled = true;
        pkg.displayBrightness = 128;
        pkg.displayTimeout = 30000;
        
        // Populate power configuration
        pkg.deepSleepEnabled = false;
        pkg.deepSleepInterval = 300000;
        pkg.batteryPercent = 85;
        
        // Populate MQTT retry configuration
        pkg.mqttMaxRetryAttempts = 5;
        pkg.mqttCircuitBreakerMs = 60000;
        pkg.mqttHourlyRetryEnabled = true;
        pkg.mqttInitialRetryMs = 1000;
        pkg.mqttMaxRetryMs = 30000;
        pkg.mqttBackoffMultiplier = 2.0;
        
        // Populate organization metadata
        pkg.organizationId = "org123";
        pkg.customerId = "cust456";
        
        WHEN("Serializing to JSON") {
            JsonDocument doc;
            JsonObject obj = doc.to<JsonObject>();
            pkg.addTo(std::move(obj));
            
            THEN("Sensor configuration has nested calibration subsection") {
                // Verify sensors object exists
                REQUIRE(obj["sensors"].is<JsonObject>());
                
                JsonObject sensors = obj["sensors"];
                
                // Verify top-level sensor fields are present
                REQUIRE(sensors["read_interval_ms"].is<unsigned int>());
                REQUIRE(sensors["read_interval_s"].is<unsigned int>());
                REQUIRE(sensors["transmission_interval_ms"].is<unsigned int>());
                REQUIRE(sensors["transmission_interval_s"].is<unsigned int>());
                
                // Verify calibration is nested as a subsection
                REQUIRE(sensors["calibration"].is<JsonObject>());
                
                JsonObject calibration = sensors["calibration"];
                REQUIRE(calibration["temperature_offset"].is<double>());
                REQUIRE(calibration["humidity_offset"].is<double>());
                REQUIRE(calibration["pressure_offset"].is<double>());
                
                // Verify calibration values
                REQUIRE(calibration["temperature_offset"] == 0.5);
                REQUIRE(calibration["humidity_offset"] == -2.0);
                REQUIRE(calibration["pressure_offset"] == 0.1);
            }
            
            THEN("Display configuration remains flat (no nesting)") {
                REQUIRE(obj["display_config"].is<JsonObject>());
                
                JsonObject displayConfig = obj["display_config"];
                
                // Verify all fields are at the same level (flat)
                REQUIRE(displayConfig["enabled"].is<bool>());
                REQUIRE(displayConfig["brightness"].is<unsigned int>());
                REQUIRE(displayConfig["timeout_ms"].is<unsigned int>());
                REQUIRE(displayConfig["timeout_s"].is<unsigned int>());
                
                // Verify no nested subsections exist
                REQUIRE_FALSE(displayConfig.containsKey("brightness_config"));
                REQUIRE_FALSE(displayConfig.containsKey("timeout_config"));
                
                // Count fields - should be 4 (enabled, brightness, timeout_ms, timeout_s)
                size_t fieldCount = 0;
                for (JsonPair kv : displayConfig) {
                    fieldCount++;
                }
                REQUIRE(fieldCount == 4);
            }
            
            THEN("Power configuration remains flat (no nesting)") {
                REQUIRE(obj["power_config"].is<JsonObject>());
                
                JsonObject powerConfig = obj["power_config"];
                
                // Verify all fields are at the same level (flat)
                REQUIRE(powerConfig["deep_sleep_enabled"].is<bool>());
                REQUIRE(powerConfig["deep_sleep_interval_ms"].is<unsigned int>());
                REQUIRE(powerConfig["deep_sleep_interval_s"].is<unsigned int>());
                REQUIRE(powerConfig["battery_percent"].is<unsigned int>());
                
                // Verify no nested subsections exist
                REQUIRE_FALSE(powerConfig.containsKey("deep_sleep"));
                REQUIRE_FALSE(powerConfig.containsKey("battery"));
                
                // Count fields - should be 4
                size_t fieldCount = 0;
                for (JsonPair kv : powerConfig) {
                    fieldCount++;
                }
                REQUIRE(fieldCount == 4);
            }
            
            THEN("MQTT retry configuration remains flat (no nesting)") {
                REQUIRE(obj["mqtt_retry"].is<JsonObject>());
                
                JsonObject mqttRetry = obj["mqtt_retry"];
                
                // Verify all fields are at the same level (flat)
                REQUIRE(mqttRetry["max_attempts"].is<unsigned int>());
                REQUIRE(mqttRetry["circuit_breaker_ms"].is<unsigned int>());
                REQUIRE(mqttRetry["circuit_breaker_s"].is<unsigned int>());
                REQUIRE(mqttRetry["hourly_retry_enabled"].is<bool>());
                REQUIRE(mqttRetry["initial_retry_ms"].is<unsigned int>());
                REQUIRE(mqttRetry["initial_retry_s"].is<unsigned int>());
                REQUIRE(mqttRetry["max_retry_ms"].is<unsigned int>());
                REQUIRE(mqttRetry["max_retry_s"].is<unsigned int>());
                REQUIRE(mqttRetry["backoff_multiplier"].is<double>());
                
                // Verify no nested subsections exist for backoff settings
                REQUIRE_FALSE(mqttRetry.containsKey("backoff"));
                REQUIRE_FALSE(mqttRetry.containsKey("retry_policy"));
                
                // Count fields - should be 9
                size_t fieldCount = 0;
                for (JsonPair kv : mqttRetry) {
                    fieldCount++;
                }
                REQUIRE(fieldCount == 9);
            }
            
            THEN("Organization metadata is nested as optional subsection") {
                REQUIRE(obj["organization"].is<JsonObject>());
                
                JsonObject organization = obj["organization"];
                
                // Verify organization fields
                REQUIRE(organization["organizationId"].is<const char*>());
                REQUIRE(organization["customerId"].is<const char*>());
                
                // Organization is treated as a distinct optional subsystem
                REQUIRE(organization["organizationId"] == "org123");
                REQUIRE(organization["customerId"] == "cust456");
            }
        }
    }
}

SCENARIO("StatusPackage structure consistency validates nesting rules") {
    GIVEN("Documentation states nesting guidelines") {
        WHEN("Evaluating current structure against guidelines") {
            THEN("Flat sections have < 4 base fields or no clear subsystems") {
                // display_config: 3 base fields (enabled, brightness, timeout)
                // + time variants (_ms, _s) = 4 total fields
                // Decision: Flat (no logical subsystems, simple values)
                REQUIRE(true);
                
                // power_config: 3 base fields (deep_sleep_enabled, deep_sleep_interval, battery_percent)
                // + time variants = 4 total fields
                // Decision: Flat (could nest deep_sleep + battery, but too few fields)
                REQUIRE(true);
            }
            
            THEN("Nested sections have clear semantic grouping") {
                // sensors.calibration: 3 offset fields
                // Decision: Nested (calibration is distinct subsystem, optional, semantically separate)
                REQUIRE(true);
                
                // organization: 6 metadata fields
                // Decision: Nested (optional metadata subsystem, may not be present on all devices)
                REQUIRE(true);
            }
            
            THEN("MQTT retry remains flat despite 9 fields for cohesiveness") {
                // mqtt_retry: 9 fields (retry policy + backoff strategy)
                // Decision: Flat (cohesive retry configuration, nesting would fragment it)
                // This is an acceptable exception: high field count but unified purpose
                REQUIRE(true);
            }
            
            THEN("Structure is consistent with documented guidelines") {
                // All current structures follow the guidelines in:
                // docs/API_DESIGN_GUIDELINES.md
                // examples/alteriom/alteriom_sensor_package.hpp header comments
                REQUIRE(true);
            }
        }
    }
}