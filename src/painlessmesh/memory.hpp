#ifndef _PAINLESS_MESH_MEMORY_HPP_
#define _PAINLESS_MESH_MEMORY_HPP_

/**
 * Memory management utilities for painlessMesh
 * 
 * This module provides optimized memory management utilities to reduce
 * allocations and improve performance in memory-constrained environments.
 */

#include "painlessmesh/configuration.hpp"
#include <memory>
#include <vector>

namespace painlessmesh {
namespace memory {

/**
 * Object pool for frequently allocated types
 * Reduces allocation overhead for commonly used objects
 */
template<typename T>
class ObjectPool {
public:
    ObjectPool(size_t initial_size = 10) {
        pool_.reserve(initial_size);
        for (size_t i = 0; i < initial_size; ++i) {
            pool_.emplace_back(std::make_unique<T>());
        }
    }
    
    std::unique_ptr<T> acquire() {
        if (!pool_.empty()) {
            auto obj = std::move(pool_.back());
            pool_.pop_back();
            return obj;
        }
        return std::make_unique<T>();
    }
    
    void release(std::unique_ptr<T> obj) {
        if (pool_.size() < max_pool_size_) {
            // Reset object to default state before returning to pool
            *obj = T{};
            pool_.push_back(std::move(obj));
        }
        // If pool is full, let unique_ptr destroy the object automatically
    }
    
    size_t size() const { return pool_.size(); }
    
private:
    std::vector<std::unique_ptr<T>> pool_;
    static constexpr size_t max_pool_size_ = 20; // Prevent unlimited growth
};

/**
 * Pre-sized string buffer to avoid frequent reallocations
 */
class StringBuffer {
public:
    explicit StringBuffer(size_t initial_capacity = 512) {
        buffer_.reserve(initial_capacity);
    }
    
    void clear() { buffer_.clear(); }
    
    void append(const TSTRING& str) {
        if (buffer_.capacity() < buffer_.size() + str.length()) {
            buffer_.reserve(std::max(buffer_.capacity() * 2, buffer_.size() + str.length()));
        }
        buffer_ += str;
    }
    
    const TSTRING& str() const { return buffer_; }
    TSTRING& str() { return buffer_; }
    
    size_t capacity() const { return buffer_.capacity(); }
    size_t size() const { return buffer_.size(); }
    
private:
    TSTRING buffer_;
};

/**
 * Memory usage statistics for monitoring
 */
struct MemoryStats {
    size_t total_allocated = 0;
    size_t total_freed = 0;
    size_t current_usage = 0;
    size_t peak_usage = 0;
    size_t allocation_count = 0;
    
    void record_allocation(size_t size) {
        total_allocated += size;
        current_usage += size;
        peak_usage = std::max(peak_usage, current_usage);
        ++allocation_count;
    }
    
    void record_deallocation(size_t size) {
        total_freed += size;
        current_usage = (current_usage >= size) ? current_usage - size : 0;
    }
    
    void reset() {
        *this = MemoryStats{};
    }
};

// Global memory statistics (optional, can be disabled for production)
#ifdef PAINLESS_MESH_ENABLE_MEMORY_STATS
extern MemoryStats global_memory_stats;
#endif

} // namespace memory
} // namespace painlessmesh

#endif // _PAINLESS_MESH_MEMORY_HPP_