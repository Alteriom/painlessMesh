---
sidebar_position: 1
---

# Installation Guide

This guide covers all the different ways to install and set up painlessMesh for your development environment.

## Arduino IDE Installation

### Method 1: Library Manager (Recommended)

1. Open Arduino IDE
2. Go to **Sketch → Include Library → Manage Libraries**
3. Search for "painlessMesh"
4. Install the latest version
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

```ini title="platformio.ini"
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps = 
    Alteriom/painlessMesh@^1.6.0
    bblanchon/ArduinoJson@^6.21.3
    arkhipenko/TaskScheduler@^3.7.0

# For ESP8266
[env:esp8266]
platform = espressif8266
board = nodemcuv2
framework = arduino
lib_deps = 
    Alteriom/painlessMesh@^1.6.0
    bblanchon/ArduinoJson@^6.21.3
    arkhipenko/TaskScheduler@^3.7.0
```

### Method 2: PlatformIO CLI

```bash
# Install via PlatformIO CLI
pio lib install "Alteriom/painlessMesh"

# Or install specific version
pio lib install "Alteriom/painlessMesh@1.6.0"
```

## Supported Hardware

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

<Tabs>
<TabItem value="esp32" label="ESP32 Boards">

painlessMesh supports all ESP32 variants:
- ESP32 DevKit
- ESP32-S2, ESP32-S3  
- ESP32-C3
- ESP32-WROOM, ESP32-WROVER

**Memory**: ~50KB RAM minimum, 320KB+ recommended

</TabItem>
<TabItem value="esp8266" label="ESP8266 Boards">

All ESP8266 boards are supported:
- NodeMCU
- Wemos D1 Mini
- ESP-12E/F
- ESP-01 (limited due to memory)

**Memory**: ~20KB RAM minimum, 80KB+ recommended

</TabItem>
</Tabs>

## Dependencies

painlessMesh requires these libraries:

| Library | Version | Purpose |
|---------|---------|---------|
| **ArduinoJson** | v6.x | JSON parsing and generation |
| **TaskScheduler** | v3.x | Task scheduling system |
| **ESP32 Core** | v2.0.0+ | For ESP32 boards |
| **ESP8266 Core** | v3.0.0+ | For ESP8266 boards |

:::warning ArduinoJson v7
painlessMesh is not yet compatible with ArduinoJson v7. Use v6.21.x for now.
:::

## Development Environment

### Visual Studio Code + PlatformIO

1. Install the **PlatformIO IDE** extension
2. Create new project or open existing
3. Add library dependencies to `platformio.ini`
4. Use **Ctrl+Shift+P → "PlatformIO: Build"** to compile

### Arduino IDE 2.0

1. Install via **Library Manager** (same as Arduino IDE 1.x)
2. Use the new autocomplete features
3. Debugging support available with compatible boards

## Desktop Testing (Advanced)

For library development and testing on your computer:

### Requirements
- CMake 3.10+
- Ninja build system  
- Boost libraries
- C++14 compatible compiler

### Ubuntu/Debian
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

## Configuration Options

### Build Flags

Add these to your `platformio.ini` if needed:

```ini title="platformio.ini"
build_flags = 
    -DPAINLESSMESH_ENABLE_DEBUG=1     # Enable debug output
    -DPAINLESSMESH_MAX_CONNECTIONS=10  # Maximum connections
    -DTASK_SCHEDULER_DEBUG=1          # TaskScheduler debug
```

### Arduino IDE Defines

Add at the top of your sketch:

```cpp title="your_sketch.ino"
#define PAINLESSMESH_ENABLE_DEBUG 1
#define PAINLESSMESH_MAX_CONNECTIONS 10

#include "painlessMesh.h"
```

## Verification

Test your installation with this minimal example:

```cpp title="installation_test.ino"
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

## Memory Requirements

### ESP32
- **RAM**: ~50KB minimum for basic mesh functionality
- **Flash**: ~200KB for core library + your application
- **Recommended**: 320KB+ RAM for complex applications

### ESP8266
- **RAM**: ~20KB minimum for basic mesh functionality  
- **Flash**: ~150KB for core library + your application
- **Recommended**: 80KB+ RAM for stable operation

:::tip Memory Optimization
ESP8266 devices may struggle with large meshes (>10 nodes) due to memory limitations.
:::

## Version Compatibility

| painlessMesh | ArduinoJson | Arduino Core |
|--------------|-------------|--------------|
| 1.6.x | 6.19+ | ESP32: 2.0.3+ / ESP8266: 3.0.0+ |
| 1.5.x | 6.15+ | ESP32: 1.0.6+ / ESP8266: 2.7.0+ |

## Next Steps

- 📚 [Quick Start Guide](./quickstart) - Build your first mesh network
- 🔧 [Basic Examples](../tutorials/basic-examples) - Common usage patterns  
- 🏗️ [Architecture Overview](../architecture/mesh-architecture) - How painlessMesh works

## Troubleshooting

**Library not found errors?**
- Check that ArduinoJson and TaskScheduler are installed
- Verify library versions are compatible
- Try cleaning and rebuilding your project

**Compilation errors?**
- Ensure you're using a supported ESP32/ESP8266 core version
- Check that your board selection matches your hardware
- See [Common Issues](../troubleshooting/common-issues) for more help