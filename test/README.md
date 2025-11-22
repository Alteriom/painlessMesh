# painlessMesh Testing

This directory contains the comprehensive test suite for painlessMesh, including unit tests, integration tests, and full simulator-based validation.

## Test Structure

```
test/
├── boost/                    # Boost.Asio-based integration tests
│   ├── tcp_integration.cpp   # Core TCP/mesh integration tests
│   ├── connection.cpp        # Connection handling tests
│   └── Arduino.h             # Arduino API stubs for PC testing
├── catch/                    # Catch2 unit tests
│   ├── catch_*.cpp           # Various unit tests
│   ├── fake_serial.cpp       # Serial port stub for testing
│   └── README_*.md           # Test-specific documentation
├── include/                  # Test utilities and helpers
│   ├── catch_utils.hpp       # Catch2 test helpers
│   ├── fake_asynctcp.hpp     # AsyncTCP stub
│   └── fake_serial.hpp       # Serial stub header
├── simulator/                # painlessMesh-simulator (git submodule)
│   └── [Full simulator tool] # For testing with 100+ virtual nodes
├── ArduinoJson/              # ArduinoJson library (git submodule)
└── TaskScheduler/            # TaskScheduler library (git submodule)
```

## Types of Tests

### 1. Unit Tests (`test/catch/`)

Fast, focused tests for individual components:

- **Protocol tests**: Message serialization/deserialization
- **Buffer tests**: Memory management
- **Routing tests**: Message routing logic
- **Logger tests**: Logging functionality
- **Package tests**: Custom package types (Alteriom)

**Example:**
```bash
./bin/catch_protocol
./bin/catch_buffer
./bin/catch_alteriom_packages
```

### 2. Integration Tests (`test/boost/tcp_integration.cpp`, `test/boost/connection.cpp`)

Tests for core mesh functionality:

- TCP connection management
- Mesh formation and topology
- Message routing across multiple hops
- Time synchronization
- Network loop detection
- Disconnection handling

**Example:**
```bash
./bin/catch_tcp_integration
./bin/catch_connection
```

### 3. Simulator Tests (External Tool)

**Full-scale simulator** at `test/simulator/`:

- Tests actual example firmware with 100+ virtual nodes
- YAML-based test scenario configuration
- Realistic network conditions (latency, packet loss, partitions)
- Metrics collection and analysis
- CI/CD integration

**Example:**
```bash
cd test/simulator
mkdir build && cd build
cmake -G Ninja .. && ninja
bin/painlessmesh-simulator --config ../../../examples/basic/test/simulator/scenarios/basic_mesh_test.yaml
```

See [SIMULATOR_TESTING.md](../docs/SIMULATOR_TESTING.md) for complete documentation.

## Quick Start

### Prerequisites

```bash
# Ubuntu/Debian
sudo apt-get install cmake ninja-build libboost-dev libboost-system-dev

# macOS
brew install cmake ninja boost

# Windows (using vcpkg)
vcpkg install boost-system boost-asio
```

### Clone Test Dependencies

```bash
cd test
git clone https://github.com/bblanchon/ArduinoJson.git
git clone https://github.com/arkhipenko/TaskScheduler
cd ..
```

### Build All Tests

```bash
# Configure
cmake -G Ninja .

# Build
ninja

# Or without Ninja
cmake .
make
```

### Run All Tests

```bash
# Run all tests
run-parts --regex catch_ bin/

# Run with summary
run-parts --regex catch_ bin/ 2>&1 | tail -20
```

### Run Specific Test

```bash
# Run a single test
./bin/catch_example_basic

# Run with verbose output
./bin/catch_example_basic -s

# Run specific scenario
./bin/catch_example_basic "Basic example - message broadcasting works"

# List all scenarios
./bin/catch_example_basic -l
```

## CI/CD Integration

Tests run automatically on every commit via GitHub Actions:

```yaml
# .github/workflows/ci.yml
- name: Install dependencies
  run: |
    sudo apt-get update
    sudo apt-get install -y cmake ninja-build libboost-dev libboost-system-dev

- name: Clone test dependencies
  run: |
    cd test
    git clone https://github.com/bblanchon/ArduinoJson.git
    git clone https://github.com/arkhipenko/TaskScheduler

- name: Build tests
  run: cmake -G Ninja . && ninja

- name: Run tests
  run: run-parts --regex catch_ bin/
```

## Writing New Tests

### For New Features (Unit Tests)

1. Create `test/catch/catch_yourfeature.cpp`
2. Include required headers
3. Write SCENARIO blocks using Catch2
4. CMake auto-discovers `catch_*.cpp` files
5. Build and run: `ninja && ./bin/catch_yourfeature`

Example:
```cpp
#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"
#include "Arduino.h"

SCENARIO("Your feature works") {
    GIVEN("Initial state") {
        WHEN("Action occurs") {
            THEN("Expected result") {
                REQUIRE(true);
            }
        }
    }
}
```

### For New Examples (Simulator Tests)

1. Create `test/boost/catch_example_yourexample.cpp`
2. Include `simulator_utils.hpp`
3. Use `SimulatedMeshNetwork` for multi-node tests
4. Add executable to `CMakeLists.txt`
5. Build and run: `ninja && ./bin/catch_example_yourexample`

Example:
```cpp
#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"
#include "Arduino.h"
#include "simulator_utils.hpp"

WiFiClass WiFi;
ESPClass ESP;

using namespace painlessmesh;
painlessmesh::logger::LogClass Log;

SCENARIO("Example validates correctly") {
    using namespace logger;
    Log.setLogLevel(ERROR);
    
    Scheduler scheduler;
    boost::asio::io_context io_service;
    SimulatedMeshNetwork network(&scheduler, 3, io_service);
    
    // Your test logic here
    network.waitForFullMesh(5000);
    REQUIRE(layout::size(network[0]->asNodeTree()) == 3);
    
    network.stop();
}
```

See [SIMULATOR_TESTING.md](../docs/development/SIMULATOR_TESTING.md) for complete guide.

## Test Coverage Goals

### Current Coverage

- ✅ Core mesh functionality (TCP, routing, topology)
- ✅ Protocol serialization/deserialization
- ✅ Basic message broadcasting
- ✅ Time synchronization
- ✅ Node discovery and tracking
- ✅ Alteriom custom packages
- ✅ Bridge health monitoring
- ✅ Message queue functionality
- ✅ Priority messaging

### In Progress

- 🔄 Example sketch validation (3/30+ examples)
- 🔄 Bridge functionality testing
- 🔄 OTA update testing
- 🔄 MQTT integration testing

### Planned

- ⏳ ESP32-specific features
- ⏳ Memory leak detection
- ⏳ Performance benchmarks
- ⏳ Stress testing (large meshes, high traffic)
- ⏳ All example sketches validated

## Debugging Tests

### Enable Verbose Logging

```cpp
Log.setLogLevel(ERROR | STARTUP | CONNECTION | SYNC | COMMUNICATION);
```

### Run with Catch2 Debugging

```bash
# Show all assertions
./bin/catch_example_basic -s

# Break on first failure
./bin/catch_example_basic -a

# Run only failed tests
./bin/catch_example_basic --last

# Time tests
./bin/catch_example_basic -d yes
```

### Common Issues

**Test hangs:**
- Reduce timeout values
- Check if `waitForFullMesh()` succeeds
- Add delays between operations

**Boost not found:**
```bash
# Set CMAKE_PREFIX_PATH
cmake -G Ninja -DCMAKE_PREFIX_PATH=/usr/local .
```

**Random failures:**
- Increase iteration counts
- Add delays between mesh operations
- Check for race conditions

## Best Practices

### 1. Write Tests First

For new features, write the test before the implementation:

```cpp
// This test will fail initially
SCENARIO("New feature works") {
    // Test the behavior you want
}

// Then implement the feature to make it pass
```

### 2. Test Edge Cases

Don't just test the happy path:

```cpp
SCENARIO("Handles disconnection gracefully") {
    // Test what happens when things go wrong
}
```

### 3. Keep Tests Fast

- Use smaller meshes when possible (2-3 nodes)
- Use appropriate timeouts
- Don't test unrelated features in one scenario

### 4. Make Tests Deterministic

- Don't rely on timing that might vary
- Use `waitUntil()` instead of fixed delays
- Set log levels to reduce output noise

### 5. Document Complex Tests

Add comments explaining what you're testing and why:

```cpp
// This test validates the fix for issue #123
// where nodes would disconnect after 5 minutes
SCENARIO("Long-running mesh stability") {
    // ...
}
```

## Continuous Integration

Tests must pass for PRs to be merged:

1. All unit tests pass
2. All integration tests pass
3. All simulator tests pass
4. No memory leaks detected
5. Code coverage meets minimum threshold

Check test status:
- GitHub Actions: https://github.com/Alteriom/painlessMesh/actions
- Latest build: Click the green checkmark on your commit

## Contributing

When contributing to painlessMesh:

1. ✅ Add tests for new features
2. ✅ Update existing tests if behavior changes
3. ✅ Ensure all tests pass locally
4. ✅ Add simulator tests for new examples
5. ✅ Document test requirements in PR

See [CONTRIBUTING.md](../CONTRIBUTING.md) for more details.

## Resources

- **Catch2 Documentation**: https://github.com/catchorg/Catch2/tree/v2.x
- **Boost.Asio**: https://www.boost.org/doc/libs/release/doc/html/boost_asio.html
- **Simulator Testing Guide**: [SIMULATOR_TESTING.md](../docs/development/SIMULATOR_TESTING.md)
- **Testing Summary**: [TESTING_SUMMARY.md](../docs/development/TESTING_SUMMARY.md)

## Questions?

- Open an issue: https://github.com/Alteriom/painlessMesh/issues
- Check existing tests for patterns
- Read the simulator testing guide
- Ask in pull request comments
