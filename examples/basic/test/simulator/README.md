# Basic Example - Simulator Tests

This directory contains simulator-based tests for the `basic.ino` example using the [painlessMesh-simulator](https://github.com/Alteriom/painlessMesh-simulator).

## Overview

The simulator allows testing the basic example with 10+ virtual nodes without physical hardware, validating:
- Mesh formation with multiple nodes
- Message broadcasting to all nodes
- Callback functionality
- Dynamic node joining
- Time synchronization

## Quick Start

### Prerequisites

Install dependencies (Ubuntu/Debian):
```bash
sudo apt-get install cmake ninja-build libboost-dev libboost-program-options-dev libyaml-cpp-dev
```

### Initialize Submodule

If not already done:
```bash
cd ../../../../test
git submodule update --init simulator
cd -
```

### Build and Run

```bash
# Build simulator
mkdir build && cd build
cmake -G Ninja ..
ninja

# Run test scenario
./painlessmesh-simulator --config ../scenarios/basic_mesh_test.yaml

# Or using the simulator from test/simulator
cd ../../../../test/simulator
mkdir -p build && cd build
cmake -G Ninja ..
ninja
./painlessmesh-simulator --config ../../../examples/basic/test/simulator/scenarios/basic_mesh_test.yaml
```

## Test Scenarios

### basic_mesh_test.yaml

Tests basic functionality with 10 nodes:
- ✓ Mesh formation
- ✓ Message broadcasting
- ✓ Callback triggering
- ✓ Dynamic node joining (node added at 30s)
- ✓ Time synchronization

**Expected Results:**
- All nodes connect within 30 seconds
- Each node receives 5+ messages within 60 seconds
- Time differences < 10ms after 45 seconds

## Firmware Adapter

The `firmware/basic_firmware.hpp` file wraps the `basic.ino` logic for simulation:

```cpp
#include "simulator/firmware/firmware_base.hpp"

class BasicFirmware : public FirmwareBase {
    // Implements same logic as basic.ino
    // - setup() initializes mesh and callbacks
    // - loop() calls mesh.update()
    // - sendMessage() broadcasts periodically
};
```

This allows running the **exact same code** that runs on hardware.

## Adding More Tests

Create new YAML scenarios in `scenarios/`:

```yaml
simulation:
  name: "Stress Test"
  duration: 120

nodes:
  - template: "basic_example"
    count: 50  # Test with 50 nodes
    
topology:
  type: "ring"  # Different topology

events:
  - type: "network_partition"  # Inject failures
    time: 60
```

## Metrics and Validation

Test results are saved to `results/basic_test_results.csv`:

```csv
timestamp,node_id,messages_sent,messages_received,topology_changes
0,6481,0,0,1
1,6481,1,0,1
2,6481,1,2,1
...
```

Analyze with:
```python
import pandas as pd
df = pd.read_csv('results/basic_test_results.csv')
print(df.groupby('node_id')['messages_received'].sum())
```

## Documentation

- [Simulator Documentation](../../../../test/simulator/README.md)
- [Integration Guide](../../../../test/simulator/docs/INTEGRATING_INTO_YOUR_PROJECT.md)
- [Configuration Reference](../../../../test/simulator/docs/CONFIGURATION_GUIDE.md)

## Troubleshooting

**Simulator not found:**
```bash
cd ../../../../test
git submodule update --init simulator
```

**Build errors:**
```bash
# Check dependencies
sudo apt-get install libboost-dev libyaml-cpp-dev

# Clean build
rm -rf build && mkdir build && cd build
cmake -G Ninja ..
```

**Test failures:**
Check the simulator output for details on which validation failed.
