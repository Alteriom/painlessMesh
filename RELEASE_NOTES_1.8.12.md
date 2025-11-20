# Release Notes - Version 1.8.12

**Release Date:** 2025-11-19

## Overview

Version 1.8.12 is a maintenance release focused on documentation improvements and code quality enhancements. This release includes changes from PRs #152 and #153, ensuring comprehensive documentation coverage and adherence to code formatting standards.

## What's New

### Documentation Enhancements

- **Comprehensive Documentation Review** - All documentation has been reviewed and updated to reflect the current state of the library
- **API Reference Updates** - Improved clarity and completeness in API documentation
- **Example Code Documentation** - Enhanced inline documentation for example projects
- **Package Documentation** - Updated documentation for all Alteriom custom packages

### Code Quality Improvements

- **Prettier Configuration** - Added prettier configuration for consistent JSON and Markdown formatting
  - New `.prettierrc` configuration file
  - New `.prettierignore` file to exclude build artifacts and dependencies
  - Added `npm run format` and `npm run format:check` scripts
- **Clang-Format Compliance** - Ensured all C++ code follows established formatting standards
- **Code Organization** - Improved code structure and maintainability

### Version Updates

- Updated all version references across:
  - `package.json` - NPM package version
  - `library.json` - PlatformIO library version
  - `library.properties` - Arduino library version
  - `src/painlessMesh.h` - Header file version comments
  - `src/AlteriomPainlessMesh.h` - Version defines and comments

## Installation

### Arduino Library Manager
```
Sketch → Include Library → Manage Libraries → Search "AlteriomPainlessMesh" → Install
```

### PlatformIO
```ini
[env:myenv]
lib_deps = 
    alteriom/AlteriomPainlessMesh @ ^1.8.12
```

### NPM
```bash
npm install @alteriom/painlessmesh@1.8.12
```

## Compatibility

- **ESP32** - All variants supported
- **ESP8266** - All variants supported
- **Arduino IDE** - Version 1.8.0 or higher
- **PlatformIO** - Latest version
- **ArduinoJson** - Version 7.4.2 or higher
- **TaskScheduler** - Version 4.0.0 or higher

## Migration Guide

This is a patch release with no breaking changes. Users can upgrade from any 1.8.x version without code modifications.

## Known Issues

None identified in this release.

## Pull Requests Included

- PR #152 - Documentation improvements
- PR #153 - Code quality enhancements

## Contributors

Special thanks to all contributors who helped make this release possible!

## Documentation

- **Main Documentation**: https://alteriom.github.io/painlessMesh/
- **API Reference**: https://alteriom.github.io/painlessMesh/#/api/doxygen
- **Examples**: https://alteriom.github.io/painlessMesh/#/tutorials/basic-examples
- **GitHub Repository**: https://github.com/Alteriom/painlessMesh

## Support

For questions, issues, or feature requests:
- **GitHub Issues**: https://github.com/Alteriom/painlessMesh/issues
- **Documentation**: https://alteriom.github.io/painlessMesh/

## License

This project is licensed under LGPL-3.0. See the LICENSE file for details.
