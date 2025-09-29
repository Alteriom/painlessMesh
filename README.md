# PainlessMesh - Alteriom Fork

> **📚 Documentation**: https://alteriom.github.io/painlessMesh/ | **📖 Wiki**: https://github.com/Alteriom/painlessMesh/wiki

[![CI/CD Pipeline](https://github.com/Alteriom/painlessMesh/actions/workflows/ci.yml/badge.svg)](https://github.com/Alteriom/painlessMesh/actions/workflows/ci.yml)
[![Documentation](https://github.com/Alteriom/painlessMesh/actions/workflows/docs.yml/badge.svg)](https://github.com/Alteriom/painlessMesh/actions/workflows/docs.yml)
[![Release](https://github.com/Alteriom/painlessMesh/actions/workflows/release.yml/badge.svg)](https://github.com/Alteriom/painlessMesh/actions/workflows/release.yml)
[![GitHub release](https://img.shields.io/github/release/Alteriom/painlessMesh.svg)](https://github.com/Alteriom/painlessMesh/releases)
[![PlatformIO Registry](https://badges.registry.platformio.org/packages/alteriom/library/painlessMesh.svg)](https://registry.platformio.org/libraries/alteriom/painlessMesh)

## Intro to painlessMesh

painlessMesh is a user-friendly library for creating mesh networks with ESP8266 and ESP32 devices. This **Alteriom fork** extends the original library with specialized packages for IoT sensor networks, device control, and status monitoring.

### 🎯 Alteriom Extensions

This fork includes three specialized packages for structured IoT communication:

- **`SensorPackage`** (Type 200) - Environmental data collection (temperature, humidity, pressure, battery levels)
- **`CommandPackage`** (Type 201) - Device control and automation commands  
- **`StatusPackage`** (Type 202) - Health monitoring and system status reporting

All packages provide type-safe serialization, automatic JSON conversion, and mesh-wide broadcasting or targeted messaging.

### 🌐 Core Features

The library handles routing and network management automatically, so you can focus on your application. It uses JSON-based messaging and syncs time across all nodes, making it ideal for coordinated behaviour like synchronized light displays or sensor networks reporting to a central node. The original version was forked from [easymesh](https://github.com/Coopdis/easyMesh).

### True ad-hoc networking

painlessMesh is a true ad-hoc network, meaning that no-planning, central controller, or router is required. Any system of 1 or more nodes will self-organize into fully functional mesh. The maximum size of the mesh is limited (we think) by the amount of memory in the heap that can be allocated to the sub-connections buffer and so should be really quite high.

### JSON based

painlessMesh uses JSON objects for all its messaging. There are a couple of reasons for this. First, it makes the code and the messages human readable and painless to understand and second, it makes it painless to integrate painlessMesh with javascript front-ends, web applications, and other apps. Some performance is lost, but I haven’t been running into performance issues yet. Converting to binary messaging would be fairly straight forward if someone wants to contribute.

### Wifi &amp; Networking

painlessMesh is designed to be used with Arduino, but it does not use the Arduino WiFi libraries, as we were running into performance issues (primarily latency) with them. Rather the networking is all done using the native esp32 and esp8266 SDK libraries, which are available through the Arduino IDE. Hopefully though, which networking libraries are used won’t matter to most users much as you can just include painlessMesh.h, run the init() and then work the library through the API.

### painlessMesh is not IP networking

painlessMesh does not create a TCP/IP network of nodes. Rather each of the nodes is uniquely identified by its 32bit chipId which is retrieved from the esp8266/esp32 using the `system_get_chip_id()` call in the SDK. Every node will have a unique number. Messages can either be broadcast to all the nodes on the mesh, or sent specifically to an individual node which is identified by its `nodeId.

### Limitations and caveats

- Try to avoid using `delay()` in your code. To maintain the mesh we need to perform some tasks in the background. Using `delay()` will stop these tasks from happening and can cause the mesh to lose stability/fall apart. Instead, we recommend using [TaskScheduler](http://playground.arduino.cc/Code/TaskScheduler) which is used in `painlessMesh` itself. Documentation can be found [here](https://github.com/arkhipenko/TaskScheduler/wiki/Full-Document). For other examples on how to use the scheduler see the example folder.
- `painlessMesh` subscribes to WiFi events. Please be aware that as a result `painlessMesh` can be incompatible with user programs/other libraries that try to bind to the same events.
- Try to be conservative in the number of messages (and especially broadcast messages) you sent per minute. This is to prevent the hardware from overloading. Both esp8266 and esp32 are limited in processing power/memory, making it easy to overload the mesh and destabilize it. And while `painlessMesh` tries to prevent this from happening, it is not always possible to do so.
- Messages can go missing or be dropped due to high traffic and you can not rely on all messages to be delivered. One suggestion to work around is to resend messages every so often. Even if some go missing, most should go through. Another option is to have your nodes send replies when they receive a message. The sending nodes can the resend the message if they haven’t gotten a reply in a certain amount of time.

## Installation

`painlessMesh` is included in both the Arduino Library Manager and the platformio library registry and can easily be installed via either of those methods.

### Dependencies

painlessMesh makes use of the following libraries, which can be installed through the Arduino Library Manager

- [ArduinoJson](https://github.com/bblanchon/ArduinoJson)
- [TaskScheduler](https://github.com/arkhipenko/TaskScheduler)
- [ESPAsyncTCP](https://github.com/me-no-dev/ESPAsyncTCP) (ESP8266)
- [AsyncTCP](https://github.com/me-no-dev/AsyncTCP) (ESP32)

If platformio is used to install the library, then the dependencies will be installed automatically.

## Quick Start with Alteriom Packages

### Basic Sensor Node

```cpp
#include "painlessMesh.h"
#include "examples/alteriom/alteriom_sensor_package.hpp"

using namespace alteriom;

#define MESH_PREFIX     "AlteriomMesh"
#define MESH_PASSWORD   "your_password"
#define MESH_PORT       5555

Scheduler userScheduler;
painlessMesh mesh;

void setup() {
    Serial.begin(115200);
    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
    mesh.onReceive(&receivedCallback);
}

void loop() {
    mesh.update();
    
    // Create and send sensor data
    SensorPackage sensor;
    sensor.temperature = 25.5;
    sensor.humidity = 60.0;
    sensor.sensorId = mesh.getNodeId();
    sensor.timestamp = mesh.getNodeTime();
    
    mesh.sendBroadcast(sensor.toJsonString());
    delay(30000); // Send every 30 seconds
}

void receivedCallback(uint32_t from, String& msg) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, msg);
    
    if (doc["type"] == 200) { // SensorPackage
        SensorPackage sensor(doc.as<JsonObject>());
        Serial.printf("Sensor %u: %.1f°C, %.1f%% RH\n", 
                     sensor.sensorId, sensor.temperature, sensor.humidity);
    }
}
```

### Package Types

| Type | Class | Purpose | Fields |
|------|-------|---------|--------|
| 200 | `SensorPackage` | Environmental data | `temperature`, `humidity`, `pressure`, `sensorId`, `timestamp`, `batteryLevel` |
| 201 | `CommandPackage` | Device control | `command`, `targetDevice`, `parameters`, `commandId` |
| 202 | `StatusPackage` | Health monitoring | `deviceStatus`, `uptime`, `freeMemory`, `wifiStrength`, `firmwareVersion` |

## Key Features

- **🔄 Automatic Mesh Formation** - Nodes discover and connect automatically
- **📡 Self-Healing Network** - Adapts when nodes join/leave
- **⏰ Time Synchronization** - Coordinated actions across all nodes  
- **🔀 Smart Routing** - Broadcast, point-to-point, and neighbor messaging
- **🔌 Plugin System** - Type-safe custom message packages
- **📱 ESP32 & ESP8266** - Full support for both platforms
- **🛡️ Memory Efficient** - Optimized for resource-constrained devices

## Examples & Use Cases

- **IoT Sensor Networks** - Environmental monitoring, smart agriculture
- **Home Automation** - Distributed lighting, HVAC control
- **Industrial Monitoring** - Equipment status, predictive maintenance  
- **Event Coordination** - Synchronized displays, distributed processing
- **Bridge Networks** - Connect mesh to WiFi/Internet/MQTT

## Getting Help

- **[FAQ](docs/troubleshooting/faq.md)** - Common questions and solutions
- **[GitHub Issues](https://github.com/Alteriom/painlessMesh/issues)** - Bug reports and feature requests  
- **[Community Forum](https://groups.google.com/forum/#!forum/painlessmesh-user)** - Community support
- **[API Documentation](http://painlessmesh.gitlab.io/painlessMesh/index.html)** - Generated API docs

## Development

### Building from Source
```bash
git clone https://github.com/Alteriom/painlessMesh.git
cd painlessMesh
git submodule update --init
cmake -G Ninja .
ninja
run-parts --regex catch_ bin/  # Run tests
```

### Requirements
- **ESP32/ESP8266**: Arduino Core 2.0.0+
- **Dependencies**: ArduinoJson 6.x, TaskScheduler 3.x  
- **Development**: CMake, Ninja, Boost (for desktop testing)

### CI/CD Pipeline

painlessMesh features a state-of-the-art automated CI/CD pipeline:

**🔄 Continuous Integration:**
- Automated builds on gcc/clang with strict warnings
- Cross-platform testing (Arduino CLI, PlatformIO)
- Code quality and formatting validation
- Comprehensive test suite execution

**🚀 Automated Releases:**
- Semantic versioning with automated tagging
- GitHub Releases with changelog generation
- Library package distribution
- Documentation deployment to GitHub Pages
- Arduino Library Manager & PlatformIO Registry integration

**📋 Release Management:**
```bash
# Bump version and prepare release
./scripts/bump-version.sh patch  # or minor, major
./scripts/validate-release.sh    # Validate release readiness

# Edit CHANGELOG.md, then commit with release prefix
git commit -am "release: v1.5.7"
git push origin main  # Triggers automated release
```

See [RELEASE_GUIDE.md](RELEASE_GUIDE.md) for complete release documentation.

## Contributing

We try to follow the [git flow](https://www.atlassian.com/git/tutorials/comparing-workflows/gitflow-workflow) development model. Which means that we have a `develop` branch and `master` branch. All development is done under feature branches, which are (when finished) merged into the development branch. When a new version is released we merge the `develop` branch into the `master` branch. For more details see the [CONTRIBUTING](https://gitlab.com/painlessMesh/painlessMesh/blob/master/CONTRIBUTING.md) file.

## Funding

If you like the library please consider giving me a tip. This means I will be able to spend more time on developing it.
You can tip me using ko-fi:

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/U7U21LWO6I)

## 📚 Complete Documentation

**🎯 New to painlessMesh?** Start here:
- **[Quick Start Guide](docs/getting-started/quickstart.md)** - Get running in minutes
- **[Installation Guide](docs/getting-started/installation.md)** - All platforms and IDEs
- **[Your First Mesh](docs/getting-started/first-mesh.md)** - Build a real multi-node network

**🔧 API Reference:**
- **[Core API](docs/api/core-api.md)** - Complete painlessMesh class reference
- **[Plugin API](docs/api/plugin-api.md)** - Custom packages and type-safe messaging
- **[Configuration](docs/api/configuration.md)** - All configuration options

**🏗️ Architecture & Design:**
- **[Mesh Architecture](docs/architecture/mesh-architecture.md)** - How painlessMesh works internally
- **[Plugin System](docs/architecture/plugin-system.md)** - Advanced plugin development
- **[Message Routing](docs/architecture/routing.md)** - Routing algorithms and strategies

**📖 Tutorials & Examples:**
- **[Basic Examples](docs/tutorials/basic-examples.md)** - Essential patterns and techniques
- **[Custom Packages](docs/tutorials/custom-packages.md)** - Type-safe message handling
- **[Sensor Networks](docs/tutorials/sensor-networks.md)** - IoT sensor network patterns

**🚀 Alteriom Extensions:**
- **[Alteriom Overview](docs/alteriom/overview.md)** - Production-ready IoT packages
- **[Sensor Packages](docs/alteriom/sensor-packages.md)** - Environmental monitoring
- **[Command System](docs/alteriom/command-system.md)** - Device control and automation

**🔧 Troubleshooting:**
- **[Common Issues](docs/troubleshooting/common-issues.md)** - Solutions to frequent problems
- **[FAQ](docs/troubleshooting/faq.md)** - Frequently asked questions
- **[Debugging Guide](docs/troubleshooting/debugging.md)** - Tools and techniques

**📋 Complete Documentation Index:** [docs/README.md](docs/README.md)

## painlessMesh API Summary

Here's a quick API overview. **For complete documentation, see [Core API Reference](docs/api/core-api.md)**

```cpp
#include "painlessMesh.h"

painlessMesh mesh;
```

### Member Functions

#### void painlessMesh::init(String ssid, String password, uint16_t port = 5555, WiFiMode_t connectMode = WIFI_AP_STA, _auth_mode authmode = AUTH_WPA2_PSK, uint8_t channel = 1, phy_mode_t phymode = PHY_MODE_11G, uint8_t maxtpw = 82, uint8_t hidden = 0, uint8_t maxconn = 4)

Add this to your setup() function.
Initialize the mesh network. This routine does the following things.

- Starts a wifi network
- Begins searching for other wifi networks that are part of the mesh
- Logs on to the best mesh network node it finds… if it doesn’t find anything, it starts a new search in 5 seconds.

`ssid` = the name of your mesh.  All nodes share same AP ssid. They are distinguished by BSSID.
`password` = wifi password to your mesh.
`port` = the TCP port that you want the mesh server to run on. Defaults to 5555 if not specified.
[`connectMode`](https://gitlab.com/painlessMesh/painlessMesh/wikis/connect-mode:-WIFI_AP,-WIFI_STA,-WIFI_AP_STA-mode) = switch between WIFI_AP, WIFI_STA and WIFI_AP_STA (default) mode

#### void painlessMesh::stop()

Stop the node. This will cause the node to disconnect from all other nodes and stop/sending messages.

#### void painlessMesh::update( void )

Add this to your loop() function
This routine runs various maintenance tasks... Not super interesting, but things don't work without it.

#### void painlessMesh::onReceive( &amp;receivedCallback )

Set a callback routine for any messages that are addressed to this node. Callback routine has the following structure.

`void receivedCallback( uint32_t from, String &amp;msg )`

Every time this node receives a message, this callback routine will the called. “from” is the id of the original sender of the message, and “msg” is a string that contains the message. The message can be anything. A JSON, some other text string, or binary data.

#### void painlessMesh::onNewConnection( &amp;newConnectionCallback )

This fires every time the local node makes a new connection. The callback has the following structure.

`void newConnectionCallback( uint32_t nodeId )`

`nodeId` is new connected node ID in the mesh.

#### void painlessMesh::onChangedConnections( &amp;changedConnectionsCallback )

This fires every time there is a change in mesh topology. Callback has the following structure.

`void onChangedConnections()`

There are no parameters passed. This is a signal only.

#### bool painlessMesh::isConnected( nodeId )

Returns if a given node is currently connected to the mesh.

`nodeId` is node ID that the request refers to.

#### void painlessMesh::onNodeTimeAdjusted( &amp;nodeTimeAdjustedCallback )

This fires every time local time is adjusted to synchronize it with mesh time. Callback has the following structure.

`void onNodeTimeAdjusted(int32_t offset)`

`offset` is the adjustment delta that has been calculated and applied to local clock.

#### void onNodeDelayReceived(nodeDelayCallback_t onDelayReceived)

This fires when a time delay measurement response is received, after a request was sent. Callback has the following structure.

`void onNodeDelayReceived(uint32_t nodeId, int32_t delay)`

`nodeId` The node that originated response.

`delay` One way network trip delay in microseconds.

#### bool painlessMesh::sendBroadcast( String &amp;msg, bool includeSelf = false)

Sends msg to every node on the entire mesh network. By default the current node is excluded from receiving the message (`includeSelf = false`). `includeSelf = true` overrides this behavior, causing the `receivedCallback` to be called when sending a broadcast message.

returns true if everything works, false if not. Prints an error message to Serial.print, if there is a failure.

#### bool painlessMesh::sendSingle(uint32_t dest, String &amp;msg)

Sends msg to the node with Id == dest.

returns true if everything works, false if not.  Prints an error message to Serial.print, if there is a failure.

#### String painlessMesh::subConnectionJson()

Returns mesh topology in JSON format.

#### std::list<uint32_t> painlessMesh::getNodeList()

Get a list of all known nodes. This includes nodes that are both directly and indirectly connected to the current node.

#### uint32_t painlessMesh::getNodeId( void )

Return the chipId of the node that we are running on.

#### uint32_t painlessMesh::getNodeTime( void )

Returns the mesh timebase microsecond counter. Rolls over 71 minutes from startup of the first node.

Nodes try to keep a common time base synchronizing to each other using [an SNTP based protocol](https://gitlab.com/painlessMesh/painlessMesh/wikis/mesh-protocol#time-sync)

#### bool painlessMesh::startDelayMeas(uint32_t nodeId)

Sends a node a packet to measure network trip delay to that node. Returns true if nodeId is connected to the mesh, false otherwise. After calling this function, user program have to wait to the response in the form of a callback specified by `void painlessMesh::onNodeDelayReceived(nodeDelayCallback_t onDelayReceived)`.

nodeDelayCallback_t is a function in the form of `void (uint32_t nodeId, int32_t delay)`.

#### void painlessMesh::stationManual( String ssid, String password, uint16_t port, uint8_t *remote_ip )

Connects the node to an AP outside the mesh. When specifying a `remote_ip` and `port`, the node opens a TCP connection after establishing the WiFi connection.

Note: The mesh must be on the same WiFi channel as the AP.

#### void painlessMesh::setDebugMsgTypes( uint16_t types )

Change the internal log level. List of types defined in Logger.hpp:
ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE


# Funding 

Most development of painlessMesh has been done as a hobby, but some specific features have been funded by the companies listed below:

![Sowillo](https://www.sowillo.com/wp-content/uploads/2019/04/Logo-Sowillo-1.png)

[Sowillo](http://sowillo.com/en/)
