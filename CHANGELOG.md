# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- State-of-the-art CI/CD pipeline with GitHub Actions
- Automated documentation generation and deployment
- Comprehensive build testing across multiple platforms (gcc, clang, Arduino, PlatformIO)
- Automated release management with semantic versioning
- Code quality checks and formatting validation

### Changed
- Migrated from GitLab CI to GitHub Actions for better integration
- Enhanced documentation structure and automated deployment

### Fixed
- Improved build consistency across different environments

## [1.5.7] - 2025-09-29

### Added
- Enhanced Alteriom-specific package documentation and examples
- Updated repository URLs and metadata for Alteriom fork
- Improved release process documentation

### Changed  
- Migrated from deprecated `actions/create-release@v1` to GitHub CLI for releases
- Updated library.properties and library.json to reflect Alteriom ownership
- Enhanced package descriptions to highlight Alteriom extensions

### Fixed
- Fixed deprecated GitHub Actions in release workflow  
- Corrected undefined variable references in upload workflow steps
- Updated repository URLs from GitLab to GitHub in library files

## [1.5.6] - Current Release

### Features
- painlessMesh is a user-friendly library for creating mesh networks with ESP8266 and ESP32 devices
- Automatic routing and network management
- JSON-based messaging system
- Time synchronization across all nodes
- Support for coordinated behaviors like synchronized displays
- Sensor network patterns for IoT applications

### Platforms Supported
- ESP32 (espressif32)
- ESP8266 (espressif8266)

### Dependencies
- ArduinoJson ^7.4.2
- TaskScheduler ^3.8.5
- AsyncTCP ^3.4.7 (ESP32)
- ESPAsyncTCP ^2.0.0 (ESP8266)

### Alteriom Extensions
- SensorPackage for environmental monitoring
- CommandPackage for device control
- StatusPackage for health monitoring
- Type-safe message handling

---

## Release Notes

### How to Release

1. Update version in both `library.properties` and `library.json`
2. Add changes to this CHANGELOG.md under the new version
3. Commit with message starting with `release:` (e.g., `release: v1.6.0`)
4. Push to main branch
5. GitHub Actions will automatically:
   - Create a git tag
   - Generate GitHub release
   - Package library for distribution
   - Update documentation

### Version Numbering

This project follows [Semantic Versioning](https://semver.org/):

- **MAJOR** version when you make incompatible API changes
- **MINOR** version when you add functionality in a backwards compatible manner  
- **PATCH** version when you make backwards compatible bug fixes

Additional labels for pre-release and build metadata are available as extensions to the MAJOR.MINOR.PATCH format.