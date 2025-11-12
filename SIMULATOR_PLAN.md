# painlessMesh Device Simulator - Complete Implementation Plan

## Executive Summary

This document provides a comprehensive plan for creating a standalone device simulator for painlessMesh that enables:
- Spawning 100+ virtual mesh nodes
- Configurable scenarios for different mesh topologies
- Simulation of ESP32/ESP8266 firmware behavior
- Validation of both the painlessMesh library and custom firmware
- Performance testing and debugging capabilities

## Table of Contents

1. [Goals and Requirements](#goals-and-requirements)
2. [Current State Analysis](#current-state-analysis)
3. [Architecture Design](#architecture-design)
4. [Repository Structure](#repository-structure)
5. [Implementation Roadmap](#implementation-roadmap)
6. [Technical Specifications](#technical-specifications)
7. [Usage Examples](#usage-examples)
8. [Integration Guidelines](#integration-guidelines)
9. [Testing Strategy](#testing-strategy)
10. [Future Enhancements](#future-enhancements)

---

## Goals and Requirements

### Primary Goals

1. **Large-Scale Simulation**: Support 100+ concurrent virtual mesh nodes
2. **Firmware Validation**: Enable testing of actual ESP32/ESP8266 firmware code
3. **Library Testing**: Validate painlessMesh library behavior under various scenarios
4. **Scenario Configuration**: Provide flexible configuration for different mesh topologies
5. **Performance Analysis**: Measure throughput, latency, and reliability
6. **Debugging Support**: Facilitate troubleshooting of mesh network issues

### Functional Requirements

**FR-1**: Spawn N virtual nodes (configurable, tested up to 100+)  
**FR-2**: Configure mesh topology (random, star, ring, full-mesh, custom)  
**FR-3**: Simulate network conditions (latency, packet loss, bandwidth limits)  
**FR-4**: Load and execute custom firmware code per node  
**FR-5**: Collect metrics and logs from all nodes  
**FR-6**: Visualize mesh topology in real-time  
**FR-7**: Support Alteriom custom packages (Sensor, Command, Status, etc.)  
**FR-8**: Inject events (node failures, network partitions, reconnections)  
**FR-9**: Save/restore simulation state  
**FR-10**: Export results for analysis  

### Non-Functional Requirements

**NFR-1**: Performance - Handle 100+ nodes without significant slowdown  
**NFR-2**: Accuracy - Realistic simulation of ESP32/ESP8266 behavior  
**NFR-3**: Usability - Simple configuration and operation  
**NFR-4**: Extensibility - Easy to add new scenarios and behaviors  
**NFR-5**: Portability - Run on Linux, macOS, and Windows  

---

## Current State Analysis

### Existing Infrastructure

painlessMesh already has simulation capabilities in the `test/boost/` directory:

**Files:**
- `test/boost/tcp_integration.cpp` - Multi-node mesh simulation
- `test/boost/connection.cpp` - Connection-level testing
- `test/boost/Arduino.h` - Mock Arduino environment

**Key Components:**

1. **MeshTest Class**: Wrapper around painlessMesh::Mesh with Boost.Asio integration
```cpp
class MeshTest : public PMesh {
  public:
    MeshTest(Scheduler *scheduler, size_t id, boost::asio::io_context &io);
    void connect(MeshTest &mesh);
    std::shared_ptr<AsyncServer> pServer;
    boost::asio::io_context &io_service;
};
```

2. **Nodes Class**: Manages multiple mesh nodes
```cpp
class Nodes {
  public:
    Nodes(Scheduler *scheduler, size_t n, boost::asio::io_context &io);
    void update();
    void stop();
    auto size();
    std::shared_ptr<MeshTest> get(size_t nodeId);
};
```

### Current Capabilities

✅ Create 8-15 nodes in tests (proven)  
✅ Automatic mesh formation  
✅ Message routing and broadcasting  
✅ Time synchronization testing  
✅ Root node designation  
✅ Loop detection  
✅ Uses real painlessMesh code  

### Limitations

❌ Limited to test scenarios (not standalone)  
❌ No configuration file support  
❌ No real-time visualization  
❌ Limited to random topology  
❌ No firmware simulation capability  
❌ No network condition simulation  
❌ No metrics collection/export  
❌ Hardcoded test cases  

---

## Architecture Design

### High-Level Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                     Simulator Application                        │
├─────────────────────────────────────────────────────────────────┤
│  Configuration      │  Scenario        │  Visualization         │
│  Loader             │  Engine          │  Engine                │
├─────────────────────┴──────────────────┴────────────────────────┤
│                     Node Manager                                 │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐        │
│  │ Virtual  │  │ Virtual  │  │ Virtual  │  │   ...    │        │
│  │ Node 1   │  │ Node 2   │  │ Node 3   │  │ Node N   │        │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘        │
├─────────────────────────────────────────────────────────────────┤
│                  Network Simulator                               │
│  (Boost.Asio + TCP/IP abstraction layer)                        │
├─────────────────────────────────────────────────────────────────┤
│               painlessMesh Library Core                          │
│  (Mesh routing, time sync, connection management)               │
└─────────────────────────────────────────────────────────────────┘
```

### Component Details

#### 1. Configuration Loader
- **Purpose**: Parse configuration files (YAML/JSON)
- **Responsibilities**:
  - Load node definitions
  - Parse topology specifications
  - Configure network conditions
  - Load firmware modules

#### 2. Node Manager
- **Purpose**: Create and manage virtual nodes
- **Responsibilities**:
  - Spawn N VirtualNode instances
  - Track node lifecycle
  - Coordinate updates
  - Handle node failures

#### 3. Virtual Node
- **Purpose**: Simulate a single ESP32/ESP8266 device
- **Components**:
  - painlessMesh instance
  - Firmware module (optional)
  - Scheduler for tasks
  - Mock hardware interfaces
  - Metrics collector

#### 4. Network Simulator
- **Purpose**: Simulate network conditions
- **Features**:
  - Configurable latency (per-link or global)
  - Packet loss simulation
  - Bandwidth throttling
  - Connection quality variation

#### 5. Scenario Engine
- **Purpose**: Execute predefined scenarios
- **Capabilities**:
  - Time-based events
  - Conditional triggers
  - Node state changes
  - Network condition changes

#### 6. Visualization Engine
- **Purpose**: Display mesh topology and statistics
- **Options**:
  - Terminal UI (ncurses/blessed)
  - Web UI (WebSocket + HTML5)
  - Export to GraphViz DOT format

#### 7. Metrics Collector
- **Purpose**: Gather and export performance data
- **Metrics**:
  - Message delivery rate
  - Latency statistics
  - Network topology changes
  - Node uptime
  - Memory usage

---

## Repository Structure

### Option A: Separate Repository (Recommended)

```
painlessMesh-simulator/
├── README.md
├── LICENSE
├── CMakeLists.txt
├── .gitignore
├── .github/
│   └── workflows/
│       └── ci.yml
├── src/
│   ├── main.cpp                    # Simulator entry point
│   ├── config_loader.hpp/cpp       # Configuration parsing
│   ├── node_manager.hpp/cpp        # Node lifecycle management
│   ├── virtual_node.hpp/cpp        # VirtualNode implementation
│   ├── network_simulator.hpp/cpp   # Network condition simulation
│   ├── scenario_engine.hpp/cpp     # Scenario execution
│   ├── metrics_collector.hpp/cpp   # Metrics gathering
│   ├── visualization/
│   │   ├── terminal_ui.hpp/cpp     # Terminal-based UI
│   │   └── web_ui.hpp/cpp          # Web-based UI
│   └── firmware/
│       ├── firmware_base.hpp       # Base class for firmware modules
│       └── example_sensor.cpp      # Example firmware implementation
├── include/
│   └── simulator/                  # Public headers
├── test/
│   ├── test_config_loader.cpp
│   ├── test_virtual_node.cpp
│   └── test_scenario_engine.cpp
├── examples/
│   ├── scenarios/
│   │   ├── simple_mesh.yaml        # 10 nodes, random topology
│   │   ├── star_topology.yaml      # Hub-and-spoke configuration
│   │   ├── stress_test.yaml        # 100+ nodes
│   │   └── network_partition.yaml  # Partition and recovery
│   ├── firmware/
│   │   ├── sensor_node/            # Sensor firmware example
│   │   ├── bridge_node/            # MQTT bridge example
│   │   └── custom_protocol/        # Custom package example
│   └── visualizations/
│       └── sample_output.html
├── docs/
│   ├── ARCHITECTURE.md
│   ├── CONFIGURATION_GUIDE.md
│   ├── FIRMWARE_DEVELOPMENT.md
│   ├── API_REFERENCE.md
│   └── TROUBLESHOOTING.md
├── scripts/
│   ├── run_scenario.sh
│   ├── analyze_results.py
│   └── export_topology.sh
└── external/                       # Git submodules
    ├── painlessMesh/               # Submodule to main repo
    ├── ArduinoJson/
    └── TaskScheduler/
```

**Pros:**
- Clean separation of concerns
- Independent versioning
- Can have different license if needed
- Easier to maintain
- Can be used with any painlessMesh version

**Cons:**
- Requires submodule setup
- Separate CI/CD pipeline

### Option B: Subdirectory in Main Repository

```
painlessMesh/
├── [existing structure]
├── simulator/
│   ├── README.md
│   ├── CMakeLists.txt
│   ├── src/
│   ├── include/
│   ├── examples/
│   └── docs/
└── [existing files]
```

**Pros:**
- Single repository
- Shared CI/CD
- Easier for contributors
- Integrated documentation

**Cons:**
- Bloats main repository
- Could complicate releases
- Mixing library and tools

### Recommendation

**Create a separate repository** (`painlessMesh-simulator`) for these reasons:

1. **Scope**: The simulator is a complete application, not a library feature
2. **Dependencies**: Simulator has additional dependencies (UI libraries, etc.)
3. **Audience**: Different user base (developers vs. end users)
4. **Versioning**: Independent release cycle
5. **Maintenance**: Easier to manage separately
6. **Integration**: Can still reference main repo via submodule

---

## Implementation Roadmap

### Phase 1: Core Infrastructure (Week 1-2)

**Milestone 1.1: Basic Simulator Framework**
- [ ] Create repository with structure
- [ ] Set up CMake build system
- [ ] Integrate painlessMesh as submodule
- [ ] Create VirtualNode class
- [ ] Implement NodeManager
- [ ] Basic command-line interface

**Deliverable**: Can spawn N nodes and form mesh

**Milestone 1.2: Configuration System**
- [ ] Design configuration schema (YAML)
- [ ] Implement ConfigLoader
- [ ] Support node definitions
- [ ] Support topology specifications
- [ ] Validation and error handling

**Deliverable**: Can configure nodes via YAML file

### Phase 2: Scenario Engine (Week 3-4)

**Milestone 2.1: Basic Scenarios**
- [ ] Implement ScenarioEngine
- [ ] Time-based event system
- [ ] Node spawn/stop events
- [ ] Message injection
- [ ] Example scenarios

**Deliverable**: Can run predefined scenarios

**Milestone 2.2: Network Simulation**
- [ ] Implement NetworkSimulator
- [ ] Latency simulation
- [ ] Packet loss simulation
- [ ] Bandwidth throttling
- [ ] Per-link configuration

**Deliverable**: Realistic network conditions

### Phase 3: Firmware Integration (Week 5-6)

**Milestone 3.1: Firmware Framework**
- [ ] Design FirmwareBase interface
- [ ] Mock Arduino/ESP APIs
- [ ] Task scheduler integration
- [ ] Example firmware modules

**Deliverable**: Can run simple firmware code

**Milestone 3.2: Alteriom Package Support**
- [ ] SensorPackage firmware
- [ ] CommandPackage handling
- [ ] StatusPackage reporting
- [ ] MQTT bridge simulation

**Deliverable**: Full Alteriom stack simulation

### Phase 4: Visualization & Metrics (Week 7-8)

**Milestone 4.1: Metrics Collection**
- [ ] Implement MetricsCollector
- [ ] Message statistics
- [ ] Latency tracking
- [ ] Topology change detection
- [ ] Export to CSV/JSON

**Deliverable**: Performance analysis data

**Milestone 4.2: Visualization**
- [ ] Terminal UI (ncurses)
- [ ] Real-time topology view
- [ ] Statistics dashboard
- [ ] Log viewer
- [ ] GraphViz export

**Deliverable**: Visual feedback during simulation

### Phase 5: Polish & Documentation (Week 9-10)

**Milestone 5.1: Testing & Validation**
- [ ] Unit tests for all components
- [ ] Integration tests
- [ ] Performance benchmarks
- [ ] Stress tests (100+ nodes)

**Milestone 5.2: Documentation**
- [ ] Architecture documentation
- [ ] Configuration guide
- [ ] Firmware development guide
- [ ] API reference
- [ ] Tutorial videos

**Deliverable**: Production-ready v1.0.0

---

## Technical Specifications

### Configuration File Format

```yaml
# Example: examples/scenarios/simple_mesh.yaml
simulation:
  name: "Simple 10-Node Mesh"
  duration: 300  # seconds, 0 = infinite
  time_scale: 1.0  # 1.0 = real-time, >1.0 = faster
  seed: 12345  # Random seed for reproducibility

network:
  latency:
    min: 10  # milliseconds
    max: 50
    distribution: "normal"  # normal, uniform, exponential
  packet_loss: 0.01  # 1% packet loss
  bandwidth: 1000000  # 1 Mbps

nodes:
  - id: "node-1"
    type: "sensor"
    firmware: "examples/firmware/sensor_node"
    position: [0, 0]  # For visualization
    config:
      mesh_prefix: "TestMesh"
      mesh_password: "testpass"
      mesh_port: 5555
      sensor_interval: 30000  # ms
      
  - id: "node-2"
    type: "bridge"
    firmware: "examples/firmware/mqtt_bridge"
    position: [100, 0]
    config:
      mesh_prefix: "TestMesh"
      mesh_password: "testpass"
      mqtt_broker: "127.0.0.1"
      mqtt_port: 1883

  # Template-based node generation
  - template: "sensor"
    count: 8
    id_prefix: "sensor-"
    firmware: "examples/firmware/sensor_node"
    config:
      mesh_prefix: "TestMesh"
      mesh_password: "testpass"
      sensor_interval: 30000

topology:
  type: "random"  # random, star, ring, mesh, custom
  # For star topology:
  # hub: "node-1"
  # For custom topology:
  # connections:
  #   - [node-1, node-2]
  #   - [node-2, node-3]

events:
  - time: 60
    action: "stop_node"
    target: "node-5"
    
  - time: 120
    action: "start_node"
    target: "node-5"
    
  - time: 180
    action: "inject_message"
    from: "node-1"
    to: "broadcast"
    payload: '{"type": 200, "temperature": 25.5}'

metrics:
  output: "results/simple_mesh.csv"
  interval: 5  # seconds
  collect:
    - message_count
    - delivery_rate
    - latency_stats
    - topology_changes
```

### VirtualNode Class API

```cpp
class VirtualNode {
public:
    VirtualNode(uint32_t nodeId, 
                const NodeConfig& config,
                Scheduler* scheduler,
                boost::asio::io_context& io);
    
    // Lifecycle
    void start();
    void stop();
    void pause();
    void resume();
    
    // Mesh interface
    painlessMesh& getMesh();
    uint32_t getNodeId() const;
    void update();
    
    // Firmware integration
    void loadFirmware(std::shared_ptr<FirmwareBase> firmware);
    void setCallback(std::function<void(uint32_t, String)> cb);
    
    // Configuration
    void setPosition(int x, int y);
    void setNetworkQuality(float quality); // 0.0-1.0
    
    // Metrics
    NodeMetrics getMetrics() const;
    
    // Debugging
    void setLogLevel(logger::LogLevel level);
    std::string getState() const;
    
private:
    std::unique_ptr<MeshTest> mesh_;
    std::shared_ptr<FirmwareBase> firmware_;
    Scheduler* scheduler_;
    boost::asio::io_context& io_;
    NodeConfig config_;
    NodeMetrics metrics_;
    bool running_;
};
```

### FirmwareBase Interface

```cpp
class FirmwareBase {
public:
    virtual ~FirmwareBase() = default;
    
    // Lifecycle hooks
    virtual void setup() = 0;
    virtual void loop() = 0;
    
    // Mesh callbacks
    virtual void onReceive(uint32_t from, String& msg) = 0;
    virtual void onNewConnection(uint32_t nodeId) = 0;
    virtual void onChangedConnections() = 0;
    
    // Provide access to mesh
    void setMesh(painlessMesh* mesh) { mesh_ = mesh; }
    
protected:
    painlessMesh* mesh_;
    Scheduler* scheduler_;
};
```

### Example Firmware Implementation

```cpp
// examples/firmware/sensor_node.cpp
class SensorNodeFirmware : public FirmwareBase {
public:
    void setup() override {
        task_send_sensor_.set(30000, TASK_FOREVER, [this]() {
            sendSensorData();
        });
        scheduler_->addTask(task_send_sensor_);
        task_send_sensor_.enable();
    }
    
    void loop() override {
        // Called every iteration
    }
    
    void onReceive(uint32_t from, String& msg) override {
        // Handle commands
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, msg);
        JsonObject obj = doc.as<JsonObject>();
        
        uint8_t type = obj["type"];
        if (type == 201) {  // CommandPackage
            handleCommand(obj);
        }
    }
    
private:
    void sendSensorData() {
        using namespace alteriom;
        SensorPackage pkg;
        pkg.from = mesh_->getNodeId();
        pkg.sensorId = mesh_->getNodeId();
        pkg.timestamp = mesh_->getNodeTime();
        pkg.temperature = 23.5 + random(-50, 50) / 10.0;
        pkg.humidity = 45.0;
        pkg.pressure = 1013.25;
        pkg.batteryLevel = 85;
        
        String msg = pkg.toJson();
        mesh_->sendBroadcast(msg);
    }
    
    void handleCommand(JsonObject& obj) {
        // Handle command
    }
    
    Task task_send_sensor_;
};
```

### Command-Line Interface

```bash
# Basic usage
./painlessmesh-simulator --config examples/scenarios/simple_mesh.yaml

# With visualization
./painlessmesh-simulator --config simple_mesh.yaml --ui terminal

# Fast-forward simulation
./painlessmesh-simulator --config stress_test.yaml --speed 10

# Export results
./painlessmesh-simulator --config test.yaml --output results/

# Interactive mode
./painlessmesh-simulator --interactive

# Run specific scenario
./painlessmesh-simulator --scenario examples/scenarios/network_partition.yaml

# Debug mode with verbose logging
./painlessmesh-simulator --config test.yaml --log-level DEBUG --log-file sim.log

# Generate topology visualization
./painlessmesh-simulator --config test.yaml --export-dot topology.dot

# Headless mode for CI
./painlessmesh-simulator --config test.yaml --headless --duration 300
```

---

## Usage Examples

### Example 1: Simple Mesh Validation

**Goal**: Verify basic mesh formation with 10 nodes

```yaml
# examples/scenarios/basic_validation.yaml
simulation:
  name: "Basic Mesh Validation"
  duration: 60

network:
  latency:
    min: 5
    max: 20
  packet_loss: 0

nodes:
  - template: "basic"
    count: 10
    firmware: "examples/firmware/echo_node"
    config:
      mesh_prefix: "TestMesh"
      mesh_password: "test123"

topology:
  type: "random"

metrics:
  output: "results/basic_validation.csv"
```

**Run:**
```bash
./painlessmesh-simulator --config examples/scenarios/basic_validation.yaml
```

**Expected Output:**
```
[00:05] All 10 nodes connected
[00:10] Time sync converged (avg diff: 234ms)
[00:15] Mesh stable, no topology changes
[00:60] Simulation complete
        - Messages sent: 150
        - Delivery rate: 100%
        - Avg latency: 12ms
```

### Example 2: Stress Test (100 Nodes)

```yaml
# examples/scenarios/stress_test.yaml
simulation:
  name: "100 Node Stress Test"
  duration: 300
  time_scale: 5.0  # 5x faster

network:
  latency:
    min: 10
    max: 100
    distribution: "normal"
  packet_loss: 0.05  # 5% loss

nodes:
  - template: "sensor"
    count: 100
    firmware: "examples/firmware/sensor_node"
    config:
      mesh_prefix: "StressMesh"
      sensor_interval: 60000

topology:
  type: "random"

metrics:
  output: "results/stress_test.csv"
  interval: 10
```

### Example 3: Network Partition Recovery

```yaml
# examples/scenarios/partition_recovery.yaml
simulation:
  name: "Network Partition and Recovery"
  duration: 600

nodes:
  - template: "sensor"
    count: 20
    id_prefix: "sensor-"

topology:
  type: "custom"
  connections:
    # Create two clusters
    - ["sensor-0", "sensor-1"]
    - ["sensor-1", "sensor-2"]
    - ["sensor-2", "sensor-3"]
    - ["sensor-3", "sensor-4"]
    # Bridge between clusters
    - ["sensor-4", "sensor-10"]
    # Second cluster
    - ["sensor-10", "sensor-11"]
    - ["sensor-11", "sensor-12"]
    - ["sensor-12", "sensor-13"]

events:
  - time: 120
    action: "break_link"
    targets: ["sensor-4", "sensor-10"]
    
  - time: 300
    action: "restore_link"
    targets: ["sensor-4", "sensor-10"]
```

### Example 4: MQTT Bridge Testing

```yaml
# examples/scenarios/mqtt_bridge.yaml
simulation:
  name: "MQTT Bridge Testing"
  duration: 300

nodes:
  - id: "bridge-1"
    type: "mqtt_bridge"
    firmware: "examples/firmware/mqtt_bridge"
    config:
      mqtt_broker: "localhost"
      mqtt_port: 1883
      mqtt_topic_prefix: "mesh/"
      
  - template: "sensor"
    count: 10
    firmware: "examples/firmware/sensor_node"
    config:
      sensor_interval: 30000

topology:
  type: "star"
  hub: "bridge-1"

external:
  mqtt_broker:
    enabled: true
    host: "localhost"
    port: 1883
    
metrics:
  output: "results/mqtt_bridge.csv"
  collect:
    - mqtt_publish_count
    - mqtt_latency
```

### Example 5: Custom Firmware Validation

```cpp
// my_firmware/custom_node.cpp
#include "simulator/firmware_base.hpp"
#include "examples/alteriom/alteriom_sensor_package.hpp"

class CustomFirmware : public FirmwareBase {
public:
    void setup() override {
        // Your custom setup
        pinMode(LED_BUILTIN, OUTPUT);
        
        task_heartbeat_.set(5000, TASK_FOREVER, [this]() {
            sendHeartbeat();
        });
        scheduler_->addTask(task_heartbeat_);
        task_heartbeat_.enable();
    }
    
    void loop() override {
        // Your custom loop logic
    }
    
    void onReceive(uint32_t from, String& msg) override {
        // Your custom message handling
        Serial.printf("Received from %u: %s\n", from, msg.c_str());
    }
    
private:
    void sendHeartbeat() {
        using namespace alteriom;
        StatusPackage status;
        status.from = mesh_->getNodeId();
        status.uptime = millis() / 1000;
        mesh_->sendBroadcast(status.toJson());
    }
    
    Task task_heartbeat_;
};

// Register firmware
REGISTER_FIRMWARE("custom_node", CustomFirmware)
```

```yaml
# Use in scenario
nodes:
  - id: "custom-1"
    firmware: "my_firmware/custom_node"
    config:
      mesh_prefix: "CustomMesh"
```

---

## Integration Guidelines

### Using with painlessMesh Library

The simulator references painlessMesh via git submodule:

```bash
# Initial setup
git clone https://github.com/Alteriom/painlessMesh-simulator.git
cd painlessMesh-simulator
git submodule update --init --recursive

# Build
mkdir build && cd build
cmake -G Ninja ..
ninja
```

### Using with Custom Firmware

1. **Place your firmware in `examples/firmware/`**:
```
examples/firmware/my_device/
├── my_device.cpp
├── my_device.hpp
└── CMakeLists.txt
```

2. **Implement FirmwareBase interface**:
```cpp
class MyDeviceFirmware : public FirmwareBase {
    // Implement required methods
};
```

3. **Reference in scenario**:
```yaml
nodes:
  - firmware: "examples/firmware/my_device"
```

### CI/CD Integration

```yaml
# .github/workflows/simulation-test.yml
name: Simulation Tests

on: [push, pull_request]

jobs:
  simulate:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
        with:
          submodules: recursive
          
      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y cmake ninja-build libboost-dev
          
      - name: Build simulator
        run: |
          cmake -G Ninja .
          ninja
          
      - name: Run basic validation
        run: |
          ./bin/painlessmesh-simulator \
            --config examples/scenarios/basic_validation.yaml \
            --headless
            
      - name: Run stress test
        run: |
          ./bin/painlessmesh-simulator \
            --config examples/scenarios/stress_test.yaml \
            --headless --duration 60
            
      - name: Upload results
        uses: actions/upload-artifact@v2
        with:
          name: simulation-results
          path: results/
```

### Docker Support

```dockerfile
# Dockerfile
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    cmake ninja-build \
    libboost-dev libboost-system-dev \
    git && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN git submodule update --init --recursive && \
    cmake -G Ninja . && \
    ninja

ENTRYPOINT ["./bin/painlessmesh-simulator"]
CMD ["--help"]
```

```bash
# Build and run
docker build -t painlessmesh-simulator .
docker run -v $(pwd)/examples:/app/examples \
           -v $(pwd)/results:/app/results \
           painlessmesh-simulator \
           --config /app/examples/scenarios/simple_mesh.yaml
```

---

## Testing Strategy

### Unit Tests

```cpp
// test/test_virtual_node.cpp
TEST_CASE("VirtualNode can be created and started") {
    Scheduler scheduler;
    boost::asio::io_context io;
    
    NodeConfig config;
    config.nodeId = 1001;
    config.meshPrefix = "TestMesh";
    
    VirtualNode node(1001, config, &scheduler, io);
    node.start();
    
    REQUIRE(node.getNodeId() == 1001);
    REQUIRE(node.getMesh().getNodeId() == 1001);
}

TEST_CASE("VirtualNode can load firmware") {
    // ... test firmware loading
}
```

### Integration Tests

```cpp
// test/test_multi_node.cpp
TEST_CASE("Multiple nodes can form mesh") {
    Scheduler scheduler;
    boost::asio::io_context io;
    
    auto node1 = std::make_shared<VirtualNode>(1001, config1, &scheduler, io);
    auto node2 = std::make_shared<VirtualNode>(1002, config2, &scheduler, io);
    
    node1->start();
    node2->start();
    
    // Run simulation
    for (int i = 0; i < 1000; ++i) {
        node1->update();
        node2->update();
        io.poll();
    }
    
    // Verify mesh formed
    REQUIRE(node1->getMesh().getNodeList().size() == 2);
    REQUIRE(node2->getMesh().getNodeList().size() == 2);
}
```

### Scenario Tests

```bash
# Run all example scenarios
./scripts/test_scenarios.sh

# Verify each scenario produces expected results
./scripts/verify_results.sh results/
```

### Performance Benchmarks

```cpp
// test/benchmark_scalability.cpp
BENCHMARK("100 nodes for 60 seconds") {
    // Measure performance with 100 nodes
    auto start = std::chrono::steady_clock::now();
    
    // Run simulation
    runScenario("examples/scenarios/stress_test.yaml");
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    
    // Should complete in reasonable time
    REQUIRE(duration.count() < 120);  // Max 2 minutes for 60-second sim
}
```

---

## Future Enhancements

### Version 1.1
- [ ] Web-based UI with real-time topology visualization
- [ ] Support for ESP-NOW and BLE-Mesh protocols
- [ ] Advanced metrics (CPU usage, memory profiling)
- [ ] Automated test case generation
- [ ] Python API for scripting

### Version 1.2
- [ ] Hardware-in-the-loop (HIL) integration
- [ ] OTA firmware update simulation
- [ ] Energy consumption modeling
- [ ] RF propagation simulation
- [ ] Geographic positioning

### Version 2.0
- [ ] Multi-protocol bridge simulation
- [ ] Cloud service integration (AWS IoT, Azure IoT)
- [ ] Machine learning for anomaly detection
- [ ] Distributed simulation (multiple hosts)
- [ ] Virtual reality visualization

---

## Dependencies

### Required
- C++14 compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.10+
- Boost.Asio 1.66+
- painlessMesh (via submodule)
- ArduinoJson (via submodule)
- TaskScheduler (via submodule)

### Optional
- ncurses (for terminal UI)
- GraphViz (for topology export)
- Python 3.8+ (for analysis scripts)
- Mosquitto (for MQTT testing)

### Development
- Catch2 (unit testing)
- Google Benchmark (performance testing)
- Doxygen (documentation)
- clang-format (code formatting)

---

## Getting Started Checklist

### For Repository Creator

- [ ] Create GitHub repository: `Alteriom/painlessMesh-simulator`
- [ ] Initialize with structure from this plan
- [ ] Add painlessMesh as submodule: `git submodule add https://github.com/Alteriom/painlessMesh.git external/painlessMesh`
- [ ] Set up CI/CD workflow
- [ ] Create initial release v0.1.0
- [ ] Write README.md with quick start guide
- [ ] Add LICENSE file (MIT recommended)

### For First-Time Users

- [ ] Clone repository: `git clone --recursive https://github.com/Alteriom/painlessMesh-simulator.git`
- [ ] Install dependencies: `sudo apt-get install cmake ninja-build libboost-dev`
- [ ] Build simulator: `cmake -G Ninja . && ninja`
- [ ] Run example: `./bin/painlessmesh-simulator --config examples/scenarios/simple_mesh.yaml`
- [ ] Read documentation in `docs/`
- [ ] Try modifying example scenarios
- [ ] Create custom firmware module

### For Contributors

- [ ] Fork repository
- [ ] Create feature branch
- [ ] Write tests for new features
- [ ] Update documentation
- [ ] Submit pull request
- [ ] Address review feedback

---

## Support and Resources

### Documentation
- Architecture: `docs/ARCHITECTURE.md`
- Configuration: `docs/CONFIGURATION_GUIDE.md`
- Firmware Development: `docs/FIRMWARE_DEVELOPMENT.md`
- API Reference: Generated by Doxygen

### Community
- GitHub Issues: Bug reports and feature requests
- Discussions: Q&A and general discussion
- Wiki: Community-contributed guides

### Related Projects
- painlessMesh: https://github.com/Alteriom/painlessMesh
- ESP-MESH: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/mesh.html

---

## Conclusion

This simulator will provide a powerful tool for:

1. **Library Validation**: Test painlessMesh under various scenarios
2. **Firmware Development**: Validate custom firmware before deployment
3. **Performance Analysis**: Identify bottlenecks and optimize
4. **Education**: Learn mesh networking concepts
5. **Debugging**: Reproduce and fix complex issues

The modular architecture ensures extensibility, while the scenario-based approach provides flexibility. By separating the simulator into its own repository, we maintain clean separation of concerns while enabling powerful integration.

**Recommendation**: Start with Phase 1 to establish the foundation, then iterate based on user feedback and real-world usage patterns.

---

## Appendix A: Comparison with Alternatives

| Feature | painlessMesh-simulator | ns-3 | OMNeT++ | Cooja |
|---------|----------------------|------|---------|-------|
| painlessMesh Support | Native | None | None | None |
| ESP32 Firmware | Yes | No | No | Limited |
| Setup Complexity | Low | High | High | Medium |
| Scalability | 100+ nodes | 1000+ | 1000+ | 100 |
| Real-time Viz | Yes | Limited | Yes | Yes |
| Learning Curve | Easy | Steep | Steep | Medium |
| Cost | Free | Free | Free | Free |

**Conclusion**: For painlessMesh-specific testing, a custom simulator is the best option.

---

## Appendix B: Performance Estimates

Based on existing test infrastructure:

| Nodes | Messages/sec | CPU Usage | Memory | Real-time Factor |
|-------|--------------|-----------|---------|------------------|
| 10    | 1000+        | 5%        | 50 MB   | 1.0x (real-time) |
| 50    | 2000+        | 25%       | 200 MB  | 0.5x (2x slower) |
| 100   | 2500+        | 50%       | 400 MB  | 0.2x (5x slower) |
| 200   | 2000+        | 90%       | 800 MB  | 0.1x (10x slower) |

**Note**: Performance depends on hardware and scenario complexity. Use `time_scale` to speed up simulations.

---

## Appendix C: Quick Reference Commands

```bash
# Installation
git clone --recursive https://github.com/Alteriom/painlessMesh-simulator.git
cd painlessMesh-simulator
cmake -G Ninja . && ninja

# Basic usage
./bin/painlessmesh-simulator --config scenario.yaml

# Common options
--ui terminal              # Show terminal UI
--speed 10                 # Run 10x faster
--log-level DEBUG          # Verbose logging
--output results/          # Save results
--duration 300             # Run for 5 minutes
--headless                 # No UI (for CI)
--export-dot topology.dot  # Export topology

# Scenario templates
./scripts/create_scenario.sh --nodes 50 --topology random
./scripts/create_scenario.sh --nodes 20 --topology star --hub node-1

# Analysis
./scripts/analyze_results.py results/simulation.csv
./scripts/plot_topology.py topology.dot -o topology.png
./scripts/export_metrics.sh results/ --format json

# Testing
ninja test                 # Run all tests
./bin/test_virtual_node    # Run specific test
./scripts/benchmark.sh     # Performance benchmarks
```

---

**Document Version**: 1.0  
**Date**: 2025-11-12  
**Author**: GitHub Copilot  
**Status**: Ready for Implementation  
