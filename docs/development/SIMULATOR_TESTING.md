# Simulator-Based Testing Framework

## Overview

The painlessMesh library includes a Boost.Asio-based simulator for testing mesh functionality without physical hardware. The simulator classes (`MeshTest` and `Nodes`) were originally created for integration testing and have now been extracted into a reusable header (`mesh_simulator.hpp`) for testing all examples.

## Architecture

The simulator uses Boost.Asio to create virtual mesh nodes that communicate over TCP/IP on localhost. This provides:

- **Deterministic testing**: No WiFi interference or hardware variability
- **Fast execution**: Tests run in seconds instead of minutes
- **Reproducible results**: Same code always produces same behavior
- **Easy debugging**: All nodes run in same process

## Simulator Classes

### MeshTest

`MeshTest` extends `painlessMesh::Mesh` with Boost.Asio networking:

```cpp
MeshTest node(&scheduler, nodeId, io_context);
node.connect(otherNode);  // Connect to another node
node.sendBroadcast("Hello");
node.sendSingle(targetId, "Direct message");
```

### Nodes

`Nodes` manages multiple `MeshTest` nodes, automatically connecting them into a mesh:

```cpp
Nodes n(&scheduler, 5, io_context);  // Create 5-node mesh

// Access individual nodes
n.nodes[0]->sendBroadcast("Message");

// Update all nodes
n.update();  // Updates all nodes and polls io_context

// Clean shutdown
n.stop();
```

## Writing Tests

### Basic Pattern

```cpp
#include "mesh_simulator.hpp"

SCENARIO("Example test") {
    using namespace logger;
    Log.setLogLevel(ERROR);

    Scheduler scheduler;
    boost::asio::io_context io_service;
    Nodes n(&scheduler, 3, io_service);

    // Let mesh form
    for (auto i = 0; i < 5000; ++i) {
        n.update();
        delay(10);
    }
    
    // Verify topology
    REQUIRE(layout::size(n.nodes[0]->asNodeTree()) == 3);
    
    // Test messaging
    size_t received = 0;
    n.nodes[1]->onReceive([&](uint32_t from, std::string msg) {
        received++;
    });
    
    n.nodes[0]->sendBroadcast("Test");
    
    for (auto i = 0; i < 1000; ++i) {
        n.update();
        delay(10);
    }
    
    REQUIRE(received >= 1);
    n.stop();
}
```

### Testing Broadcasts

```cpp
Nodes n(&scheduler, 4, io_service);

// Setup receivers
size_t count[4] = {0};
for (size_t i = 0; i < 4; ++i) {
    n.nodes[i]->onReceive([&, i](uint32_t from, std::string msg) {
        count[i]++;
    });
}

// Send broadcast
n.nodes[0]->sendBroadcast("Hello all");

// Process messages
for (auto i = 0; i < 1000; ++i) {
    n.update();
    delay(10);
}

// Verify delivery (sender doesn't receive own broadcast)
REQUIRE(count[1] >= 1);
REQUIRE(count[2] >= 1);
REQUIRE(count[3] >= 1);
```

### Testing Point-to-Point

```cpp
Nodes n(&scheduler, 3, io_service);

std::string received;
n.nodes[2]->onReceive([&](uint32_t from, std::string msg) {
    received = msg;
});

n.nodes[0]->sendSingle(n.nodes[2]->getNodeId(), "Direct");

for (auto i = 0; i < 1000; ++i) {
    n.update();
    delay(10);
}

REQUIRE(received == "Direct");
```

### Testing Time Sync

```cpp
Nodes n(&scheduler, 10, io_service);

// Initial time differences are large
int initialDiff = 0;
for (size_t i = 0; i < n.size() - 1; ++i) {
    initialDiff += std::abs((int)n.nodes[0]->getNodeTime() -
                           (int)n.nodes[i + 1]->getNodeTime());
}
initialDiff /= n.size();

// Run time sync
for (auto i = 0; i < 10000; ++i) {
    n.update();
    delay(10);
}

// Final differences are small
int finalDiff = 0;
for (size_t i = 0; i < n.size() - 1; ++i) {
    finalDiff += std::abs((int)n.nodes[0]->getNodeTime() -
                         (int)n.nodes[i + 1]->getNodeTime());
}
finalDiff /= n.size();

REQUIRE(finalDiff < initialDiff);
REQUIRE(finalDiff < 10000);  // Within 10ms average
```

## Building and Running

### Prerequisites

```bash
sudo apt-get install cmake ninja-build libboost-system-dev
```

### Clone Dependencies

```bash
cd test
git clone https://github.com/bblanchon/ArduinoJson.git
git clone https://github.com/arkhipenko/TaskScheduler
cd ..
```

### Build and Run

```bash
cmake -G Ninja .
ninja

# Run specific test
./bin/catch_example_basic

# Run all tests
run-parts --regex catch_ bin/
```

## Test Organization

- `test/include/mesh_simulator.hpp` - Simulator class definitions
- `test/boost/tcp_integration.cpp` - Original integration tests
- `test/boost/catch_example_*.cpp` - Example validation tests

## Adding New Tests

1. Create `test/boost/catch_example_yourtest.cpp`
2. Include `mesh_simulator.hpp`
3. Use `MeshTest` or `Nodes` classes
4. Follow existing test patterns
5. Add to `CMakeLists.txt`:

```cmake
add_executable(catch_example_yourtest 
    test/boost/catch_example_yourtest.cpp 
    test/catch/fake_serial.cpp src/scheduler.cpp)
target_include_directories(catch_example_yourtest PUBLIC 
    test/include/ test/boost/ test/ArduinoJson/src/ 
    test/TaskScheduler/src/ src/)
TARGET_LINK_LIBRARIES(catch_example_yourtest ${Boost_LIBRARIES})
```

## Best Practices

1. **Use appropriate timeouts**: Mesh formation needs 5000-10000 iterations
2. **Call update() and delay()**: Use `n.update(); delay(10);` pattern
3. **Clean up**: Always call `n.stop()` at test end
4. **Set log level**: Use `Log.setLogLevel(ERROR);` to reduce noise
5. **Test one concept per scenario**: Keep tests focused

## Troubleshooting

### Tests hang
- Increase iteration count for mesh formation
- Check that callbacks don't block

### Messages not delivered
- Ensure mesh has time to form before sending
- Use sufficient iterations for message propagation

### Build errors
- Verify Boost is installed
- Check test dependencies are cloned
- Ensure CMake configuration succeeded

## References

- Original simulator: `test/boost/tcp_integration.cpp`
- Simulator header: `test/include/mesh_simulator.hpp`
- Example tests: `test/boost/catch_example_*.cpp`
