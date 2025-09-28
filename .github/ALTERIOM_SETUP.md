# Alteriom painlessMesh Setup Guide

This document explains the complete setup for using GitHub Copilot and MCP (Model Context Protocol) with the Alteriom painlessMesh repository.

## What's Been Implemented

### 1. GitHub Copilot Instructions (`.github/copilot-instructions.md`)
- Comprehensive project overview and architecture guide
- Build and test instructions specific to painlessMesh
- Code structure explanation with key files identified
- Alteriom-specific development considerations
- Best practices for mesh network development
- Common tasks and troubleshooting guidance

### 2. MCP Server Configuration (`.github/mcp-server.json`)
- Filesystem access server for code navigation
- Bash execution server for build/test operations
- Git integration server for version control
- Custom painlessMesh server for specialized operations

### 3. Custom MCP Server (`.github/mcp-painlessmesh.js`)
Provides specialized tools for painlessMesh development:
- **build_project**: Build with CMake and Ninja
- **run_tests**: Execute test suites with filtering
- **analyze_mesh_code**: Code structure analysis
- **create_alteriom_package**: Generate package templates
- **validate_dependencies**: Check project dependencies

### 4. Alteriom Package Examples (`examples/alteriom/`)
- **SensorPackage**: Environmental data broadcasting
- **CommandPackage**: Device command handling
- **StatusPackage**: Health and status reporting
- Complete Arduino example implementation
- Comprehensive test suite

### 5. Development Documentation
- **README-DEVELOPMENT.md**: Complete development guide
- **ALTERIOM_SETUP.md**: This setup guide
- Example usage patterns and integration guides

## Quick Start

### 1. Environment Setup
```bash
# Install system dependencies
sudo apt-get install cmake ninja-build libboost-dev libboost-system-dev

# Clone and setup repository
git clone https://github.com/Alteriom/painlessMesh.git
cd painlessMesh

# Setup dependencies
cd test
git clone https://github.com/bblanchon/ArduinoJson.git
git clone https://github.com/arkhipenko/TaskScheduler
cd ..

# Build project
cmake -G Ninja .
ninja
```

### 2. Test Alteriom Extensions
```bash
# Run all tests
run-parts --regex catch_ bin/

# Run Alteriom-specific tests
./bin/catch_alteriom_packages

# Verify 41 assertions pass in 5 test cases
```

### 3. Setup MCP Server
```bash
# Install Node.js dependencies
cd .github
npm install

# Test MCP server
node mcp-painlessmesh.js
# Should output: "painlessMesh MCP server running on stdio"
```

### 4. GitHub Copilot Integration
The Copilot instructions are automatically loaded when working in this repository. The MCP server configuration enables enhanced capabilities:

- **Code Navigation**: Full filesystem access for understanding project structure
- **Build Operations**: Direct build and test execution through Copilot
- **Code Analysis**: Automated analysis of mesh networking components
- **Package Generation**: Templates for new Alteriom-specific packages

## Using the Custom Packages

### Basic Sensor Node
```cpp
#include "painlessMesh.h"
#include "examples/alteriom/alteriom_sensor_package.hpp"

using namespace alteriom;

void setup() {
    // Initialize mesh...
    
    // Setup periodic sensor broadcasting
    userScheduler.addTask(Task(30000, TASK_FOREVER, []() {
        SensorPackage sensor;
        sensor.temperature = readTemperature();
        sensor.humidity = readHumidity();
        sensor.sensorId = mesh.getNodeId();
        sensor.timestamp = mesh.getNodeTime();
        
        mesh.sendBroadcast(sensor.toJsonString());
    }));
}
```

### Command Handling
```cpp
void onReceive(uint32_t from, String& msg) {
    auto doc = parseJson(msg);
    uint8_t msgType = doc["type"];
    
    switch(msgType) {
        case 201: // CommandPackage
            CommandPackage cmd(doc.as<JsonObject>());
            if (cmd.dest == mesh.getNodeId()) {
                executeCommand(cmd);
            }
            break;
    }
}
```

## MCP Server Tools Usage

With GitHub Copilot, you can now use natural language to:

```
"Build the project and run tests"
"Analyze the mesh protocol implementation"
"Create a new sensor package for GPS data with latitude and longitude fields"
"Check if all dependencies are properly installed"
"Run only the plugin tests"
```

These commands will be automatically translated to the appropriate MCP server tool calls.

## Project Structure

```
painlessMesh/
├── .github/
│   ├── copilot-instructions.md    # Copilot guidance
│   ├── mcp-server.json           # MCP configuration
│   ├── mcp-painlessmesh.js       # Custom MCP server
│   ├── package.json              # Node.js dependencies
│   └── README-DEVELOPMENT.md     # Development guide
├── examples/alteriom/
│   ├── alteriom_sensor_package.hpp  # Package definitions
│   ├── alteriom_sensor_node.ino     # Arduino example
│   └── README.md                     # Usage guide
├── test/catch/
│   └── catch_alteriom_packages.cpp  # Alteriom tests
└── [standard painlessMesh structure]
```

## Benefits for Alteriom Development

1. **Enhanced Copilot Understanding**: Comprehensive project context enables better code suggestions
2. **Streamlined Development**: MCP tools automate common development tasks
3. **Custom Package Framework**: Ready-to-use templates for Alteriom-specific needs
4. **Comprehensive Testing**: Full test coverage for custom extensions
5. **Documentation**: Complete guides for onboarding and development

## Next Steps

1. **Customize Packages**: Modify the example packages for specific Alteriom requirements
2. **Add Integrations**: Extend the MCP server with additional Alteriom-specific tools
3. **Scale Testing**: Add integration tests for multi-node scenarios
4. **Deploy**: Use the examples as templates for production Alteriom devices

## Support

- Review `.github/copilot-instructions.md` for detailed development guidance
- Check `.github/README-DEVELOPMENT.md` for comprehensive setup information
- Run `./bin/catch_alteriom_packages` to verify all Alteriom extensions work correctly
- Use the MCP server tools through GitHub Copilot for automated development tasks