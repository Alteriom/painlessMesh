# Testing painlessMesh Examples with the Simulator

## Quick Start Guide

This guide shows how to validate painlessMesh examples using the integrated simulator.

### Prerequisites

```bash
# Ubuntu/Debian
sudo apt-get install cmake ninja-build libboost-dev libyaml-cpp-dev

# macOS
brew install cmake ninja boost yaml-cpp
```

### Initialize Simulator

```bash
# Clone with submodules (if starting fresh)
git clone --recursive https://github.com/Alteriom/painlessMesh.git

# OR initialize if already cloned
cd painlessMesh
git submodule update --init test/simulator
```

### Run Basic Example Test

```bash
# Build simulator
cd test/simulator
mkdir build && cd build
cmake -G Ninja ..
ninja

# Run basic example validation
./painlessmesh-simulator --config \
  ../../../examples/basic/test/simulator/scenarios/basic_mesh_test.yaml
```

### Expected Output

```
Starting simulation: Basic Example - Mesh Formation and Broadcasting
Duration: 60 seconds

[00:00] Initializing 10 nodes...
[00:05] Node 6481 connected to mesh
[00:08] Node 6482 connected to mesh
[00:12] Node 6483 connected to mesh
...
[00:30] ✓ All nodes connected (10/10)
[00:45] ✓ Messages delivered (avg 8.5 messages/node)
[00:60] ✓ Time synchronized (max drift: 5.2ms)

PASS: All validation criteria met
Results saved to: results/basic_test_results.csv
```

## What Gets Tested

### Basic Example Test Scenario

The `basic_mesh_test.yaml` scenario validates:

1. **Mesh Formation** ✓
   - 10 virtual nodes form a connected mesh
   - All nodes discover each other within 30 seconds

2. **Message Broadcasting** ✓
   - Each node periodically sends broadcasts
   - All nodes receive messages from others
   - Minimum 5 messages delivered per node

3. **Time Synchronization** ✓
   - Node clocks start with random offsets
   - Time sync protocol converges within 45 seconds
   - Final time differences < 10ms

4. **Dynamic Topology** ✓
   - New node joins at 30 seconds
   - Network adapts and includes new node
   - Messages continue to flow

## Test Configuration

Edit `examples/basic/test/simulator/scenarios/basic_mesh_test.yaml`:

```yaml
simulation:
  duration: 60  # Test duration in seconds

nodes:
  - template: "basic_example"
    count: 10   # Number of virtual nodes

topology:
  type: "random"
  connectivity: 0.7  # 70% mesh connectivity

validation:
  - check: "all_nodes_connected"
    timeout: 30
  - check: "messages_delivered"
    min_messages_per_node: 5
```

## Analyzing Results

Results are saved to CSV:

```bash
cat results/basic_test_results.csv
```

```csv
timestamp,node_id,messages_sent,messages_received,connections
0,6481,0,0,0
1,6481,1,0,2
5,6481,1,3,3
10,6481,2,7,3
...
```

Use Python/Excel/etc to analyze:

```python
import pandas as pd

df = pd.read_csv('results/basic_test_results.csv')
print(f"Total messages: {df['messages_sent'].sum()}")
print(f"Avg per node: {df.groupby('node_id')['messages_received'].max().mean()}")
```

## Adding More Tests

### Test with 50 Nodes

Copy and modify the scenario:

```bash
cp examples/basic/test/simulator/scenarios/basic_mesh_test.yaml \
   examples/basic/test/simulator/scenarios/stress_test.yaml
```

Edit `stress_test.yaml`:
```yaml
nodes:
  - template: "basic_example"
    count: 50  # Scale up!
```

Run it:
```bash
./painlessmesh-simulator --config \
  ../../../examples/basic/test/simulator/scenarios/stress_test.yaml
```

### Inject Network Failures

Add events to `basic_mesh_test.yaml`:

```yaml
events:
  # Partition network at 30s
  - type: "network_partition"
    time: 30
    duration: 15
    groups: [[0,1,2,3,4], [5,6,7,8,9]]
  
  # Heal at 45s
  - type: "network_heal"
    time: 45
```

This tests if the mesh recovers from partitions!

## Testing Other Examples

To test other examples (startHere, echo, etc):

1. Create `test/simulator/` directory in the example
2. Copy firmware adapter pattern from `basic/`
3. Create YAML scenario
4. Run test

See [docs/SIMULATOR_TESTING.md](docs/SIMULATOR_TESTING.md) for detailed instructions.

## CI/CD Integration

Add to `.github/workflows/ci.yml`:

```yaml
- name: Test examples with simulator
  run: |
    cd test/simulator/build
    ./painlessmesh-simulator --config \
      ../../../examples/basic/test/simulator/scenarios/basic_mesh_test.yaml
```

## Troubleshooting

### Simulator not found

```bash
git submodule update --init test/simulator
```

### Build fails

```bash
# Check dependencies
sudo apt-get install cmake ninja-build libboost-dev libyaml-cpp-dev

# Clean rebuild
cd test/simulator
rm -rf build && mkdir build && cd build
cmake -G Ninja .. && ninja
```

### Test times out

Increase duration in YAML:
```yaml
simulation:
  duration: 120  # Give more time
```

### Need more details

Enable verbose logging:
```bash
./painlessmesh-simulator --config test.yaml --verbose
```

## Documentation

- **Complete Guide**: [docs/SIMULATOR_TESTING.md](docs/SIMULATOR_TESTING.md)
- **Simulator Repo**: https://github.com/Alteriom/painlessMesh-simulator
- **Getting Started**: [test/simulator/GETTING_STARTED.md](test/simulator/GETTING_STARTED.md)
- **Configuration**: [test/simulator/docs/CONFIGURATION_GUIDE.md](test/simulator/docs/CONFIGURATION_GUIDE.md)

## Benefits of Simulator Testing

✅ **Fast** - Complete test in 60 seconds vs hours with hardware
✅ **Scalable** - Test with 100+ nodes on a laptop  
✅ **Reproducible** - Same scenario always gives same results
✅ **Realistic** - Actual firmware code runs in simulated environment
✅ **Automated** - Integrate with CI/CD
✅ **Cost-effective** - No hardware required

## Next Steps

1. Run the basic example test (see above)
2. Experiment with different scenarios
3. Add tests for other examples you use
4. Integrate into your CI/CD pipeline
5. Share your scenarios with the community!
