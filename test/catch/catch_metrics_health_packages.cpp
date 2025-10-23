#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"
#include "Arduino.h"
#include "catch_utils.hpp"
#include "alteriom/alteriom_sensor_package.hpp"

using namespace alteriom;
using namespace painlessmesh;

SCENARIO("MetricsPackage serialization works correctly") {
    GIVEN("A MetricsPackage with comprehensive metrics") {
        auto pkg = MetricsPackage();
        pkg.from = 12345;
        
        // CPU and Processing
        pkg.cpuUsage = 45;
        pkg.loopIterations = 10000;
        pkg.taskQueueSize = 5;
        
        // Memory Metrics
        pkg.freeHeap = 100000;
        pkg.minFreeHeap = 95000;
        pkg.heapFragmentation = 15;
        pkg.maxAllocHeap = 80000;
        
        // Network Performance
        pkg.bytesReceived = 1500000;
        pkg.bytesSent = 1200000;
        pkg.packetsReceived = 5000;
        pkg.packetsSent = 4500;
        pkg.packetsDropped = 10;
        pkg.currentThroughput = 8192;
        
        // Timing and Latency
        pkg.avgResponseTime = 150000;
        pkg.maxResponseTime = 500000;
        pkg.avgMeshLatency = 25;
        
        // Connection Quality
        pkg.connectionQuality = 85;
        pkg.wifiRSSI = -55;
        
        // Metadata
        pkg.collectionTimestamp = 1234567890;
        pkg.collectionInterval = 60000;
        
        REQUIRE(pkg.type == 204);
        
        WHEN("Converting it to and from Variant") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<MetricsPackage>();
            
            THEN("Should result in the same values") {
                REQUIRE(pkg2.from == pkg.from);
                REQUIRE(pkg2.type == pkg.type);
                
                // CPU and Processing
                REQUIRE(pkg2.cpuUsage == pkg.cpuUsage);
                REQUIRE(pkg2.loopIterations == pkg.loopIterations);
                REQUIRE(pkg2.taskQueueSize == pkg.taskQueueSize);
                
                // Memory Metrics
                REQUIRE(pkg2.freeHeap == pkg.freeHeap);
                REQUIRE(pkg2.minFreeHeap == pkg.minFreeHeap);
                REQUIRE(pkg2.heapFragmentation == pkg.heapFragmentation);
                REQUIRE(pkg2.maxAllocHeap == pkg.maxAllocHeap);
                
                // Network Performance
                REQUIRE(pkg2.bytesReceived == pkg.bytesReceived);
                REQUIRE(pkg2.bytesSent == pkg.bytesSent);
                REQUIRE(pkg2.packetsReceived == pkg.packetsReceived);
                REQUIRE(pkg2.packetsSent == pkg.packetsSent);
                REQUIRE(pkg2.packetsDropped == pkg.packetsDropped);
                REQUIRE(pkg2.currentThroughput == pkg.currentThroughput);
                
                // Timing and Latency
                REQUIRE(pkg2.avgResponseTime == pkg.avgResponseTime);
                REQUIRE(pkg2.maxResponseTime == pkg.maxResponseTime);
                REQUIRE(pkg2.avgMeshLatency == pkg.avgMeshLatency);
                
                // Connection Quality
                REQUIRE(pkg2.connectionQuality == pkg.connectionQuality);
                REQUIRE(pkg2.wifiRSSI == pkg.wifiRSSI);
                
                // Metadata
                REQUIRE(pkg2.collectionTimestamp == pkg.collectionTimestamp);
                REQUIRE(pkg2.collectionInterval == pkg.collectionInterval);
            }
        }
    }
}

SCENARIO("HealthCheckPackage serialization works correctly") {
    GIVEN("A HealthCheckPackage with health indicators") {
        auto pkg = HealthCheckPackage();
        pkg.from = 54321;
        
        // Overall Health
        pkg.healthStatus = 1;  // Warning
        pkg.problemFlags = 0x0001 | 0x0004;  // Low memory + connection instability
        
        // Memory Health
        pkg.memoryHealth = 75;
        pkg.memoryTrend = 1024;  // Losing 1KB/hour
        
        // Network Health
        pkg.networkHealth = 80;
        pkg.packetLossPercent = 5;
        pkg.reconnectionCount = 3;
        
        // Performance Health
        pkg.performanceHealth = 90;
        pkg.missedDeadlines = 2;
        pkg.maxLoopTime = 150;
        
        // Environmental
        pkg.temperature = 45;
        pkg.temperatureHealth = 95;
        
        // Uptime and Stability
        pkg.uptime = 86400;  // 1 day
        pkg.crashCount = 0;
        pkg.lastRebootReason = 1;  // Normal startup
        
        // Predictive
        pkg.estimatedTimeToFailure = 48;  // 48 hours
        pkg.recommendations = "Increase memory allocation";
        
        // Metadata
        pkg.checkTimestamp = 1234567890;
        pkg.nextCheckDue = 1234571490;  // 1 hour later
        
        REQUIRE(pkg.type == 205);
        
        WHEN("Converting it to and from Variant") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<HealthCheckPackage>();
            
            THEN("Should result in the same values") {
                REQUIRE(pkg2.from == pkg.from);
                REQUIRE(pkg2.type == pkg.type);
                
                // Overall Health
                REQUIRE(pkg2.healthStatus == pkg.healthStatus);
                REQUIRE(pkg2.problemFlags == pkg.problemFlags);
                
                // Memory Health
                REQUIRE(pkg2.memoryHealth == pkg.memoryHealth);
                REQUIRE(pkg2.memoryTrend == pkg.memoryTrend);
                
                // Network Health
                REQUIRE(pkg2.networkHealth == pkg.networkHealth);
                REQUIRE(pkg2.packetLossPercent == pkg.packetLossPercent);
                REQUIRE(pkg2.reconnectionCount == pkg.reconnectionCount);
                
                // Performance Health
                REQUIRE(pkg2.performanceHealth == pkg.performanceHealth);
                REQUIRE(pkg2.missedDeadlines == pkg.missedDeadlines);
                REQUIRE(pkg2.maxLoopTime == pkg.maxLoopTime);
                
                // Environmental
                REQUIRE(pkg2.temperature == pkg.temperature);
                REQUIRE(pkg2.temperatureHealth == pkg.temperatureHealth);
                
                // Uptime and Stability
                REQUIRE(pkg2.uptime == pkg.uptime);
                REQUIRE(pkg2.crashCount == pkg.crashCount);
                REQUIRE(pkg2.lastRebootReason == pkg.lastRebootReason);
                
                // Predictive
                REQUIRE(pkg2.estimatedTimeToFailure == pkg.estimatedTimeToFailure);
                REQUIRE(pkg2.recommendations == pkg.recommendations);
                
                // Metadata
                REQUIRE(pkg2.checkTimestamp == pkg.checkTimestamp);
                REQUIRE(pkg2.nextCheckDue == pkg.nextCheckDue);
            }
        }
    }
}

SCENARIO("MetricsPackage handles edge cases") {
    GIVEN("A MetricsPackage with minimum values") {
        auto pkg = MetricsPackage();
        pkg.from = 1;
        pkg.cpuUsage = 0;
        pkg.freeHeap = 0;
        pkg.wifiRSSI = -127;  // Worst signal
        
        WHEN("Converting to Variant and back") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<MetricsPackage>();
            
            THEN("Edge values should be preserved") {
                REQUIRE(pkg2.cpuUsage == 0);
                REQUIRE(pkg2.freeHeap == 0);
                REQUIRE(pkg2.wifiRSSI == -127);
            }
        }
    }
    
    GIVEN("A MetricsPackage with maximum values") {
        auto pkg = MetricsPackage();
        pkg.from = 4294967295;  // Max uint32_t
        pkg.cpuUsage = 100;
        pkg.freeHeap = 4294967295;
        pkg.wifiRSSI = 0;  // Best signal
        
        WHEN("Converting to Variant and back") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<MetricsPackage>();
            
            THEN("Maximum values should be preserved") {
                REQUIRE(pkg2.from == 4294967295);
                REQUIRE(pkg2.cpuUsage == 100);
                REQUIRE(pkg2.freeHeap == 4294967295);
                REQUIRE(pkg2.wifiRSSI == 0);
            }
        }
    }
}

SCENARIO("HealthCheckPackage problem flags work correctly") {
    GIVEN("A HealthCheckPackage with various problem flags") {
        auto pkg = HealthCheckPackage();
        
        WHEN("Setting individual problem flags") {
            pkg.problemFlags = 0x0001;  // Low memory
            REQUIRE(pkg.problemFlags == 0x0001);
            
            pkg.problemFlags |= 0x0002;  // Add high CPU
            REQUIRE(pkg.problemFlags == 0x0003);
            
            pkg.problemFlags |= 0x0080;  // Add mesh partition
            REQUIRE(pkg.problemFlags == 0x0083);
            
            THEN("Converting to Variant preserves flag state") {
                auto var = protocol::Variant(&pkg);
                auto pkg2 = var.to<HealthCheckPackage>();
                REQUIRE(pkg2.problemFlags == 0x0083);
            }
        }
    }
}

SCENARIO("HealthCheckPackage health status levels") {
    GIVEN("Different health status scenarios") {
        auto pkg = HealthCheckPackage();
        
        WHEN("System is healthy") {
            pkg.healthStatus = 2;  // Healthy
            pkg.memoryHealth = 100;
            pkg.networkHealth = 100;
            pkg.performanceHealth = 100;
            
            THEN("All health indicators should be optimal") {
                auto var = protocol::Variant(&pkg);
                auto pkg2 = var.to<HealthCheckPackage>();
                REQUIRE(pkg2.healthStatus == 2);
                REQUIRE(pkg2.memoryHealth == 100);
                REQUIRE(pkg2.networkHealth == 100);
                REQUIRE(pkg2.performanceHealth == 100);
            }
        }
        
        WHEN("System has warnings") {
            pkg.healthStatus = 1;  // Warning
            pkg.memoryHealth = 65;
            pkg.networkHealth = 70;
            pkg.performanceHealth = 75;
            
            THEN("Warning status should be preserved") {
                auto var = protocol::Variant(&pkg);
                auto pkg2 = var.to<HealthCheckPackage>();
                REQUIRE(pkg2.healthStatus == 1);
                REQUIRE(pkg2.memoryHealth == 65);
            }
        }
        
        WHEN("System is critical") {
            pkg.healthStatus = 0;  // Critical
            pkg.memoryHealth = 20;
            pkg.networkHealth = 30;
            pkg.performanceHealth = 25;
            pkg.problemFlags = 0x0FFF;  // Multiple problems
            
            THEN("Critical status should be preserved") {
                auto var = protocol::Variant(&pkg);
                auto pkg2 = var.to<HealthCheckPackage>();
                REQUIRE(pkg2.healthStatus == 0);
                REQUIRE(pkg2.problemFlags == 0x0FFF);
            }
        }
    }
}
