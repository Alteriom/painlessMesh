//  The following compile options are required for painlessMesh
#define _TASK_PRIORITY      // Support for layered scheduling priority
#define _TASK_STD_FUNCTION  // Support for std::function (ESP8266 and ESP32)
                            // Required for painlessMesh lambda callbacks

// NOTE: _TASK_THREAD_SAFE is currently DISABLED due to incompatibility
// with _TASK_STD_FUNCTION in TaskScheduler v4.0.x
// 
// The thread-safe mode would prevent FreeRTOS assertion failures on ESP32,
// but TaskScheduler's implementation has bugs when trying to cast function
// pointers to std::function objects in processRequests().
//
// Workaround: Use semaphore timeout increase (mesh.hpp line 544: 10ms -> 100ms)
// See: docs/troubleshooting/SENSOR_NODE_CONNECTION_CRASH.md
//
// #ifdef ESP32
// #define _TASK_THREAD_SAFE   // DISABLED - Incompatible with _TASK_STD_FUNCTION
// #endif

