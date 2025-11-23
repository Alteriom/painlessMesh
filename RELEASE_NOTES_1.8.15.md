# Release Notes v1.8.15

**Release Date**: November 23, 2025

## Overview

This release integrates the painlessMesh-simulator for automated validation of example sketches, providing a comprehensive testing framework that validates mesh behavior with virtual nodes. This enhances the library's quality assurance and helps prevent regressions.

## What's New

### Simulator Integration

**Automated Example Validation**
- Integrated [painlessMesh-simulator](https://github.com/Alteriom/painlessMesh-simulator) as git submodule
- YAML-based test scenarios for configuration-driven testing
- Validates mesh formation, message broadcasting, and time synchronization
- Runs automatically in CI/CD pipeline on every push and pull request
- Framework supports testing with 100+ virtual nodes without hardware

**Test Coverage**
- Basic example validation with 5 virtual nodes
- Mesh formation verification (30 second timeout)
- Message delivery validation (5+ messages per node)
- Time synchronization testing (<10ms drift)
- Network metrics collection (CSV output)

### Documentation

**New Documentation**
- `RELEASE_READINESS_PLAN.md` - Comprehensive library audit and release assessment
- `TESTING_WITH_SIMULATOR.md` - Quick start guide for using the simulator
- `docs/SIMULATOR_TESTING.md` - Complete integration guide with CI details
- `examples/basic/test/simulator/README.md` - Example-specific testing instructions

**Release Assessment**
- Confirmed all 119+ test assertions passing
- Security scans passing (CodeQL)
- Builds verified on all platforms (ESP8266, ESP32, Desktop)
- Production-ready confirmation from maintainer

## Improvements

### CI/CD Pipeline

**Enhanced Automation**
- New `simulator-tests` job runs on every commit
- Automatically builds simulator and executes test scenarios
- Uploads test results as artifacts for debugging
- 120-second timeout to prevent hanging
- Integrated with existing CI jobs (desktop, Arduino, PlatformIO builds)

### Build System

**Dependencies**
- Added libboost-program-options-dev for simulator build
- Updated all documentation with correct dependency lists
- Fixed CMakeLists.txt references to removed test files

## Bug Fixes

- Fixed build system references to deleted example test files
- Corrected simulator executable path in CI configuration
- Fixed YAML configuration format to match simulator API

## Breaking Changes

None. This release is fully backward compatible.

## Upgrade Instructions

### For End Users

No action required. This is a fully backward-compatible release focused on testing infrastructure improvements.

### For Contributors/Developers

If you want to run simulator tests locally:

```bash
# Initialize simulator submodule
git submodule update --init test/simulator

# Install dependencies (Ubuntu/Debian)
sudo apt-get install cmake ninja-build libboost-dev libboost-program-options-dev libyaml-cpp-dev

# Build simulator
cd test/simulator && mkdir build && cd build
cmake -G Ninja .. && ninja

# Run basic example test
bin/painlessmesh-simulator --config \
  ../../../examples/basic/test/simulator/scenarios/basic_mesh_test.yaml
```

## Test Results

**Library Tests**: ✅ ALL PASSING
```
✓ catch_tcp_integration    - 113 assertions
✓ catch_connection         - 6 assertions  
✓ 30+ unit tests           - router, bridge, messaging, etc.
✓ Simulator basic test     - 5 nodes, mesh formation validated

Total: 119+ assertions, ALL PASSING
```

**Security**: ✅ CodeQL passing, no alerts

**Builds**: ✅ Desktop, Arduino, PlatformIO, ESP8266, ESP32

## Known Issues

None identified. See [GitHub Issues](https://github.com/Alteriom/painlessMesh/issues) for any reports.

## Migration Guide

No migration required. This release is fully backward compatible with v1.8.14 and earlier.

## Contributors

This release was made possible by:
- **@Alteriom** - Simulator integration, testing infrastructure, documentation
- **GitHub Copilot** - Development assistance and code review
- **Community Contributors** - Issue reports and feedback

## Next Steps

### Future Enhancements

**Expanded Test Coverage**
- Additional simulator scenarios for remaining examples (startHere, echoNode, bridge, MQTT)
- Extended test coverage with edge cases (network failures, packet loss, etc.)
- Performance benchmarking with 100+ node simulations

**Custom Firmware**
- Contributors can add custom firmware types to painlessMesh-simulator
- Pattern established for testing custom mesh behaviors
- Documentation for extending simulator capabilities

## Links

- [GitHub Release](https://github.com/Alteriom/painlessMesh/releases/tag/v1.8.15)
- [Full Changelog](https://github.com/Alteriom/painlessMesh/blob/main/CHANGELOG.md)
- [Issue #163 - Improve validation](https://github.com/Alteriom/painlessMesh/issues/163)
- [Pull Request #164](https://github.com/Alteriom/painlessMesh/pull/164)
- [painlessMesh-simulator Repository](https://github.com/Alteriom/painlessMesh-simulator)
- [Documentation](https://github.com/Alteriom/painlessMesh/blob/main/DOCUMENTATION_INDEX.md)

## Support

For questions or issues:
- GitHub Issues: https://github.com/Alteriom/painlessMesh/issues
- Documentation: https://github.com/Alteriom/painlessMesh/blob/main/README.md
- Release Guide: https://github.com/Alteriom/painlessMesh/blob/main/RELEASE_GUIDE.md

---

**Thank you for using AlteriomPainlessMesh!**

This release represents a significant step forward in ensuring library quality through automated testing. We're committed to maintaining high standards and appreciate the community's continued support.
