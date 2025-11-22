# Simulator-Based Testing Framework

## Overview

The painlessMesh library includes a comprehensive simulator-based testing framework that validates example sketches and library functionality in a controlled, deterministic environment. This framework uses Boost.Asio to create virtual mesh networks that can be tested without physical hardware.

## Why Simulator Testing?

Simulator testing provides several key advantages:

- **Regression Prevention**: Automatically catches bugs before they reach production
- **Example Validation**: Ensures all example sketches work as documented
- **Deterministic Testing**: Reproducible tests without WiFi interference or hardware variability
- **CI/CD Integration**: Runs automatically on every commit
- **Fast Iteration**: Test complex mesh scenarios in seconds, not minutes
- **Coverage**: Tests real mesh behavior including routing, time sync, and topology

## Architecture

### Components

1. **Boost.Asio Networking**: Simulates TCP/IP connections between virtual nodes
2. **SimulatedMeshNode**: Wraps painlessMesh with testing utilities
3. **SimulatedMeshNetwork**: Manages multiple nodes in a test scenario
4. **Test Utilities**: Helper functions for common testing patterns

### File Structure

```
test/
├── boost/                          # Boost-based simulator tests
│   ├── catch_example_basic.cpp     # Tests for basic.ino example
│   ├── catch_example_starthere.cpp # Tests for startHere.ino example
│   ├── catch_example_timesync.cpp  # Time synchronization tests
│   └── tcp_integration.cpp         # Original integration tests
├── include/
│   └── simulator_utils.hpp         # Reusable simulator utilities
└── catch/                          # Unit tests (Catch2)
```

## Using the Simulator

### Basic Example

```cpp
#include "simulator_utils.hpp"

SCENARIO("Test message broadcasting") {
    Scheduler scheduler;
    boost::asio::io_context io_service;
    
    // Create a 3-node network
    SimulatedMeshNetwork network(&scheduler, 3, io_service);
    
    // Wait for mesh formation
    network.waitForFullMesh(5000);
    
    // Send a message
    network[0]->sendBroadcast("Hello mesh!");
    network.runFor(1000);
    
    // Verify delivery
    REQUIRE(network[1]->getMessagesReceived() >= 1);
    REQUIRE(network[2]->getLastMessage() == "Hello mesh!");
}
```

### Advanced Example

```cpp
SCENARIO("Test time synchronization") {
    Scheduler scheduler;
    boost::asio::io_context io_service;
    
    // Create network with 8-12 random nodes
    auto nodeCount = runif(8, 12);
    SimulatedMeshNetwork network(&scheduler, nodeCount, io_service);
    
    // Measure initial time differences
    int initialDiff = 0;
    for (size_t i = 0; i < network.size() - 1; ++i) {
        initialDiff += std::abs((int)network[0]->getNodeTime() -
                               (int)network[i + 1]->getNodeTime());
    }
    initialDiff /= network.size();
    
    // Run time sync protocol
    network.runFor(10000, 10);
    
    // Measure final time differences
    int finalDiff = 0;
    for (size_t i = 0; i < network.size() - 1; ++i) {
        finalDiff += std::abs((int)network[0]->getNodeTime() -
                             (int)network[i + 1]->getNodeTime());
    }
    finalDiff /= network.size();
    
    // Verify synchronization
    REQUIRE(finalDiff < initialDiff);
    REQUIRE(finalDiff < 10000); // Within 10ms average
}
```

## API Reference

### SimulatedMeshNode

A test wrapper around painlessMesh with built-in tracking and utilities.

#### Constructor

```cpp
SimulatedMeshNode(Scheduler *scheduler, size_t nodeId, 
                  boost::asio::io_context &io)
```

#### Key Methods

- `connectTo(SimulatedMeshNode &otherNode)` - Connect to another node
- `setReceiveCallback(callback)` - Set custom message receive handler
- `setConnectionCallback(callback)` - Set custom connection change handler
- `getMessagesReceived()` - Get count of received messages
- `getLastMessage()` - Get content of last received message
- `getLastMessageFrom()` - Get sender ID of last message
- `getConnectionsChanged()` - Get count of connection changes
- `resetCounters()` - Reset all counters to zero

### SimulatedMeshNetwork

Manages a collection of simulated nodes forming a mesh network.

#### Constructor

```cpp
SimulatedMeshNetwork(Scheduler *scheduler, size_t numNodes, 
                    boost::asio::io_context &io, size_t baseId = 6481)
```

#### Key Methods

- `update()` - Update all nodes (call in a loop)
- `stop()` - Stop all nodes
- `size()` - Get number of nodes
- `operator[](size_t index)` - Get node by index
- `getByNodeId(uint32_t nodeId)` - Get node by its mesh ID
- `runFor(iterations, delayMs)` - Run simulation for N iterations
- `waitForFullMesh(timeout)` - Wait until all nodes see full mesh
- `waitUntil(condition, timeout)` - Wait until condition is met

## Testing Patterns

### 1. Mesh Formation

```cpp
GIVEN("A mesh network") {
    SimulatedMeshNetwork network(&scheduler, 5, io_service);
    
    WHEN("Network stabilizes") {
        bool formed = network.waitForFullMesh(10000);
        
        THEN("All nodes see full topology") {
            REQUIRE(formed);
            for (size_t i = 0; i < 5; ++i) {
                REQUIRE(layout::size(network[i]->asNodeTree()) == 5);
            }
        }
    }
}
```

### 2. Message Delivery

```cpp
GIVEN("Connected nodes") {
    SimulatedMeshNetwork network(&scheduler, 3, io_service);
    network.waitForFullMesh(5000);
    
    WHEN("Message is broadcast") {
        network[0]->sendBroadcast("Test message");
        network.runFor(1000);
        
        THEN("All other nodes receive it") {
            REQUIRE(network[1]->getLastMessage() == "Test message");
            REQUIRE(network[2]->getLastMessage() == "Test message");
        }
    }
}
```

### 3. Custom Callbacks

```cpp
GIVEN("Node with custom callback") {
    SimulatedMeshNetwork network(&scheduler, 2, io_service);
    
    size_t callbackCount = 0;
    std::string lastMsg;
    
    network[0]->setReceiveCallback([&](uint32_t from, std::string msg) {
        callbackCount++;
        lastMsg = msg;
    });
    
    WHEN("Messages arrive") {
        network.waitForFullMesh(2000);
        network[1]->sendBroadcast("Hello");
        network.runFor(500);
        
        THEN("Callback is invoked") {
            REQUIRE(callbackCount > 0);
            REQUIRE(lastMsg == "Hello");
        }
    }
}
```

### 4. Time Synchronization

```cpp
GIVEN("Mesh with time sync") {
    SimulatedMeshNetwork network(&scheduler, 4, io_service);
    
    WHEN("Sync protocol runs") {
        network.waitForFullMesh(5000);
        network.runFor(10000, 10);
        
        THEN("Times converge") {
            int maxDiff = 0;
            for (size_t i = 1; i < 4; ++i) {
                int diff = std::abs((int)network[0]->getNodeTime() -
                                   (int)network[i]->getNodeTime());
                maxDiff = std::max(maxDiff, diff);
            }
            REQUIRE(maxDiff < 50000); // Within 50ms
        }
    }
}
```

### 5. Disconnection Handling

```cpp
GIVEN("Formed mesh") {
    SimulatedMeshNetwork network(&scheduler, 3, io_service);
    network.waitForFullMesh(5000);
    
    WHEN("Connection breaks") {
        if (network[0]->subs.size() > 0) {
            (*network[0]->subs.begin())->close();
            network.runFor(1000);
            
            THEN("Topology updates") {
                REQUIRE(layout::size(network[0]->asNodeTree()) < 3);
            }
        }
    }
}
```

## Building and Running Tests

### Prerequisites

- CMake 3.10+
- Boost library (system component)
- Ninja build system (optional but recommended)

### Build Tests

```bash
# Clone dependencies if needed
cd test
git clone https://github.com/bblanchon/ArduinoJson.git
git clone https://github.com/arkhipenko/TaskScheduler
cd ..

# Configure and build
cmake -G Ninja .
ninja

# Or without Ninja
cmake .
make
```

### Run Tests

```bash
# Run all tests
run-parts --regex catch_ bin/

# Run specific test
./bin/catch_example_basic
./bin/catch_example_starthere
./bin/catch_example_timesync

# Run with verbose output
./bin/catch_example_basic -s

# Run specific test case
./bin/catch_example_basic "Basic example - message broadcasting works"
```

### CI/CD Integration

Tests run automatically on GitHub Actions for every commit:

```yaml
# .github/workflows/ci.yml
- name: Build tests
  run: cmake -G Ninja . && ninja

- name: Run tests
  run: run-parts --regex catch_ bin/
```

## Writing New Tests

### 1. Choose Test File

- Use `test/boost/catch_example_*.cpp` for example validations
- Use descriptive names matching the example being tested

### 2. Follow the Pattern

```cpp
#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"
#include "Arduino.h"
#include "simulator_utils.hpp"

WiFiClass WiFi;
ESPClass ESP;

using namespace painlessmesh;
painlessmesh::logger::LogClass Log;

SCENARIO("Descriptive test name") {
    using namespace logger;
    Log.setLogLevel(ERROR);
    
    Scheduler scheduler;
    boost::asio::io_context io_service;
    
    GIVEN("Initial setup") {
        // Setup code
        
        WHEN("Action occurs") {
            // Test action
            
            THEN("Expected outcome") {
                REQUIRE(/* assertion */);
            }
        }
    }
}
```

### 3. Add to CMakeLists.txt

```cmake
add_executable(catch_example_yourtest test/boost/catch_example_yourtest.cpp 
               test/catch/fake_serial.cpp src/scheduler.cpp)
target_include_directories(catch_example_yourtest PUBLIC 
                          test/include/ test/boost/ test/ArduinoJson/src/ 
                          test/TaskScheduler/src/ src/)
TARGET_LINK_LIBRARIES(catch_example_yourtest ${Boost_LIBRARIES})
```

### 4. Test Your Test

```bash
ninja
./bin/catch_example_yourtest -s
```

## Best Practices

### 1. Use Descriptive Scenarios

❌ Bad:
```cpp
SCENARIO("Test 1") {
```

✅ Good:
```cpp
SCENARIO("Basic example - message broadcasting works") {
```

### 2. Test One Concept Per Scenario

Each SCENARIO should test a single, clear concept or feature.

### 3. Use Appropriate Timeouts

- Mesh formation: 5,000-10,000 iterations
- Message delivery: 500-1,000 iterations
- Time sync: 10,000+ iterations

### 4. Clean Up Resources

Always call `network.stop()` at the end of tests.

### 5. Use Random Variations

Use `runif()` to test with varying network sizes:

```cpp
auto nodeCount = runif(8, 12);
SimulatedMeshNetwork network(&scheduler, nodeCount, io_service);
```

### 6. Check Both Success and Failure

Test both expected successes and how failures are handled:

```cpp
WHEN("Connection breaks") {
    // Test disconnection handling
}
```

## Troubleshooting

### Test Hangs

If a test hangs, it's usually waiting for a condition that never occurs:

- Reduce timeout values
- Add debug logging: `Log.setLogLevel(ERROR | COMMUNICATION);`
- Check if mesh formation succeeded with `waitForFullMesh()`

### Flaky Tests

If tests pass sometimes and fail others:

- Increase iteration counts for mesh operations
- Add delays between operations
- Check for race conditions in callbacks

### Build Errors

If CMake can't find Boost:

```bash
# Ubuntu/Debian
sudo apt-get install libboost-dev libboost-system-dev

# macOS
brew install boost

# Windows
# Download from boost.org or use vcpkg
```

## Examples Covered

Current simulator tests cover:

- ✅ `basic.ino` - Message broadcasting and callbacks
- ✅ `startHere.ino` - Node lists, time sync, delay measurement
- ✅ Time synchronization - Comprehensive sync validation

### Planned Coverage

- [ ] `bridge.ino` - Bridge to internet functionality
- [ ] `bridge_failover` - Automatic bridge election
- [ ] `otaSender`/`otaReceiver` - OTA updates
- [ ] `priority` examples - Priority messaging
- [ ] Alteriom examples - Custom package testing
- [ ] MQTT examples - MQTT bridge functionality

## Contributing

When adding new examples to the repository:

1. Create corresponding simulator tests
2. Follow the patterns in existing test files
3. Ensure tests pass locally before committing
4. Document any special testing requirements

## References

- [Catch2 Documentation](https://github.com/catchorg/Catch2)
- [Boost.Asio Documentation](https://www.boost.org/doc/libs/release/doc/html/boost_asio.html)
- [painlessMesh API](../../README.md)
- [CONTRIBUTING.md](../../CONTRIBUTING.md)
