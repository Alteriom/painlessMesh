#ifndef _PAINLESSMESH_SCHEDULER_QUEUE_HPP_
#define _PAINLESSMESH_SCHEDULER_QUEUE_HPP_

#ifdef ESP32

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <TaskSchedulerDeclarations.h>

namespace painlessmesh {
namespace scheduler {

// Configuration constants
constexpr size_t TS_QUEUE_LEN = 16;           // Task request queue length
constexpr TickType_t TS_ENQUEUE_WAIT_MS = 10; // Max wait for enqueue (ms)
constexpr TickType_t TS_DEQUEUE_WAIT_MS = 0;  // No wait for dequeue

// Global queue handle (initialized in mesh init)
extern QueueHandle_t tsQueue;

/**
 * Initialize the TaskScheduler request queue
 * Must be called during mesh initialization on ESP32
 * 
 * @return true if queue created successfully, false otherwise
 */
bool initQueue();

}  // namespace scheduler
}  // namespace painlessmesh

#endif  // ESP32

#endif  // _PAINLESSMESH_SCHEDULER_QUEUE_HPP_
