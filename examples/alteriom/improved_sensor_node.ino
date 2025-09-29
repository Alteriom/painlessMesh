/**
 * Improved Sensor Node Example with Validation and Metrics
 * 
 * This example demonstrates the new security and performance improvements
 * in painlessMesh including input validation, rate limiting, and performance metrics.
 */

#include "painlessMesh.h"
#include "examples/alteriom/alteriom_sensor_package.hpp"

// Include new improvement modules
#include "painlessmesh/validation.hpp"
#include "painlessmesh/metrics.hpp"
#include "painlessmesh/memory.hpp"

#define MESH_PREFIX     "AlteriomMesh"
#define MESH_PASSWORD   "securePassword123"
#define MESH_PORT       5555

#define SENSOR_TASK_INTERVAL    30000   // Send sensor data every 30 seconds
#define METRICS_TASK_INTERVAL   60000   // Report metrics every minute
#define VALIDATION_ENABLED      true    // Enable message validation

Scheduler userScheduler;
painlessMesh mesh;

// New improvement features
painlessmesh::validation::MessageValidator validator;
painlessmesh::validation::RateLimiter rateLimiter(5, 1000); // 5 msgs/sec max
painlessmesh::metrics::MetricsCollector metrics;
painlessmesh::memory::ObjectPool<alteriom::SensorPackage> sensorPool(5);

// Task declarations
Task sensorTask(SENSOR_TASK_INTERVAL, TASK_FOREVER, &sendSensorData);
Task metricsTask(METRICS_TASK_INTERVAL, TASK_FOREVER, &reportMetrics);

void setup() {
    Serial.begin(115200);
    
    // Configure validation with custom settings
    painlessmesh::validation::ValidationConfig config;
    config.max_message_size = 4096;        // Smaller max size for sensors
    config.max_string_length = 256;        // Reasonable string limits
    config.strict_type_checking = true;    // Enable strict validation
    validator.set_config(config);
    
    mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
    mesh.onReceive(&receivedCallback);
    mesh.onNewConnection(&newConnectionCallback);
    mesh.onChangedConnections(&changedConnectionCallback);
    mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
    
    userScheduler.addTask(sensorTask);
    userScheduler.addTask(metricsTask);
    sensorTask.enable();
    metricsTask.enable();
    
    Serial.println("Enhanced Sensor Node Started");
    Serial.println("Features: Validation, Rate Limiting, Metrics, Memory Optimization");
}

void loop() {
    mesh.update();
    metrics.update(); // Update performance metrics
}

void sendSensorData() {
    // Use object pool for memory efficiency
    auto sensorPackage = sensorPool.acquire();
    
    // Simulate sensor readings
    sensorPackage->temperature = 20.0 + random(-5, 15);  // 15-35°C
    sensorPackage->humidity = 40.0 + random(0, 40);      // 40-80%
    sensorPackage->pressure = 1013.25 + random(-50, 50); // Near sea level
    sensorPackage->batteryLevel = 85 + random(-10, 15);  // Battery simulation
    sensorPackage->sensorId = mesh.getNodeId();
    sensorPackage->timestamp = mesh.getNodeTime();
    
    // Convert to message variant
    painlessmesh::protocol::Variant variant(sensorPackage.get());
    String message;
    variant.printTo(message);
    
    // Validate message before sending (optional security check)
    if (VALIDATION_ENABLED) {
        JsonDocument doc;
        deserializeJson(doc, message);
        JsonObject obj = doc.as<JsonObject>();
        
        auto result = validator.validate_message(obj, message.length());
        if (result != painlessmesh::validation::ValidationResult::VALID) {
            Serial.printf("Message validation failed: %s\n", 
                         validator.get_error_message(result));
            sensorPool.release(std::move(sensorPackage)); // Return to pool
            return;
        }
    }
    
    // Send the message
    bool sent = mesh.sendBroadcast(message);
    if (sent) {
        metrics.message_stats().record_sent_message(message.length());
        Serial.printf("Sensor data sent: T=%.1f°C, H=%.1f%%, P=%.1f hPa, Batt=%d%%\n",
                     sensorPackage->temperature, sensorPackage->humidity, 
                     sensorPackage->pressure, sensorPackage->batteryLevel);
    } else {
        metrics.message_stats().record_dropped_message();
        Serial.println("Failed to send sensor data - network congestion");
    }
    
    // Return object to pool for reuse
    sensorPool.release(std::move(sensorPackage));
}

void reportMetrics() {
    String metricsJson = metrics.generate_status_json();
    Serial.println("=== Performance Metrics ===");
    Serial.println(metricsJson);
    
    // Check for performance issues
    if (metrics.has_performance_issues()) {
        Serial.println("WARNING: Performance issues detected!");
        
        if (metrics.memory_stats().is_memory_critical()) {
            Serial.println("- Critical memory shortage");
        }
        
        if (metrics.message_stats().packet_loss_rate() > 0.1) {
            Serial.printf("- High packet loss: %.1f%%\n", 
                         metrics.message_stats().packet_loss_rate() * 100);
        }
        
        if (metrics.topology_stats().connection_stability() < 0.8) {
            Serial.printf("- Poor connection stability: %.1f%%\n",
                         metrics.topology_stats().connection_stability() * 100);
        }
    }
    
    Serial.println("===========================");
}

void receivedCallback(uint32_t from, String& msg) {
    // Record metrics
    metrics.message_stats().record_received_message(msg.length());
    
    // Rate limiting check
    if (!rateLimiter.allow_message(from)) {
        Serial.printf("Rate limit exceeded for node %u, dropping message\n", from);
        metrics.message_stats().record_dropped_message();
        return;
    }
    
    // Validate incoming message
    if (VALIDATION_ENABLED) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, msg);
        
        if (error) {
            Serial.printf("JSON parsing failed from %u: %s\n", from, error.c_str());
            metrics.message_stats().record_parse_error();
            return;
        }
        
        JsonObject obj = doc.as<JsonObject>();
        auto result = validator.validate_message(obj, msg.length());
        
        if (result != painlessmesh::validation::ValidationResult::VALID) {
            Serial.printf("Message validation failed from %u: %s\n", 
                         from, validator.get_error_message(result));
            metrics.message_stats().record_validation_error();
            return;
        }
    }
    
    // Process the validated message
    JsonDocument doc;
    deserializeJson(doc, msg);
    JsonObject obj = doc.as<JsonObject>();
    uint8_t msgType = obj["type"];
    
    switch(msgType) {
        case 200: // SensorPackage
            handleSensorData(alteriom::SensorPackage(obj), from);
            break;
        case 201: // CommandPackage  
            handleCommand(alteriom::CommandPackage(obj), from);
            break;
        case 202: // StatusPackage
            handleStatus(alteriom::StatusPackage(obj), from);
            break;
        default:
            Serial.printf("Unknown message type %d from %u\n", msgType, from);
    }
}

void handleSensorData(alteriom::SensorPackage sensorData, uint32_t from) {
    Serial.printf("Sensor data from %u: T=%.1f°C, H=%.1f%%, P=%.1f hPa\n", 
                 from, sensorData.temperature, sensorData.humidity, sensorData.pressure);
}

void handleCommand(alteriom::CommandPackage command, uint32_t from) {
    Serial.printf("Command from %u: cmd=%d, target=%u\n", 
                 from, command.command, command.targetDevice);
    
    // Example: Handle LED control command
    if (command.command == 1 && command.targetDevice == mesh.getNodeId()) {
        digitalWrite(LED_BUILTIN, command.parameters.indexOf("ON") >= 0 ? HIGH : LOW);
        Serial.println("LED command executed");
    }
}

void handleStatus(alteriom::StatusPackage status, uint32_t from) {
    Serial.printf("Status from %u: status=%d, uptime=%u, mem=%u\n",
                 from, status.deviceStatus, status.uptime, status.freeMemory);
}

void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("New connection: %u\n", nodeId);
    
    // Update topology metrics
    auto nodeList = mesh.getNodeList();
    metrics.topology_stats().record_topology_change(
        nodeList.size() + 1,  // +1 for this node
        mesh.connectionCount(),
        0  // TODO: Calculate max hops
    );
}

void changedConnectionCallback() {
    Serial.printf("Changed connections\n");
    
    // Update topology metrics
    auto nodeList = mesh.getNodeList();
    metrics.topology_stats().record_topology_change(
        nodeList.size() + 1,
        mesh.connectionCount(), 
        0  // TODO: Calculate max hops
    );
}

void nodeTimeAdjustedCallback(int32_t offset) {
    Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}