#include "painlessmesh/scheduler_queue.hpp"

#ifdef ESP32

namespace painlessmesh {
namespace scheduler {

// Global queue handle for TaskScheduler thread-safe operations
QueueHandle_t tsQueue = NULL;

bool initQueue() {
    if (tsQueue != NULL) {
        return true;  // Already initialized
    }
    
    tsQueue = xQueueCreate(TS_QUEUE_LEN, sizeof(_task_request_t));
    return (tsQueue != NULL);
}

}  // namespace scheduler
}  // namespace painlessmesh

/**
 * Enqueue a task request (called from any thread/ISR)
 * This function is required when _TASK_THREAD_SAFE is enabled
 * 
 * Overrides the weak default implementation in TaskScheduler.h
 */
bool _task_enqueue_request(_task_request_t* req) {
    using namespace painlessmesh::scheduler;
    
    if (tsQueue == NULL) {
        return false;  // Queue not initialized
    }
    
    if (xPortInIsrContext()) {
        // Called from ISR context
        BaseType_t xHigherPriorityTaskWokenByPost = pdFALSE;
        BaseType_t rc = xQueueSendFromISR(tsQueue, req, &xHigherPriorityTaskWokenByPost);
        if (xHigherPriorityTaskWokenByPost) {
            portYIELD_FROM_ISR();
        }
        return (rc == pdTRUE);
    } else {
        // Called from normal task context
        return (xQueueSend(tsQueue, req, pdMS_TO_TICKS(TS_ENQUEUE_WAIT_MS)) == pdTRUE);
    }
}

/**
 * Dequeue a task request (called by scheduler)
 * This function is required when _TASK_THREAD_SAFE is enabled
 * 
 * Overrides the weak default implementation in TaskScheduler.h
 */
bool _task_dequeue_request(_task_request_t* req) {
    using namespace painlessmesh::scheduler;
    
    if (tsQueue == NULL) {
        return false;  // Queue not initialized
    }
    
    if (xPortInIsrContext()) {
        // Called from ISR context (shouldn't normally happen)
        BaseType_t xHigherPriorityTaskWokenByPost = pdFALSE;
        BaseType_t rc = xQueueReceiveFromISR(tsQueue, req, &xHigherPriorityTaskWokenByPost);
        if (xHigherPriorityTaskWokenByPost) {
            portYIELD_FROM_ISR();
        }
        return (rc == pdTRUE);
    } else {
        // Called from normal task context
        return (xQueueReceive(tsQueue, req, TS_DEQUEUE_WAIT_MS) == pdTRUE);
    }
}

#endif  // ESP32
