# painlessMesh Device Simulator - Quick Start Guide

> **Full details in [SIMULATOR_PLAN.md](SIMULATOR_PLAN.md)**

## What is This?

A standalone device simulator that allows you to:
- ✅ Spawn 100+ virtual ESP32/ESP8266 mesh nodes
- ✅ Test your actual firmware code without hardware
- ✅ Validate painlessMesh library under various scenarios
- ✅ Simulate network conditions (latency, packet loss)
- ✅ Visualize mesh topology in real-time
- ✅ Collect performance metrics

## Quick Decision: Separate Repository

**Recommended: Create `painlessMesh-simulator` as a new repository**

**Why separate?**
- Different scope (application vs. library)
- Independent versioning and releases
- Different dependencies (UI libraries, etc.)
- Different audience (developers vs. end users)
- Cleaner maintenance

**Structure:**
```
painlessMesh-simulator/
├── src/                    # Simulator code
├── examples/
│   ├── scenarios/          # YAML scenario files
│   └── firmware/           # Example firmware modules
├── external/
│   └── painlessMesh/       # Git submodule to this repo
└── docs/                   # Documentation
```

## Implementation Phases

### Phase 1: Core (Weeks 1-2)
- Basic simulator framework
- VirtualNode class
- Configuration system (YAML)
- **Deliverable**: Spawn N nodes, form mesh

### Phase 2: Scenarios (Weeks 3-4)
- Event-based scenario engine
- Network condition simulation
- **Deliverable**: Run predefined scenarios

### Phase 3: Firmware (Weeks 5-6)
- Firmware integration framework
- Mock Arduino/ESP APIs
- Alteriom package support
- **Deliverable**: Run actual firmware code

### Phase 4: Visualization (Weeks 7-8)
- Metrics collection
- Terminal UI
- GraphViz export
- **Deliverable**: Visual feedback

### Phase 5: Polish (Weeks 9-10)
- Testing and validation
- Documentation
- **Deliverable**: Production v1.0.0

## Example Usage

### Basic Scenario
```yaml
# simple_mesh.yaml
simulation:
  name: "10 Node Test"
  duration: 60

nodes:
  - template: "sensor"
    count: 10
    firmware: "examples/firmware/sensor_node"
    config:
      mesh_prefix: "TestMesh"
      mesh_password: "test123"

topology:
  type: "random"
```

### Run Simulation
```bash
./painlessmesh-simulator --config simple_mesh.yaml --ui terminal
```

### Stress Test (100+ Nodes)
```yaml
simulation:
  duration: 300
  time_scale: 5.0  # 5x faster

nodes:
  - template: "sensor"
    count: 100
    firmware: "examples/firmware/sensor_node"
```

## Key Features

### 1. Configuration-Driven
- YAML-based scenario files
- Template-based node generation
- Network condition simulation
- Event-based testing

### 2. Firmware Integration
```cpp
class MyFirmware : public FirmwareBase {
public:
    void setup() override {
        // Your setup code
    }
    
    void loop() override {
        // Your loop code
    }
    
    void onReceive(uint32_t from, String& msg) override {
        // Handle messages
    }
};
```

### 3. Realistic Simulation
- Based on actual painlessMesh code
- Boost.Asio for networking
- Configurable latency/packet loss
- Time synchronization testing

### 4. Performance Testing
- 100+ node support
- Message delivery statistics
- Latency measurements
- Topology change tracking

## Getting Started (Once Created)

### 1. Create Repository
```bash
# On GitHub: Create painlessMesh-simulator repo
git clone https://github.com/Alteriom/painlessMesh-simulator.git
cd painlessMesh-simulator

# Add painlessMesh as submodule
git submodule add https://github.com/Alteriom/painlessMesh.git external/painlessMesh
git submodule update --init --recursive
```

### 2. Build
```bash
sudo apt-get install cmake ninja-build libboost-dev
cmake -G Ninja .
ninja
```

### 3. Run Example
```bash
./bin/painlessmesh-simulator --config examples/scenarios/simple_mesh.yaml
```

### 4. Create Your Scenario
```bash
# Copy and modify template
cp examples/scenarios/simple_mesh.yaml my_scenario.yaml
# Edit my_scenario.yaml
./bin/painlessmesh-simulator --config my_scenario.yaml
```

## Use Cases

### 1. Library Development
Test painlessMesh changes with 100+ nodes before releasing

### 2. Firmware Validation
Verify your ESP32 firmware works correctly in mesh

### 3. Performance Analysis
Identify bottlenecks and optimize message routing

### 4. Scenario Testing
Test network partitions, node failures, recovery

### 5. CI/CD Integration
Automated testing of firmware changes

## Integration with Your Firmware

### Option A: Copy Your Code
```
examples/firmware/my_device/
├── my_device.cpp          # Your firmware
├── my_device.hpp
└── config.yaml            # Scenario for testing
```

### Option B: Submodule Your Repo
```bash
git submodule add https://github.com/you/your-firmware.git examples/firmware/my_device
```

### Use in Scenario
```yaml
nodes:
  - firmware: "examples/firmware/my_device"
    config:
      # Your device config
```

## CI/CD Example

```yaml
# .github/workflows/test-firmware.yml
name: Firmware Simulation

on: [push, pull_request]

jobs:
  simulate:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
        with:
          submodules: recursive
      
      - name: Install dependencies
        run: sudo apt-get install cmake ninja-build libboost-dev
      
      - name: Build simulator
        run: cmake -G Ninja . && ninja
      
      - name: Run basic test
        run: |
          ./bin/painlessmesh-simulator \
            --config scenarios/my_test.yaml \
            --headless --duration 60
      
      - name: Check results
        run: ./scripts/verify_results.sh results/
```

## Key Commands

```bash
# Basic run
./painlessmesh-simulator --config scenario.yaml

# With terminal UI
./painlessmesh-simulator --config scenario.yaml --ui terminal

# Fast-forward (10x speed)
./painlessmesh-simulator --config scenario.yaml --speed 10

# Export topology
./painlessmesh-simulator --config scenario.yaml --export-dot topology.dot

# Headless (for CI)
./painlessmesh-simulator --config scenario.yaml --headless

# Verbose logging
./painlessmesh-simulator --config scenario.yaml --log-level DEBUG
```

## Next Steps

1. **Create Repository**: Set up `painlessMesh-simulator` repo
2. **Implement Phase 1**: Basic framework and node management
3. **Test**: Verify 10-node mesh formation
4. **Iterate**: Add features based on feedback
5. **Document**: Write guides and tutorials

## Architecture Overview

```
┌─────────────────────────────────────┐
│      Simulator Application          │
├─────────────────────────────────────┤
│  Config → Scenarios → Visualization │
├─────────────────────────────────────┤
│         Node Manager                │
│  ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐  │
│  │Node1│ │Node2│ │Node3│ │ ... │  │
│  └─────┘ └─────┘ └─────┘ └─────┘  │
├─────────────────────────────────────┤
│      Network Simulator              │
│      (Boost.Asio)                   │
├─────────────────────────────────────┤
│      painlessMesh Library           │
└─────────────────────────────────────┘
```

## Benefits

### For Library Developers
- Test with 100+ nodes locally
- Reproduce complex bugs
- Validate changes before release
- Performance benchmarking

### For Firmware Developers
- Test without hardware
- Rapid iteration
- Edge case testing
- CI/CD integration

### For Researchers
- Network topology experiments
- Protocol evaluation
- Performance analysis
- Algorithm validation

## Technical Details

### Based on Existing Code
The simulator builds on `test/boost/tcp_integration.cpp` which already demonstrates:
- Multi-node simulation (tested with 12+ nodes)
- Mesh formation and routing
- Time synchronization
- Message passing

### Key Technologies
- **C++14**: Core language
- **Boost.Asio**: Networking
- **painlessMesh**: Mesh library (via submodule)
- **ArduinoJson**: Message serialization
- **YAML-CPP**: Configuration parsing
- **CMake**: Build system

### Performance Targets
- 100 nodes: Real-time on modern hardware
- 200+ nodes: May need time scaling (2-5x slower)
- Configurable time_scale for fast-forward

## Support

- **Full Plan**: [SIMULATOR_PLAN.md](SIMULATOR_PLAN.md)
- **Architecture**: See Phase 1 in full plan
- **API Reference**: After implementation
- **Examples**: `examples/scenarios/` (after creation)

## Contributing

Once the repository is created:

1. Fork the repository
2. Create feature branch
3. Implement component
4. Add tests
5. Update documentation
6. Submit pull request

## License

To be determined (recommend MIT to match painlessMesh)

---

**Status**: Planning Complete - Ready for Implementation  
**Repository**: To be created at `github.com/Alteriom/painlessMesh-simulator`  
**Timeline**: 10 weeks to v1.0.0  
**Contact**: See painlessMesh repository for maintainer info
