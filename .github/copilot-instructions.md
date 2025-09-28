# GitHub Copilot Instructions for painlessMesh

This repository is a fork of the painlessMesh library specifically tailored for Alteriom's needs. painlessMesh is a user-friendly library for creating mesh networks with ESP8266 and ESP32 devices.

## Project Overview

painlessMesh handles routing and network management automatically, allowing developers to focus on their applications. The library uses JSON-based messaging and syncs time across all nodes, making it ideal for coordinated behaviors like synchronized light displays or sensor networks reporting to a central node.

### Key Technologies
- **C++14** standard with Arduino framework support
- **ESP8266/ESP32** platforms (espressif8266, espressif32)
- **Boost.Asio** for networking (PC testing)
- **ArduinoJson** for message serialization
- **TaskScheduler** for task management
- **CMake** build system with Ninja generator
- **Catch2** testing framework

## Development Environment Setup

### Prerequisites
- CMake 3.10 or higher
- Ninja build system
- Boost development libraries (`libboost-dev`, `libboost-system-dev`)
- C++14 compatible compiler

### Building the Project
```bash
# Initialize and update submodules (if using git submodules)
git submodule init
git submodule update

# Configure with CMake
cmake -G Ninja .

# Build
ninja
```

### Running Tests
```bash
# Run all tests
run-parts --regex catch_ bin/

# Run specific test suites
./bin/catch_base64
./bin/catch_plugin
./bin/catch_protocol
./bin/catch_tcp_integration
```

## Code Structure

### Core Components
- `src/painlessmesh/` - Core mesh networking logic
  - `mesh.hpp` - Main mesh class and interfaces
  - `tcp.hpp` - TCP connection handling
  - `protocol.hpp` - Message protocol definitions
  - `plugin.hpp` - Plugin system for extensibility
  - `logger.hpp` - Logging infrastructure
  - `router.hpp` - Message routing logic

### Test Infrastructure
- `test/catch/` - Unit tests using Catch2 framework
- `test/boost/` - Integration tests using Boost.Asio
- `test/include/` - Test utilities and mocks

### Key Files to Understand
- `CMakeLists.txt` - Build configuration
- `library.json` - PlatformIO library definition
- `src/painlessmesh/configuration.hpp` - Build-time configuration
- `test/catch/Arduino.h` - Arduino API mocking for PC tests

## Alteriom-Specific Considerations

When working on this repository for Alteriom's needs:

### Custom Package Development
- Extend `plugin::SinglePackage` or `plugin::BroadcastPackage` for custom message types
- Use the plugin system to add domain-specific functionality
- Follow the pattern shown in `test/catch/catch_plugin.cpp`

### Performance Optimization
- Use the performance plugin (`plugin/performance.hpp`) for monitoring
- Consider mesh topology and node limits (MAX_CONN configuration)
- Monitor time synchronization accuracy for coordinated behaviors

### Testing Strategy
- Write unit tests in `test/catch/` following existing patterns
- Use integration tests in `test/boost/` for network behavior
- Mock Arduino APIs using the existing test infrastructure

## Best Practices

### Code Style
- Follow the existing C++ style (use `.clang-format` configuration)
- Use meaningful variable and function names
- Add documentation for public APIs

### Mesh Network Design
- Design for fault tolerance (nodes can join/leave dynamically)
- Use appropriate message types (Single vs Broadcast)
- Consider network bandwidth and message frequency
- Implement proper error handling for network operations

### Memory Management
- Be mindful of memory constraints on ESP devices
- Use smart pointers appropriately
- Consider string vs TSTRING usage based on platform

## Common Tasks

### Adding a New Message Type
1. Create a class inheriting from `plugin::SinglePackage` or `plugin::BroadcastPackage`
2. Implement JSON serialization methods
3. Add type constant and routing configuration
4. Write tests following `catch_plugin.cpp` pattern

### Extending Network Functionality
1. Use the plugin system in `plugin.hpp`
2. Follow patterns in existing plugins (`performance.hpp`, `remote.hpp`)
3. Test with both unit tests and integration tests

### Debugging Network Issues
1. Use the logging system (`logger.hpp`) with appropriate log levels
2. Test with `catch_tcp_integration` scenarios
3. Monitor node connectivity and message routing

## Architecture Notes

- The library supports both Arduino (ESP8266/ESP32) and PC environments
- PC testing uses Boost.Asio for networking simulation
- Time synchronization is crucial for coordinated mesh behavior
- The plugin system allows for modular extensions without core changes