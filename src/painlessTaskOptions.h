//  The following compile options are required for painlessMesh
#define _TASK_PRIORITY      // Support for layered scheduling priority

// Thread-safe scheduler for ESP32 to prevent FreeRTOS assertion failures
// See: docs/troubleshooting/SENSOR_NODE_CONNECTION_CRASH.md
#ifdef ESP32
#define _TASK_THREAD_SAFE   // Enable FreeRTOS queue-based task control
// Note: _TASK_STD_FUNCTION is incompatible with _TASK_THREAD_SAFE in TaskScheduler v4.0.x
// Using traditional callback functions for thread-safe mode
#else
#define _TASK_STD_FUNCTION  // Support for std::function (ESP8266 ONLY when not using thread-safe mode)
#endif

