# Arduino Library Manager Submission Guide

## Overview

This document provides instructions for submitting AlteriomPainlessMesh to the Arduino Library Manager registry, which will enable users to install and update the library directly from the Arduino IDE's Library Manager.

## Current Status

**Status**: ❌ Not yet registered  
**Latest Version**: 1.8.2  
**Repository**: https://github.com/Alteriom/painlessMesh

## Problem Statement

Users report that the Arduino IDE:
- Does not show the current version (1.8.2)
- Shows an old version (1.6.1) or no version at all
- Cannot install the library via Library Manager
- Has issues when trying to add the library as a ZIP file

## Root Cause

The library is not registered in the Arduino Library Manager's official registry at:
https://github.com/arduino/library-registry

Without registration, the library cannot be discovered or installed via Arduino IDE's Library Manager.

## Solution: Submit to Arduino Library Registry

### Prerequisites Check ✅

Before submission, verify that the library meets all Arduino Library Manager requirements.

**Automated Validation**:

Run the validation script to check all requirements:

```bash
./scripts/validate-arduino-compliance.sh
```

This script verifies:

- ✅ **library.properties file**: Present and properly formatted
- ✅ **Version field**: Set to 1.8.2
- ✅ **src/ directory**: Contains all library source code
- ✅ **examples/ directory**: Contains working example sketches (29+ examples)
- ✅ **Valid license**: LGPL-3.0 (open source)
- ✅ **README.md**: Comprehensive documentation
- ✅ **Git tags**: Releases are properly tagged (v1.8.2, etc.)
- ✅ **GitHub repository**: Public and accessible
- ✅ **Library name**: Unique (AlteriomPainlessMesh)
- ✅ **Version consistency**: All package files have matching versions
- ✅ **Header files**: Present in src/ directory
- ✅ **keywords.txt**: Present (optional but recommended)

**Current Status**: ✅ All checks passing

### library.properties Validation

Current library.properties contents:

```properties
name=AlteriomPainlessMesh
version=1.8.2
author=Coopdis,Scotty Franzyshen,Edwin van Leeuwen,Germán Martín,Maximilian Schwarz,Doanh Doanh,Alteriom
maintainer=Alteriom
sentence=A painless way to setup a mesh with ESP8266 and ESP32 devices with Alteriom extensions
paragraph=painlessMesh is a user-friendly library for creating mesh networks with ESP8266 and ESP32 devices. This Alteriom fork includes additional packages for sensor data (SensorPackage), device commands (CommandPackage), and status monitoring (StatusPackage). It handles routing and network management automatically, so you can focus on your application. The library uses JSON-based messaging and syncs time across all nodes, making it ideal for coordinated behaviour like synchronized light displays or sensor networks reporting to a central node.
category=Communication
url=https://github.com/Alteriom/painlessMesh
architectures=esp8266,esp32
includes=painlessMesh.h
depends=ArduinoJson, TaskScheduler
```

**Status**: ✅ All required fields present and properly formatted

### Submission Process

#### Step 1: Prepare Submission Information

Gather the following information for the submission:

```
Repository URL: https://github.com/Alteriom/painlessMesh
Library Name: AlteriomPainlessMesh
Current Version: 1.8.2
Latest Release Tag: v1.8.2
Category: Communication
Architectures: ESP8266, ESP32
Dependencies: ArduinoJson, TaskScheduler
License: LGPL-3.0
```

#### Step 2: Create Submission Issue

1. Go to: https://github.com/arduino/library-registry
2. Click on "Issues" tab
3. Click "New Issue"
4. Use the template below:

```markdown
## Add AlteriomPainlessMesh to Arduino Library Manager

**Repository URL**: https://github.com/Alteriom/painlessMesh

**Library Name**: AlteriomPainlessMesh

**Current Version**: 1.8.2

**Release Tag**: v1.8.2

**Description**: 
AlteriomPainlessMesh is a user-friendly library for creating mesh networks with ESP8266 and ESP32 devices. This is an enhanced fork of the original painlessMesh library with additional features:

- **SensorPackage** (Type 200): Environmental data collection with temperature, humidity, pressure monitoring
- **StatusPackage** (Type 202): Device health monitoring with memory, uptime, and WiFi metrics
- **CommandPackage** (Type 400): Remote device control and automation
- **MetricsPackage** (Type 204): Comprehensive performance metrics for dashboards
- **HealthCheckPackage** (Type 605): Proactive problem detection and predictive maintenance
- **Bridge Coordination** (Type 613): Multi-bridge support for high availability
- **Message Queue**: Offline message queuing for critical sensor data

The library handles mesh routing and network management automatically, uses JSON-based messaging, and syncs time across all nodes. Ideal for IoT sensor networks, smart agriculture, home automation, and industrial monitoring.

**Category**: Communication

**Architectures**: esp8266, esp32

**Dependencies**: 
- ArduinoJson (^7.4.2)
- TaskScheduler (^4.0.0)

**License**: LGPL-3.0

**Documentation**: https://alteriom.github.io/painlessMesh/

**Maintainer**: Alteriom (https://github.com/Alteriom)

**Additional Information**:
- Comprehensive CI/CD with automated testing
- 19+ working examples included
- 710+ unit tests passing
- Active maintenance and regular releases
- PlatformIO Registry: https://registry.platformio.org/libraries/sparck75/AlteriomPainlessMesh
- NPM Package: https://www.npmjs.com/package/@alteriom/painlessmesh

This library is ready for Arduino Library Manager indexing. All requirements are met:
- ✅ Valid library.properties file
- ✅ Proper directory structure (src/, examples/)
- ✅ Semantic versioning with git tags
- ✅ Open source license
- ✅ Examples compile successfully
- ✅ Comprehensive documentation
```

#### Step 3: Wait for Review

The Arduino team will review the submission:

1. **Automated checks**: The Arduino bot will validate the repository structure
2. **Manual review**: Arduino team will verify library quality
3. **Approval**: If everything is correct, the library will be added to `repositories.txt`
4. **Indexing**: The Arduino Library Manager will start indexing new releases

**Expected Timeline**: 1-2 weeks (can be longer depending on queue)

#### Step 4: Verify Registration

Once approved, verify the library is accessible:

1. Open Arduino IDE
2. Go to: Sketch → Include Library → Manage Libraries
3. Search for "AlteriomPainlessMesh"
4. Verify version 1.8.2 (or latest) appears
5. Test installation

### Alternative: Direct Pull Request (Advanced)

If you prefer to submit via pull request:

1. Fork: https://github.com/arduino/library-registry
2. Edit `repositories.txt`
3. Add line: `https://github.com/Alteriom/painlessMesh`
4. Create pull request with description
5. Wait for review and merge

**Note**: The issue submission method is recommended for first-time submissions.

## Post-Registration

### Automatic Updates

Once registered, future releases will be automatically indexed:

1. Create new release on GitHub with semantic version tag (e.g., v1.8.3)
2. Update `library.properties`, `library.json`, `package.json` versions
3. Arduino Library Manager automatically detects new releases
4. Users can update via Library Manager

**Update Frequency**: Arduino Library Manager checks for updates every 24-48 hours

### Maintenance

To ensure continued compatibility:

- Keep `library.properties` version in sync with git tags
- Follow semantic versioning (MAJOR.MINOR.PATCH)
- Test examples before each release
- Update CHANGELOG.md with changes
- Maintain backward compatibility when possible

## Testing Library Manager Installation

Once registered, test the installation process:

### Test 1: Fresh Installation

```
1. Open Arduino IDE
2. Sketch → Include Library → Manage Libraries
3. Search: "AlteriomPainlessMesh"
4. Click "Install"
5. Verify installation completes
6. Check: Tools → Manage Libraries → Installed
```

### Test 2: Example Compilation

```
1. File → Examples → AlteriomPainlessMesh → basic
2. Select board: ESP32 Dev Module or ESP8266 board
3. Verify/Compile sketch
4. Confirm compilation succeeds
```

### Test 3: Update Detection

```
1. Release new version (e.g., v1.8.3)
2. Wait 24-48 hours for indexing
3. Open Library Manager
4. Search: "AlteriomPainlessMesh"
5. Verify "Update" button appears
6. Click Update and verify installation
```

## Troubleshooting

### Library Not Appearing in Manager

**Cause**: Not yet indexed or submission not approved
**Solution**: 
- Check submission issue status
- Wait 24-48 hours after approval
- Verify `library.properties` has correct format

### Version Not Updating

**Cause**: Git tag doesn't match library.properties version
**Solution**:
- Ensure version in library.properties matches git tag
- Example: library.properties has `version=1.8.2` → git tag must be `v1.8.2`
- Push corrected tag to GitHub

### Compilation Errors in Library Manager

**Cause**: Missing dependencies or platform-specific issues
**Solution**:
- Verify `depends=` field in library.properties lists all dependencies
- Test compilation on target platforms before release
- Check Arduino forum for reported issues

### Old Version Showing (1.6.1 Issue)

**Cause**: Library not properly registered or name conflict
**Solution**:
- Complete Arduino Library Manager registration
- Verify library name is unique
- Check if old library version exists under different name

## Reference Links

- **Arduino Library Manager**: https://www.arduino.cc/reference/en/libraries/
- **Library Registry**: https://github.com/arduino/library-registry
- **Submission Guide**: https://support.arduino.cc/hc/en-us/articles/360012175419
- **Library Specification**: https://arduino.github.io/arduino-cli/latest/library-specification/
- **Library Manager FAQ**: https://support.arduino.cc/hc/en-us/articles/360016077340

## Support

If you encounter issues with the Arduino Library Manager submission:

1. **Check Documentation**: Review Arduino's official library submission guide
2. **Search Issues**: Look for similar submissions in arduino/library-registry issues
3. **Ask Arduino Forum**: Post questions in Arduino's library development forum
4. **Contact Maintainer**: Open issue in this repository for help

## Summary

**Action Required**: Submit the library to Arduino Library Manager registry

**Method**: Create issue at https://github.com/arduino/library-registry

**Expected Result**: Library becomes installable via Arduino IDE's Library Manager

**Timeline**: 1-2 weeks for review and approval

**Maintenance**: Future releases automatically indexed every 24-48 hours

Once registered, users will be able to:
- ✅ Search for "AlteriomPainlessMesh" in Arduino IDE
- ✅ Install with one click
- ✅ Update to latest versions automatically
- ✅ Access library documentation and examples
