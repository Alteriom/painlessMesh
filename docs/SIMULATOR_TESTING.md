# Simulator-Based Testing for painlessMesh

## Overview

painlessMesh includes integration with the [painlessMesh-simulator](https://github.com/Alteriom/painlessMesh-simulator) to enable large-scale testing of examples and firmware without physical hardware.

The simulator allows you to:
- 🚀 Test with 100+ virtual nodes simultaneously
- 🔧 Validate actual firmware code in a controlled environment
- 📋 Configure test scenarios with YAML files
- 🌐 Simulate realistic network conditions (latency, packet loss, partitions)
- 📊 Collect metrics and analyze performance
- 🔄 Integrate with CI/CD pipelines

## Architecture

```
painlessMesh Repository
├── src/                          # Library code
├── examples/                     # Example sketches
│   ├── basic/
│   │   ├── basic.ino            # Original Arduino sketch
│   │   └── test/simulator/      # Simulator tests
│   │       ├── firmware/        # Firmware adapter
│   │       ├── scenarios/       # YAML test scenarios
│   │       └── CMakeLists.txt   # Build configuration
│   └── [other examples]/
└── test/
    └── simulator/               # painlessMesh-simulator (submodule)
```

## Quick Start

### 1. Initialize Simulator Submodule

```bash
cd test
git submodule update --init simulator
```

### 2. Install Dependencies

**Ubuntu/Debian:**
```bash
sudo apt-get install cmake ninja-build libboost-dev libboost-program-options-dev libyaml-cpp-dev
```

**macOS:**
```bash
brew install cmake ninja boost yaml-cpp
```

**Windows:**
See [test/simulator/BUILD_WINDOWS_STATUS.md](../test/simulator/BUILD_WINDOWS_STATUS.md)

### 3. Run a Test

```bash
cd test/simulator
mkdir build && cd build
cmake -G Ninja ..
ninja

# Run basic example test
./painlessmesh-simulator --config ../../../examples/basic/test/simulator/scenarios/basic_mesh_test.yaml
```

## Example Test Structure

### Basic Example

The `examples/basic/` example includes complete simulator tests:

**Files:**
- `test/simulator/firmware/basic_firmware.hpp` - Firmware adapter
- `test/simulator/scenarios/basic_mesh_test.yaml` - Test configuration
- `test/simulator/README.md` - Detailed instructions

**Test Scenario (basic_mesh_test.yaml):**
```yaml
simulation:
  name: "Basic Example Test"
  duration: 60

nodes:
  - template: "basic_example"
    count: 10
    config:
      mesh_prefix: "whateverYouLike"
      mesh_password: "somethingSneaky"

validation:
  - check: "all_nodes_connected"
    timeout: 30
  - check: "messages_delivered"
    min_messages_per_node: 5
```

**Run it:**
```bash
cd examples/basic/test/simulator
mkdir build && cd build
cmake -G Ninja .. && ninja
./painlessmesh-simulator --config ../scenarios/basic_mesh_test.yaml
```

## Creating Tests for Your Example

### Step 1: Create Directory Structure

```bash
cd examples/your_example
mkdir -p test/simulator/firmware test/simulator/scenarios
```

### Step 2: Create Firmware Adapter

Create `test/simulator/firmware/your_firmware.hpp`:

```cpp
#pragma once
#include "simulator/firmware/firmware_base.hpp"
#include <painlessMesh.h>

class YourFirmware : public FirmwareBase {
public:
    void setup(painlessMesh* mesh, Scheduler* userScheduler) override {
        mesh_ = mesh;
        
        // Copy your setup() logic from the .ino file
        mesh_->init("YourPrefix", "password", userScheduler, 5555);
        mesh_->onReceive([this](uint32_t from, String& msg) {
            // Your receive callback
        });
        
        // Add your tasks, etc.
    }

    void loop() override {
        // Copy your loop() logic
        if (mesh_) mesh_->update();
    }

    const char* getName() const override {
        return "YourExample";
    }

private:
    painlessMesh* mesh_;
};
```

### Step 3: Create Test Scenario

Create `test/simulator/scenarios/your_test.yaml`:

```yaml
simulation:
  name: "Your Example Test"
  duration: 60

nodes:
  - template: "your_firmware"
    count: 10

validation:
  - check: "all_nodes_connected"
    timeout: 30
```

### Step 4: Create CMakeLists.txt

Copy from `examples/basic/test/simulator/CMakeLists.txt` and adapt paths.

### Step 5: Run Test

```bash
cd test/simulator/build
./painlessmesh-simulator --config ../../../examples/your_example/test/simulator/scenarios/your_test.yaml
```

## Test Scenarios

### Available Validations

```yaml
validation:
  # Mesh formation
  - check: "all_nodes_connected"
    timeout: 30
  
  # Message delivery
  - check: "messages_delivered"
    min_messages_per_node: 5
    timeout: 60
  
  # Time synchronization
  - check: "time_synchronized"
    max_time_diff_ms: 10000
    timeout: 45
  
  # Custom metrics
  - check: "custom_metric"
    metric_name: "your_metric"
    min_value: 100
```

### Network Conditions

```yaml
network:
  latency:
    min_ms: 10
    max_ms: 100
  bandwidth_kbps: 256
  packet_loss_percent: 5

events:
  # Network partition
  - type: "network_partition"
    time: 30
    duration: 15
    groups: [[0,1,2], [3,4,5]]
  
  # Node failures
  - type: "node_crash"
    time: 45
    nodes: [2, 5]
  
  # Node recovery
  - type: "node_restart"
    time: 50
    nodes: [2, 5]
```

### Topology Options

```yaml
topology:
  type: "random"      # Random connections
  # OR
  type: "ring"        # Ring topology
  # OR
  type: "star"        # Star topology
  # OR
  type: "mesh"        # Full mesh
  # OR
  type: "tree"        # Tree topology
  
  connectivity: 0.7   # For random: 70% connectivity
```

## Metrics and Analysis

### Collected Metrics

The simulator automatically collects:
- Messages sent/received per node
- Topology changes
- Connection count
- Time synchronization drift
- Custom application metrics

### Output Format

Results are saved as CSV:

```csv
timestamp,node_id,messages_sent,messages_received,connections,time_drift_ms
0,6481,0,0,0,150000
1,6481,1,0,2,145000
2,6481,1,3,2,140000
...
```

### Analysis

```python
import pandas as pd

df = pd.read_csv('results/test_results.csv')

# Messages per node
print(df.groupby('node_id')['messages_received'].sum())

# Average connections
print(df.groupby('timestamp')['connections'].mean())

# Time sync performance
print(df.groupby('timestamp')['time_drift_ms'].max())
```

## CI/CD Integration

### GitHub Actions Integration

Simulator tests are integrated into the CI/CD pipeline in `.github/workflows/ci.yml`:

**The `simulator-tests` job:**
- Runs on every push and pull request
- Builds the simulator from the submodule
- Executes example test scenarios
- Uploads results as artifacts

**Configuration:**
```yaml
simulator-tests:
  name: Simulator Integration Tests
  runs-on: ubuntu-latest
  steps:
    - uses: actions/checkout@v4
      with:
        submodules: recursive
    
    - name: Install dependencies
      run: |
        sudo apt-get install cmake ninja-build libboost-dev libboost-program-options-dev libyaml-cpp-dev
    
    - name: Build simulator
      run: |
        cd test/simulator
        mkdir build && cd build
        cmake -G Ninja .. && ninja
    
    - name: Run tests
      run: |
        cd test/simulator/build
        ./painlessmesh-simulator --config \
          ../../../examples/basic/test/simulator/scenarios/basic_mesh_test.yaml
```

This ensures example sketches are validated on every code change.

## Examples with Simulator Tests

### Currently Available

- ✅ `examples/basic/` - Basic mesh formation and broadcasting

### Coming Soon

- ⏳ `examples/startHere/` - Getting started example
- ⏳ `examples/echoNode/` - Echo server/client
- ⏳ `examples/bridge/` - Internet bridge functionality
- ⏳ `examples/mqttBridge/` - MQTT integration

## Documentation

- **Simulator Repository**: https://github.com/Alteriom/painlessMesh-simulator
- **Getting Started**: [test/simulator/GETTING_STARTED.md](../test/simulator/GETTING_STARTED.md)
- **Integration Guide**: [test/simulator/docs/INTEGRATING_INTO_YOUR_PROJECT.md](../test/simulator/docs/INTEGRATING_INTO_YOUR_PROJECT.md)
- **Configuration Reference**: [test/simulator/docs/CONFIGURATION_GUIDE.md](../test/simulator/docs/CONFIGURATION_GUIDE.md)

## Troubleshooting

### Submodule not initialized

```bash
cd test
git submodule update --init simulator
```

### Build errors

```bash
# Check dependencies
sudo apt-get install cmake ninja-build libboost-dev libyaml-cpp-dev

# Clean rebuild
cd test/simulator
rm -rf build
mkdir build && cd build
cmake -G Ninja ..
ninja
```

### Simulation timeouts

Increase timeout in YAML:
```yaml
simulation:
  duration: 120  # Increase from 60
```

### Memory issues with large meshes

Reduce node count or increase system resources.

## Benefits

✅ **Fast iteration** - Test in seconds vs hours of hardware testing
✅ **Reproducible** - Same scenario always produces same results  
✅ **Scalable** - Test with 100+ nodes on a laptop
✅ **Automated** - Integrate with CI/CD
✅ **Cost-effective** - No hardware required
✅ **Realistic** - Same code runs on hardware and simulator

## Contributing

To add simulator tests for more examples:

1. Create test structure in `examples/your_example/test/simulator/`
2. Adapt the .ino logic into a firmware adapter
3. Create test scenarios with validation criteria
4. Document in README.md
5. Submit pull request

See existing examples for patterns to follow.
