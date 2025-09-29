#define CATCH_CONFIG_MAIN

#include "catch2/catch.hpp"

#include "Arduino.h"
#include "catch_utils.hpp"
#include "painlessmesh/metrics.hpp"

using namespace painlessmesh;
using namespace painlessmesh::metrics;

SCENARIO("Timer functionality works correctly") {
    GIVEN("A timer instance") {
        Timer timer;
        
        WHEN("Measuring elapsed time") {
            uint32_t start_time = timer.elapsed_ms();
            
            // Simulate some work (not real delay to avoid slowing tests)
            for (volatile int i = 0; i < 1000; i++) {
                // Busy wait
            }
            
            uint32_t end_time = timer.elapsed_ms();
            
            THEN("Elapsed time should be non-negative") {
                REQUIRE(end_time >= start_time);
            }
        }
        
        WHEN("Resetting the timer") {
            timer.reset();
            uint32_t elapsed = timer.elapsed_ms();
            
            THEN("Elapsed time should be close to zero") {
                REQUIRE(elapsed < 10); // Allow some margin for execution time
            }
        }
    }
}

SCENARIO("Message statistics tracking works correctly") {
    GIVEN("A MessageStats instance") {
        MessageStats stats;
        
        WHEN("Recording sent messages") {
            stats.record_sent_message(100);
            stats.record_sent_message(200);
            
            THEN("Statistics should be updated correctly") {
                REQUIRE(stats.messages_sent == 2);
                REQUIRE(stats.bytes_sent == 300);
            }
        }
        
        WHEN("Recording received messages") {
            stats.record_received_message(150);
            stats.record_received_message(250);
            
            THEN("Statistics should be updated correctly") {
                REQUIRE(stats.messages_received == 2);
                REQUIRE(stats.bytes_received == 400);
            }
        }
        
        WHEN("Recording latency measurements") {
            stats.record_latency(10);
            stats.record_latency(20);
            stats.record_latency(30);
            
            THEN("Latency statistics should be calculated correctly") {
                REQUIRE(stats.min_latency_ms == 10);
                REQUIRE(stats.max_latency_ms == 30);
                REQUIRE(stats.average_latency_ms() == 20);
                REQUIRE(stats.latency_samples == 3);
            }
        }
        
        WHEN("Recording dropped messages") {
            stats.record_sent_message(100);
            stats.record_sent_message(100);  
            stats.record_dropped_message();
            
            THEN("Packet loss rate should be calculated correctly") {
                double loss_rate = stats.packet_loss_rate();
                REQUIRE(loss_rate > 0.33); // Should be 1/3
                REQUIRE(loss_rate < 0.34);
            }
        }
        
        WHEN("Resetting statistics") {
            stats.record_sent_message(100);
            stats.record_received_message(200);
            stats.reset();
            
            THEN("All counters should be zero") {
                REQUIRE(stats.messages_sent == 0);
                REQUIRE(stats.messages_received == 0);
                REQUIRE(stats.bytes_sent == 0);
                REQUIRE(stats.bytes_received == 0);
            }
        }
    }
}

SCENARIO("Memory statistics work correctly") {
    GIVEN("A MemoryStats instance") {
        MemoryStats stats;
        
        WHEN("Updating memory statistics") {
            stats.update();
            
            THEN("Memory values should be reasonable") {
                // We can't make exact assertions about memory values 
                // since they depend on the runtime environment
                REQUIRE(stats.heap_free >= 0);
                REQUIRE(stats.heap_max_alloc >= 0);
            }
        }
        
        WHEN("Checking memory status") {
            stats.heap_free = 5000; // Simulate low memory
            
            THEN("Memory status should be detected correctly") {
                REQUIRE(stats.is_memory_critical() == true);
                REQUIRE(stats.is_memory_low() == true);
            }
            
            stats.heap_free = 30000; // Simulate normal memory
            
            THEN("Memory status should be normal") {
                REQUIRE(stats.is_memory_critical() == false);
                REQUIRE(stats.is_memory_low() == false);
            }
        }
    }
}

SCENARIO("Topology statistics work correctly") {
    GIVEN("A TopologyStats instance") {
        TopologyStats stats;
        
        WHEN("Recording topology changes") {
            stats.record_topology_change(5, 8, 3);
            stats.record_topology_change(6, 10, 4);
            
            THEN("Statistics should be updated correctly") {
                REQUIRE(stats.node_count == 6);
                REQUIRE(stats.connection_count == 10);
                REQUIRE(stats.max_hops == 4);
                REQUIRE(stats.connection_changes == 2);
            }
        }
        
        WHEN("Recording failed connections") {
            stats.record_topology_change(5, 8, 3);
            stats.record_failed_connection();
            
            THEN("Connection stability should decrease") {
                double stability = stats.connection_stability();
                REQUIRE(stability == 0.0); // 1 failure out of 1 change
            }
        }
        
        WHEN("Having successful connections") {
            stats.record_topology_change(5, 8, 3);
            stats.record_topology_change(6, 10, 4);
            // No failed connections recorded
            
            THEN("Connection stability should be perfect") {
                double stability = stats.connection_stability();
                REQUIRE(stability == 1.0); // No failures
            }
        }
    }
}

SCENARIO("MetricsCollector integrates all metrics") {
    GIVEN("A MetricsCollector instance") {
        MetricsCollector collector;
        
        WHEN("Recording various events") {
            collector.message_stats().record_sent_message(100);
            collector.message_stats().record_received_message(150);
            collector.topology_stats().record_topology_change(5, 8, 3);
            
            THEN("All metrics should be accessible") {
                REQUIRE(collector.message_stats().messages_sent == 1);
                REQUIRE(collector.message_stats().messages_received == 1);
                REQUIRE(collector.topology_stats().node_count == 5);
            }
        }
        
        WHEN("Generating status JSON") {
            collector.message_stats().record_sent_message(100);
            collector.message_stats().record_latency(25);
            
            TSTRING json = collector.generate_status_json();
            
            THEN("JSON should contain key metrics") {
                REQUIRE(json.find("uptime") != std::string::npos);
                REQUIRE(json.find("messages") != std::string::npos);
                REQUIRE(json.find("memory") != std::string::npos);
                REQUIRE(json.find("topology") != std::string::npos);
                REQUIRE(json.find("sent") != std::string::npos);
            }
        }
        
        WHEN("Checking for performance issues") {
            // Simulate normal conditions
            collector.memory_stats().heap_free = 50000; // Good memory
            
            bool has_issues = collector.has_performance_issues();
            
            THEN("Should report no issues") {
                REQUIRE(has_issues == false);
            }
            
            // Simulate critical memory
            collector.memory_stats().heap_free = 5000; // Critical memory
            
            has_issues = collector.has_performance_issues();
            
            THEN("Should report performance issues") {
                REQUIRE(has_issues == true);
            }
        }
        
        WHEN("Resetting metrics") {
            collector.message_stats().record_sent_message(100);
            collector.topology_stats().record_topology_change(5, 8, 3);
            
            collector.reset();
            
            THEN("All metrics should be reset") {
                REQUIRE(collector.message_stats().messages_sent == 0);
                REQUIRE(collector.topology_stats().node_count == 0);
            }
        }
    }
}