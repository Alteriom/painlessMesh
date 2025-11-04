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