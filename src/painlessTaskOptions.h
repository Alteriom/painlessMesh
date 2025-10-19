//  The following compile options are required for painlessMesh
#define _TASK_PRIORITY      // Support for layered scheduling priority
#define _TASK_STD_FUNCTION  // Support for std::function (ESP8266 and ESP32
                            // ONLY)

// Thread-safe scheduler for ESP32 to prevent FreeRTOS assertion failures
// See: docs/troubleshooting/SENSOR_NODE_CONNECTION_CRASH.md
#ifdef ESP32
#define _TASK_THREAD_SAFE   // Enable FreeRTOS queue-based task control
#endif

