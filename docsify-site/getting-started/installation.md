# Installation Guide

This guide covers all the different ways to install and set up painlessMesh for your development environment.

## Arduino IDE Installation

### Method 1: Library Manager (Recommended)

1. Open Arduino IDE
2. Go to **Sketch → Include Library → Manage Libraries**
3. Search for "painlessMesh"
4. Install the latest version by "Coopdis"
5. Install dependencies when prompted:
   - ArduinoJson
   - TaskScheduler

### Method 2: Manual Installation

1. Download the latest release from [GitHub](https://github.com/Alteriom/painlessMesh/releases)
2. Extract the ZIP file
3. Copy the `painlessMesh` folder to your Arduino libraries directory:
   - **Windows**: `Documents\Arduino\libraries\`
   - **macOS**: `~/Documents/Arduino/libraries/`
   - **Linux**: `~/Arduino/libraries/`
4. Restart Arduino IDE

## PlatformIO Installation

### Method 1: platformio.ini (Recommended)

Add to your `platformio.ini` file:

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps = 
    painlessMesh
    bblanchon/ArduinoJson@^6.21.3
    arkhipenko/TaskScheduler@^3.7.0

# For ESP8266
[env:esp8266]
platform = espressif8266
board = nodemcuv2
framework = arduino
lib_deps = 
    painlessMesh
    bblanchon/ArduinoJson@^6.21.3
    arkhipenko/TaskScheduler@^3.7.0
```

### Method 2: PlatformIO Library Manager

```bash
# Install via PlatformIO CLI
pio lib install "painlessMesh"

# Or install specific version
pio lib install "painlessMesh@1.5.0"
```

## Board Support

### ESP32 Boards
painlessMesh supports all ESP32 variants:
- ESP32 DevKit
- ESP32-S2
- ESP32-S3  
- ESP32-C3
- ESP32-WROOM
- ESP32-WROVER

### ESP8266 Boards
All ESP8266 boards are supported:
- NodeMCU
- Wemos D1 Mini
- ESP-12E/F
- ESP-01 (with limitations due to memory)

## Dependencies

painlessMesh requires these libraries:

### Core Dependencies
- **ArduinoJson** (v6.x) - JSON parsing and generation
- **TaskScheduler** (v3.x) - Task scheduling system

### Platform Dependencies
- **ESP32 Arduino Core** (v2.0.0+) for ESP32 boards
- **ESP8266 Arduino Core** (v3.0.0+) for ESP8266 boards

## Development Environment Setup

### For Library Development

If you plan to contribute to painlessMesh or need the latest development version:

```bash
# Clone the repository
git clone https://github.com/Alteriom/painlessMesh.git
cd painlessMesh

# Initialize submodules
git submodule init
git submodule update

# Install test dependencies (for desktop testing)
cd test
git clone https://github.com/bblanchon/ArduinoJson.git
git clone https://github.com/arkhipenko/TaskScheduler.git
cd ..

# Build tests (requires CMake and Ninja)
cmake -G Ninja .
ninja
```

### Desktop Testing (Linux/macOS/Windows)

For development and testing on your computer:

#### Requirements
- CMake 3.10+
- Ninja build system
- Boost libraries
- C++14 compatible compiler

#### Ubuntu/Debian
```bash
sudo apt update
sudo apt install cmake ninja-build libboost-all-dev build-essential

# Clone and build
git clone https://github.com/Alteriom/painlessMesh.git
cd painlessMesh
git submodule update --init
cmake -G Ninja .
ninja

# Run tests
run-parts --regex catch_ bin/
```

#### macOS
```bash
# Install dependencies
brew install cmake ninja boost

# Clone and build
git clone https://github.com/Alteriom/painlessMesh.git
cd painlessMesh
git submodule update --init
cmake -G Ninja .
ninja

# Run tests
run-parts --regex catch_ bin/
```

#### Windows
Use Visual Studio with CMake support or install dependencies via vcpkg:

```powershell
# Install vcpkg first, then:
vcpkg install boost:x64-windows
cmake -G Ninja -DCMAKE_TOOLCHAIN_FILE=path/to/vcpkg.cmake .
ninja
```

## Version Compatibility

### Current Stable Version
- **painlessMesh**: 1.5.x
- **ArduinoJson**: 6.21.x
- **TaskScheduler**: 3.7.x

### Legacy Support
- painlessMesh 1.4.x - Compatible with older ESP cores
- ArduinoJson 5.x - No longer supported

## Memory Requirements

### ESP32
- **RAM**: ~50KB minimum for basic mesh functionality
- **Flash**: ~200KB for core library + your application
- **Recommended**: 320KB+ RAM for complex applications

### ESP8266
- **RAM**: ~20KB minimum for basic mesh functionality  
- **Flash**: ~150KB for core library + your application
- **Recommended**: 80KB+ RAM for stable operation
- **Note**: ESP-01 (512KB flash) may have limitations

## Configuration Options

### Build Flags

Add these to your build configuration if needed:

```ini
# platformio.ini
build_flags = 
    -DPAINLESSMESH_ENABLE_DEBUG=1     # Enable debug output
    -DPAINLESSMESH_MAX_CONNECTIONS=10  # Maximum connections
    -DTASK_SCHEDULER_DEBUG=1          # TaskScheduler debug
```

### Arduino IDE Defines

Add at the top of your sketch:

```cpp
#define PAINLESSMESH_ENABLE_DEBUG 1
#define PAINLESSMESH_MAX_CONNECTIONS 10
```

## IDE-Specific Setup

### Visual Studio Code + PlatformIO

1. Install the PlatformIO IDE extension
2. Create new project or open existing
3. Add library dependencies to `platformio.ini`
4. Use Ctrl+Shift+P → "PlatformIO: Build" to compile

### Arduino IDE 2.0

1. Install via Library Manager (same as Arduino IDE 1.x)
2. Use the new autocomplete features for better development experience
3. Debugging support available with compatible boards

## Verification

Test your installation with this minimal example:

```cpp
#include "painlessMesh.h"

painlessMesh mesh;

void setup() {
    Serial.begin(115200);
    Serial.println("painlessMesh installation test");
    
    // If this compiles and uploads successfully, installation is correct
    mesh.init("TestNetwork", "password", 5555);
    Serial.println("painlessMesh initialized successfully!");
}

void loop() {
    mesh.update();
}
```

If this compiles and uploads without errors, your installation is complete!

## Next Steps

- Try the [Quick Start Guide](quickstart.md) to create your first mesh
- Explore [Basic Examples](../tutorials/basic-examples.md)
- Read about [Mesh Architecture](../architecture/mesh-architecture.md)

## Troubleshooting Installation

**Library not found errors?**
- Check that ArduinoJson and TaskScheduler are installed
- Verify library versions are compatible
- Try cleaning and rebuilding your project

**Compilation errors?**
- Ensure you're using a supported ESP32/ESP8266 core version
- Check that your board selection matches your hardware
- See [Common Issues](../troubleshooting/common-issues.md) for more help