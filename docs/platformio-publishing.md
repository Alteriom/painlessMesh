# PlatformIO Library Publishing Guide

This guide explains how to publish the AlteriomPainlessMesh library to the PlatformIO Library Registry.

## Prerequisites

### 1. PlatformIO Account Setup

1. Visit [https://platformio.org/](https://platformio.org/) and create an account
2. Verify your email address
3. Log in to your PlatformIO account

### 2. Authentication Token

1. Go to [https://platformio.org/account/token](https://platformio.org/account/token)
2. Generate a new Personal Access Token
3. Copy the token and store it securely
4. Set up the token in your local environment:

   ```powershell
   # Set environment variable (Windows PowerShell)
   $env:PLATFORMIO_AUTH_TOKEN="YOUR_TOKEN_HERE"
   
   # Or for Command Prompt
   set PLATFORMIO_AUTH_TOKEN=YOUR_TOKEN_HERE
   
   # Or use login command
   pio account login
   ```

## Library Configuration
4. Set up the token in your local environment:
   ```powershell
   pio account token --set YOUR_TOKEN_HERE
   ```

## Library Configuration

### Required Files
Ensure these files are properly configured:

#### 1. `library.json` (Primary PlatformIO Configuration)
```json
{
    "name": "AlteriomPainlessMesh",
    "keywords": "ethernet, m2m, iot, mesh, alteriom, sensor, esp32, esp8266, json, time-sync, wireless, communication",
    "description": "painlessMesh library with Alteriom extensions for sensor networks",
    "repository": {
        "type": "git", 
        "url": "https://github.com/Alteriom/painlessMesh"
    },
    "version": "1.6.1",
    "frameworks": ["arduino"],
    "platforms": ["espressif8266", "espressif32"],
    "dependencies": [...],
    "authors": [...],
    "license": "LGPL-3.0",
    "homepage": "https://github.com/Alteriom/painlessMesh",
    "headers": "painlessMesh.h",
    "examples": ["examples/basic/basic.ino", "examples/alteriom/alteriom_sensor_node.ino"],
    "export": {
        "include": "src"
    }
}
```

#### 2. `library.properties` (Arduino Library Manager)
Should remain compatible for dual publishing:
```properties
name=AlteriomPainlessMesh
version=1.6.1
author=Coopdis,Scotty Franzyshen,Edwin van Leeuwen,Germán Martín,Maximilian Schwarz,Doanh Doanh,Alteriom
maintainer=Alteriom
sentence=A painless way to setup a mesh with ESP8266 and ESP32 devices with Alteriom extensions
paragraph=...
category=Communication
url=https://github.com/Alteriom/painlessMesh
architectures=esp8266,esp32
includes=AlteriomPainlessMesh.h
depends=ArduinoJson, TaskScheduler
```

## Publishing Process

### Method 1: Git Tag Publishing (Recommended)

This method automatically publishes when you create a Git tag:

1. **Ensure all files are committed and pushed:**
   ```powershell
   git add .
   git commit -m "Prepare v1.6.1 for PlatformIO Library Registry"
   git push origin main
   ```

2. **Create and push a Git tag:**
   ```powershell
   git tag v1.6.1
   git push origin v1.6.1
   ```

3. **PlatformIO will automatically detect the new tag and import the library**
   - Monitor at: [https://platformio.org/lib/show/LIBRARY_ID/AlteriomPainlessMesh](https://platformio.org/lib)
   - It may take 5-15 minutes for the library to appear

### Method 2: Manual Package Upload

If automatic detection doesn't work:

1. **Create a tarball of your library:**
   ```powershell
   # Create archive excluding unnecessary files
   tar --exclude='.git' --exclude='test' --exclude='bin' --exclude='.vscode' --exclude='node_modules' -czf AlteriomPainlessMesh-1.6.1.tar.gz .
   ```

2. **Submit via PlatformIO Library Registry:**
   - Visit [https://platformio.org/lib/register](https://platformio.org/lib/register)
   - Upload the created tarball
   - Fill in any additional metadata

### Method 3: Using PlatformIO CLI

```powershell
# Ensure you're authenticated
pio account token --set YOUR_TOKEN_HERE

# Publish the library
pio pkg publish .
```

## Verification

### 1. Check Library Status
```powershell
# Search for your published library
pio pkg search "AlteriomPainlessMesh"

# View detailed information
pio pkg show alteriom/AlteriomPainlessMesh
```

### 2. Test Installation
Create a test project to verify the library can be installed:
```powershell
mkdir test_project
cd test_project
pio project init --board esp32dev

# Add to platformio.ini:
# lib_deps = alteriom/AlteriomPainlessMesh@^1.6.1

pio pkg install
```

## Updating the Library

For future releases:

1. **Update version numbers:**
   - `library.json` → `"version": "1.6.2"`
   - `library.properties` → `version=1.6.2`
   - `package.json` → `"version": "1.6.2"`

2. **Commit changes:**
   ```powershell
   git add .
   git commit -m "Bump version to 1.6.2"
   git push origin main
   ```

3. **Create new tag:**
   ```powershell
   git tag v1.6.2
   git push origin v1.6.2
   ```

4. **Verify update appears in registry**

## Troubleshooting

### Common Issues

1. **Library name conflicts:**
   - Use a unique name like "AlteriomPainlessMesh" instead of "painlessMesh"
   - Check existing libraries: `pio pkg search "painless"`

2. **Authentication errors:**
   - Verify token: `pio account show`
   - Regenerate token if needed

3. **Dependency resolution errors:**
   - Ensure all dependencies exist in PlatformIO Registry
   - Check version constraints (use `^` for flexible versions)

4. **Git repository requirements:**
   - Repository must be publicly accessible
   - Tags must follow semantic versioning (v1.6.1)
   - library.json must be in repository root

### Useful Commands

```powershell
# Check authentication status
pio account show

# List all your published packages
pio pkg search --owner="YOUR_USERNAME"

# View package statistics
pio pkg stats

# Update package metadata
pio pkg update
```

## Best Practices

1. **Semantic Versioning:**
   - Use format: MAJOR.MINOR.PATCH (e.g., 1.6.1)
   - Increment MAJOR for breaking changes
   - Increment MINOR for new features
   - Increment PATCH for bug fixes

2. **Documentation:**
   - Include comprehensive README.md
   - Provide working examples in examples/ directory
   - Document all public APIs

3. **Testing:**
   - Test library installation in clean environments
   - Verify examples compile successfully
   - Test on both ESP8266 and ESP32 platforms

4. **Dependency Management:**
   - Specify minimum required versions
   - Use version ranges (^1.6.0) for flexibility
   - Platform-specific dependencies when needed

## PlatformIO Library Registry URLs

- **Library Registry:** [https://platformio.org/lib](https://platformio.org/lib)
- **Account Management:** [https://platformio.org/account](https://platformio.org/account)
- **Submit Library:** [https://platformio.org/lib/register](https://platformio.org/lib/register)
- **Documentation:** [https://docs.platformio.org/en/latest/librarymanager/index.html](https://docs.platformio.org/en/latest/librarymanager/index.html)

---

**Note:** After successful publication, users will be able to install your library using:
```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps = alteriom/AlteriomPainlessMesh@^1.6.1
```