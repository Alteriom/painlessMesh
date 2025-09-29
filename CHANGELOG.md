# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- TBD

### Changed

- TBD

### Fixed

- TBD

## [1.6.1] - 2025-09-29

### Added

- **Arduino Library Manager Support**: Updated library name to "Alteriom PainlessMesh" for better discoverability
- **NPM Package Publishing**: Complete NPM publication setup with dual registry support
  - Public NPM registry: `@alteriom/painlessmesh`
  - GitHub Packages registry: `@alteriom/painlessmesh` (scoped)
  - Automated version consistency across library.properties, library.json, and package.json
- **GitHub Wiki Automation**: Automatic wiki synchronization on releases
  - Home page generated from README.md
  - API Reference with auto-generated documentation
  - Examples page with links to repository examples
  - Installation guide for multiple platforms
- **Enhanced Release Workflow**: Comprehensive publication automation
  - NPM publishing to both public and GitHub Packages registries
  - GitHub Wiki updates with structured documentation
  - Arduino Library Manager submission instructions
  - Release validation with multi-file version consistency
- **Arduino IDE Support**: Added keywords.txt for syntax highlighting
- **Distribution Documentation**: Complete guides for all publication channels
  - RELEASE_SUMMARY.md template for release notes
  - TRIGGER_RELEASE.md for step-by-step release instructions
  - Enhanced RELEASE_GUIDE.md with comprehensive publication workflow
- **Version Management**: Enhanced bump-version script
  - Updates all three version files simultaneously (library.properties, library.json, package.json)
  - Comprehensive version consistency validation
  - Clear instructions for NPM and GitHub Packages publication

### Changed  
- **Library Name**: Updated from "Painless Mesh" to "Alteriom PainlessMesh" for Arduino Library Manager
- **Release Process**: Streamlined to support multiple package managers
  - Single commit with "release:" prefix triggers full publication pipeline
  - Automated testing, building, and publishing across all channels
  - Wiki documentation automatically synchronized
- **CI/CD Pipeline**: Enhanced with NPM publication capabilities
  - Dual NPM registry publishing (public + GitHub Packages)
  - Automated wiki updates with generated content
  - Comprehensive pre-release validation
- **Documentation Structure**: Reorganized for multi-channel distribution
  - Clear separation between automatic and manual processes
  - Platform-specific installation instructions
  - Troubleshooting guides for each distribution channel

### Fixed
- **NPM Publishing**: Fixed registry configuration issues that prevented NPM publication
- **GitHub Pages**: Improved workflow to handle cases where Pages is not configured
- **PlatformIO Build**: Fixed include paths in improved_sensor_node.ino example
- **Package Configuration**: Consistent version management across all package files
- **Release Documentation**: Complete coverage of all distribution channels
- **Version Validation**: Prevents releases with inconsistent version numbers

## [1.6.0] - 2025-09-29

### Added
- Enhanced Alteriom-specific package documentation and examples
- Updated repository URLs and metadata for Alteriom fork
- Improved release process documentation
- Fixed deprecated GitHub Actions in release workflow
- Added concurrency controls to prevent duplicate workflow runs
- Comprehensive Alteriom package documentation and quick start guide

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