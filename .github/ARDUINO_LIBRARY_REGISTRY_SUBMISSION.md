# Arduino Library Manager Submission Template

Copy this template and create an issue at: https://github.com/arduino/library-registry

---

## Add AlteriomPainlessMesh to Arduino Library Manager

**Repository URL**: https://github.com/Alteriom/painlessMesh

**Library Name**: AlteriomPainlessMesh

**Current Version**: 1.8.2

**Release Tag**: v1.8.2

**Description**: 

AlteriomPainlessMesh is a user-friendly library for creating mesh networks with ESP8266 and ESP32 devices. This is an enhanced fork of the original painlessMesh library with specialized packages for IoT sensor networks, device control, and status monitoring.

### Key Features

**Core IoT Packages:**
- **SensorPackage** (Type 200): Environmental data collection with temperature, humidity, pressure monitoring, battery tracking
- **StatusPackage** (Type 202): Device health monitoring with uptime, memory, WiFi metrics, firmware version tracking
- **CommandPackage** (Type 400): Remote device control and automation with JSON parameter support

**Advanced Monitoring (Phase 2):**
- **MetricsPackage** (Type 204): Comprehensive performance metrics (CPU, memory, network throughput, latency) for dashboard integration
- **HealthCheckPackage** (Type 605): Proactive problem detection with memory leak detection, predictive maintenance, crash tracking

**Mesh Topology & Management:**
- **EnhancedStatusPackage** (Type 604): Detailed mesh statistics and performance metrics
- **MeshNodeListPackage** (Type 600): Node discovery and inventory (supports 50 nodes)
- **MeshTopologyPackage** (Type 601): Network topology visualization with connection quality
- **MeshAlertPackage** (Type 602): Network event notifications with configurable severity
- **MeshBridgePackage** (Type 603): Protocol bridging for heterogeneous networks

**Bridge & High Availability (v1.8.0+):**
- **BridgeStatusPackage** (Type 610): Real-time Internet connectivity monitoring
- **BridgeElectionPackage** (Type 611): Automatic bridge failover with RSSI-based election
- **BridgeTakeoverPackage** (Type 612): Bridge transition announcements
- **BridgeCoordinationPackage** (Type 613): Multi-bridge coordination and load balancing
- **NTPTimeSyncPackage** (Type 614): Bridge-to-mesh NTP time distribution
- **MessageQueue**: Offline message queuing for critical sensor data during Internet outages

The library handles mesh routing and network management automatically, uses JSON-based messaging, and syncs time across all nodes. Perfect for IoT sensor networks, smart agriculture, home automation, and industrial monitoring.

**Category**: Communication

**Architectures**: esp8266, esp32

**Dependencies**: 
- ArduinoJson (^7.4.2)
- TaskScheduler (^4.0.0)
- AsyncTCP (^3.4.7) - ESP32 only
- ESPAsyncTCP (^2.0.0) - ESP8266 only

**License**: LGPL-3.0

**Documentation**: https://alteriom.github.io/painlessMesh/

**Maintainer**: Alteriom (https://github.com/Alteriom)

**Additional Information**:
- **GitHub Repository**: https://github.com/Alteriom/painlessMesh
- **CI/CD**: Comprehensive automated testing across platforms
- **Examples**: 19+ working examples included
- **Tests**: 710+ unit tests passing
- **Active Development**: Regular releases with semantic versioning
- **PlatformIO Registry**: https://registry.platformio.org/libraries/sparck75/AlteriomPainlessMesh
- **NPM Package**: https://www.npmjs.com/package/@alteriom/painlessmesh
- **GitHub Packages**: @alteriom/painlessmesh

### Compliance Checklist

This library meets all Arduino Library Manager requirements:

- ✅ **Repository Structure**: Proper src/ and examples/ directories
- ✅ **library.properties**: Valid and complete (version=1.8.2)
- ✅ **Git Tags**: Semantic versioning with proper tags (v1.8.2, v1.8.1, etc.)
- ✅ **Examples**: 19+ examples that compile successfully
- ✅ **License**: Open source (LGPL-3.0)
- ✅ **Documentation**: Comprehensive README and online docs
- ✅ **Compilation**: Tested and verified on ESP32 and ESP8266
- ✅ **Dependencies**: All dependencies available in Library Manager
- ✅ **Versioning**: Consistent across library.properties, library.json, package.json

### Release History

Recent releases:
- **v1.8.2** (2025-11-11): Multi-bridge coordination, message queue for offline mode
- **v1.8.1** (2025-11-10): GitHub Copilot custom agent support
- **v1.8.0** (2025-11-09): Diagnostics API, bridge health monitoring, RTC integration
- **v1.7.9** (2025-11-08): CI/CD pipeline fixes
- **v1.7.8** (2025-11-05): MQTT schema compliance, enhanced packages

Full changelog: https://github.com/Alteriom/painlessMesh/blob/main/CHANGELOG.md

### Support & Community

- **Issues**: https://github.com/Alteriom/painlessMesh/issues
- **Discussions**: https://github.com/Alteriom/painlessMesh/discussions
- **Documentation**: https://alteriom.github.io/painlessMesh/

---

**Submission Type**: New Library Registration

**Ready for Indexing**: ✅ Yes

Thank you for reviewing this submission!
