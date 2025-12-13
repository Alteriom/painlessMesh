#ifndef ALTERIOM_PAINLESS_MESH_H
#define ALTERIOM_PAINLESS_MESH_H

/**
 * @file AlteriomPainlessMesh.h
 * @brief Main header file for AlteriomPainlessMesh library
 * 
 * This is the primary header file that includes all necessary components
 * for the AlteriomPainlessMesh library, including the core painlessMesh
 * functionality and Alteriom-specific extensions.
 */

// Include the core painlessMesh library
#include "painlessMesh.h"

// Note: Alteriom extensions (alteriom_sensor_package.hpp) are provided as examples
// and should be included directly in your sketch when needed

/**
 * @namespace alteriom
 * @brief Namespace containing Alteriom-specific extensions for painlessMesh
 * 
 * This namespace includes production-ready packages for common IoT scenarios:
 * - SensorPackage: Environmental sensor data broadcasting
 * - CommandPackage: Device control and configuration
 * - StatusPackage: Device health and operational status
 */

/**
 * @brief AlteriomPainlessMesh library version information
 */
#define ALTERIOM_PAINLESS_MESH_VERSION "1.9.7"
#define ALTERIOM_PAINLESS_MESH_VERSION_MAJOR 1
#define ALTERIOM_PAINLESS_MESH_VERSION_MINOR 9
#define ALTERIOM_PAINLESS_MESH_VERSION_PATCH 7

/**
 * @brief Library description and usage information
 * 
 * AlteriomPainlessMesh is an enhanced fork of the popular painlessMesh library
 * that adds production-ready, type-safe packages for common IoT communication
 * patterns. It provides:
 * 
 * - Automatic mesh network formation and management
 * - Type-safe messaging with compile-time validation
 * - Pre-built packages for sensors, commands, and status reporting
 * - Cross-platform compatibility (ESP32, ESP8266, desktop)
 * - Time synchronization across all mesh nodes
 * 
 * @section quick_start Quick Start
 * 
 * @code{.cpp}
 * #include "AlteriomPainlessMesh.h"
 * 
 * #define MESH_PREFIX     "MyMesh"
 * #define MESH_PASSWORD   "MyPassword"
 * #define MESH_PORT       5555
 * 
 * Scheduler userScheduler;
 * painlessMesh mesh;
 * 
 * void setup() {
 *     Serial.begin(115200);
 *     
 *     mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
 *     mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
 *     
 *     mesh.onReceive([](uint32_t from, String& msg) {
 *         Serial.printf("Received: %s\n", msg.c_str());
 *     });
 * }
 * 
 * void loop() {
 *     mesh.update();
 * }
 * @endcode
 * 
 * @section alteriom_packages Alteriom Packages
 * 
 * @code{.cpp}
 * using namespace alteriom;
 * 
 * // Send sensor data
 * SensorPackage sensor;
 * sensor.temperature = 25.5;
 * sensor.humidity = 60.0;
 * mesh.sendPackage(&sensor);
 * 
 * // Handle received sensor data
 * mesh.onPackage(200, [](protocol::Variant& variant) {
 *     SensorPackage data = variant.to<SensorPackage>();
 *     Serial.printf("Temperature: %.1f°C\n", data.temperature);
 *     return true;
 * });
 * @endcode
 */

#endif // ALTERIOM_PAINLESS_MESH_H