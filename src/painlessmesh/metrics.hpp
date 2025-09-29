#ifndef _PAINLESS_MESH_METRICS_HPP_
#define _PAINLESS_MESH_METRICS_HPP_

/**
 * Performance metrics and monitoring utilities for painlessMesh
 * 
 * This module provides comprehensive performance monitoring capabilities
 * to help optimize mesh network performance and diagnose issues.
 */

#include "painlessmesh/configuration.hpp"
#include <chrono>

namespace painlessmesh {
namespace metrics {

/**
 * High-resolution timer for performance measurements
 */
class Timer {
public:
    Timer() : start_time_(get_time()) {}
    
    void reset() { start_time_ = get_time(); }
    
    uint32_t elapsed_ms() const {
        return static_cast<uint32_t>(get_time() - start_time_);
    }
    
    uint32_t elapsed_us() const {
#ifdef ESP32
        return static_cast<uint32_t>((esp_timer_get_time() - start_time_us_) / 1000);
#else
        return elapsed_ms() * 1000; // Fallback to millisecond precision
#endif
    }
    
private:
    uint32_t get_time() const {
#ifdef ARDUINO
        return millis();
#else
        auto now = std::chrono::steady_clock::now();
        auto duration = now.time_since_epoch();
        return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
#endif
    }
    
    uint32_t start_time_;
#ifdef ESP32
    int64_t start_time_us_ = esp_timer_get_time();
#endif
};

/**
 * Message statistics tracking
 */
struct MessageStats {
    uint32_t messages_sent = 0;
    uint32_t messages_received = 0;
    uint32_t messages_dropped = 0;
    uint32_t messages_retransmitted = 0;
    uint32_t bytes_sent = 0;
    uint32_t bytes_received = 0;
    uint32_t parse_errors = 0;
    uint32_t validation_errors = 0;
    
    // Latency tracking
    uint32_t min_latency_ms = UINT32_MAX;
    uint32_t max_latency_ms = 0;
    uint32_t total_latency_ms = 0;
    uint32_t latency_samples = 0;
    
    void record_sent_message(size_t bytes) {
        messages_sent++;
        bytes_sent += bytes;
    }
    
    void record_received_message(size_t bytes) {
        messages_received++;
        bytes_received += bytes;
    }
    
    void record_dropped_message() {
        messages_dropped++;
    }
    
    void record_retransmission() {
        messages_retransmitted++;
    }
    
    void record_parse_error() {
        parse_errors++;
    }
    
    void record_validation_error() {
        validation_errors++;
    }
    
    void record_latency(uint32_t latency_ms) {
        min_latency_ms = std::min(min_latency_ms, latency_ms);
        max_latency_ms = std::max(max_latency_ms, latency_ms);
        total_latency_ms += latency_ms;
        latency_samples++;
    }
    
    uint32_t average_latency_ms() const {
        return latency_samples > 0 ? total_latency_ms / latency_samples : 0;
    }
    
    double throughput_bps() const {
        uint32_t uptime_s = millis() / 1000;
        return uptime_s > 0 ? (bytes_sent + bytes_received) * 8.0 / uptime_s : 0.0;
    }
    
    double packet_loss_rate() const {
        uint32_t total_attempted = messages_sent + messages_dropped;
        return total_attempted > 0 ? static_cast<double>(messages_dropped) / total_attempted : 0.0;
    }
    
    void reset() {
        *this = MessageStats{};
    }
};

/**
 * Memory usage tracking
 */
struct MemoryStats {
    uint32_t heap_free = 0;
    uint32_t heap_max_alloc = 0;
    uint32_t heap_min_free = UINT32_MAX;
    uint32_t psram_free = 0;
    uint32_t stack_high_water = 0;
    
    void update() {
#ifdef ESP32  
        heap_free = ESP.getFreeHeap();
        heap_max_alloc = ESP.getMaxAllocHeap();
        heap_min_free = std::min(heap_min_free, heap_free);
        
        #ifdef BOARD_HAS_PSRAM
        psram_free = ESP.getFreePsram();
        #endif
        
        stack_high_water = uxTaskGetStackHighWaterMark(NULL);
#elif defined(ESP8266)
        heap_free = ESP.getFreeHeap();
        heap_max_alloc = ESP.getMaxFreeBlockSize();
        heap_min_free = std::min(heap_min_free, heap_free);
#endif
    }
    
    bool is_memory_critical() const {
        return heap_free < 10000; // Less than 10KB free is critical
    }
    
    bool is_memory_low() const {
        return heap_free < 20000; // Less than 20KB free is low
    }
};

/**
 * Network topology metrics
 */
struct TopologyStats {
    uint32_t node_count = 0;
    uint32_t connection_count = 0;
    uint32_t max_hops = 0;
    uint32_t connection_changes = 0;
    uint32_t failed_connections = 0;
    
    void record_topology_change(uint32_t nodes, uint32_t connections, uint32_t hops) {
        node_count = nodes;
        connection_count = connections;
        max_hops = hops;
        connection_changes++;
    }
    
    void record_failed_connection() {
        failed_connections++;
    }
    
    double connection_stability() const {
        return connection_changes > 0 ? 
            1.0 - static_cast<double>(failed_connections) / connection_changes : 1.0;
    }
};

/**
 * Comprehensive metrics collector
 */
class MetricsCollector {
public:
    MetricsCollector() : start_time_(millis()) {}
    
    MessageStats& message_stats() { return message_stats_; }
    MemoryStats& memory_stats() { return memory_stats_; }
    TopologyStats& topology_stats() { return topology_stats_; }
    
    const MessageStats& message_stats() const { return message_stats_; }
    const MemoryStats& memory_stats() const { return memory_stats_; }
    const TopologyStats& topology_stats() const { return topology_stats_; }
    
    /**
     * Update all metrics (call periodically)
     */
    void update() {
        memory_stats_.update();
        last_update_ = millis();
    }
    
    /**
     * Get uptime in seconds
     */
    uint32_t uptime_seconds() const {
        return (millis() - start_time_) / 1000;
    }
    
    /**
     * Generate JSON status report
     */
    TSTRING generate_status_json() const {
        TSTRING json = "{";
        json += "\"uptime\":"; json += std::to_string(uptime_seconds()); json += ",";
        json += "\"messages\":{";
        json += "\"sent\":"; json += std::to_string(message_stats_.messages_sent); json += ",";
        json += "\"received\":"; json += std::to_string(message_stats_.messages_received); json += ",";
        json += "\"dropped\":"; json += std::to_string(message_stats_.messages_dropped); json += ",";
        json += "\"avg_latency\":"; json += std::to_string(message_stats_.average_latency_ms()); json += ",";
        json += "\"throughput\":"; json += std::to_string(message_stats_.throughput_bps()); json += ",";
        json += "\"packet_loss\":"; json += std::to_string(message_stats_.packet_loss_rate());
        json += "},";
        json += "\"memory\":{";
        json += "\"heap_free\":"; json += std::to_string(memory_stats_.heap_free); json += ",";
        json += "\"heap_min\":"; json += std::to_string(memory_stats_.heap_min_free); json += ",";
        json += "\"critical\":"; json += (memory_stats_.is_memory_critical() ? "true" : "false");
        json += "},";
        json += "\"topology\":{";
        json += "\"nodes\":"; json += std::to_string(topology_stats_.node_count); json += ",";
        json += "\"connections\":"; json += std::to_string(topology_stats_.connection_count); json += ",";
        json += "\"max_hops\":"; json += std::to_string(topology_stats_.max_hops); json += ",";
        json += "\"stability\":"; json += std::to_string(topology_stats_.connection_stability());
        json += "}";
        json += "}";
        return json;
    }
    
    /**
     * Reset all metrics
     */
    void reset() {
        message_stats_.reset();
        topology_stats_ = TopologyStats{};
        start_time_ = millis();
    }
    
    /**
     * Check if metrics indicate performance issues
     */
    bool has_performance_issues() const {
        return memory_stats_.is_memory_critical() ||
               message_stats_.packet_loss_rate() > 0.1 ||  // > 10% packet loss
               topology_stats_.connection_stability() < 0.8; // < 80% stability
    }
    
private:
    MessageStats message_stats_;
    MemoryStats memory_stats_;
    TopologyStats topology_stats_;
    uint32_t start_time_;
    uint32_t last_update_ = 0;
};

// Global metrics instance (optional, can be disabled)
#ifdef PAINLESS_MESH_ENABLE_METRICS
extern MetricsCollector global_metrics;
#endif

} // namespace metrics
} // namespace painlessmesh

#endif // _PAINLESS_MESH_METRICS_HPP_