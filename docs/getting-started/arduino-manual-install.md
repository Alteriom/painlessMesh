# Manual Arduino IDE Installation (ZIP Import)

This guide explains how to manually install painlessMesh in Arduino IDE using a ZIP file. This method is useful when:
- The Arduino Library Manager hasn't indexed the latest version yet
- You want to test unreleased versions from GitHub
- You need a specific version that's not in the Library Manager

## Quick Start

### Method 1: Download Release ZIP

1. **Download** the latest release ZIP from:
   - [GitHub Releases](https://github.com/Alteriom/painlessMesh/releases)
   - Look for `painlessMesh-vX.X.X.zip`

2. **Install in Arduino IDE**:
   - Open Arduino IDE
   - Go to **Sketch → Include Library → Add .ZIP Library...**
   - Select the downloaded `painlessMesh-vX.X.X.zip` file
   - Wait for "Library installed" message

3. **Install Dependencies**:
   - Go to **Sketch → Include Library → Manage Libraries...**
   - Search and install:
     - **ArduinoJson** (v6.21.x or v7.x)
     - **TaskScheduler** (v3.7.0+)

4. **Verify Installation**:
   - Go to **File → Examples → painlessMesh**
   - Open any example (e.g., `basic`)
   - Verify it compiles

### Method 2: Create ZIP from Repository

If you need to create a ZIP file from the repository source:

```bash
# Clone the repository
git clone https://github.com/Alteriom/painlessMesh.git
cd painlessMesh

# Create Arduino IDE compatible ZIP
./scripts/create-arduino-zip.sh
```

This creates `dist/painlessMesh-vX.X.X.zip` ready for Arduino IDE import.

## What's Included in the ZIP

The ZIP file contains:
- ✅ `src/` - Complete library source code
- ✅ `examples/` - All Arduino examples (19+ sketches)
- ✅ `library.properties` - Arduino library metadata
- ✅ `README.md` - Documentation
- ✅ `LICENSE` - LGPL-3.0 license
- ✅ `CHANGELOG.md` - Version history
- ✅ `keywords.txt` - Syntax highlighting

## Directory Structure

After installation, the library will be in:
- **Windows**: `Documents\Arduino\libraries\painlessMesh\`
- **macOS**: `~/Documents/Arduino/libraries/painlessMesh/`
- **Linux**: `~/Arduino/libraries/painlessMesh/`

```
painlessMesh/
├── src/                    # Library source files
│   ├── painlessMesh.h     # Main header
│   ├── painlessmesh/      # Core implementation
│   └── arduino/           # Arduino-specific code
├── examples/              # Example sketches
│   ├── basic/
│   ├── bridge/
│   ├── mqtt/
│   └── ...
├── library.properties     # Library metadata
├── README.md
├── LICENSE
└── keywords.txt
```

## Creating ZIP Files

### Automated Script

Use the provided script for consistent ZIP creation:

```bash
# From repository root
./scripts/create-arduino-zip.sh
```

**Output**: `dist/painlessMesh-vX.X.X.zip`

**Features**:
- ✅ Proper directory structure for Arduino IDE
- ✅ Includes all required files
- ✅ Excludes development files (.git, test/, .github/)
- ✅ Matches GitHub release format
- ✅ Version number from library.properties

### Manual ZIP Creation

If you need to create a ZIP manually:

1. **Create directory structure**:
   ```bash
   mkdir -p painlessMesh-package/painlessMesh
   cd painlessMesh-package/painlessMesh
   ```

2. **Copy required files**:
   ```bash
   # From your painlessMesh repository
   cp -r src examples library.properties README.md LICENSE .
   ```

3. **Optional additions**:
   ```bash
   cp CHANGELOG.md keywords.txt .
   ```

4. **Create ZIP**:
   ```bash
   cd ..
   zip -r painlessMesh-v1.8.3.zip painlessMesh/
   ```

5. **Important**: The ZIP must contain a folder named `painlessMesh` with the library files inside. Do NOT zip the files directly.

## Verification

After installation, verify it works:

### Test Compilation

```cpp
#include "painlessMesh.h"

painlessMesh mesh;

void setup() {
    Serial.begin(115200);
    Serial.println("painlessMesh test");
    
    mesh.init("TestNetwork", "password", 5555);
    Serial.println("Initialized successfully!");
}

void loop() {
    mesh.update();
}
```

If this compiles without errors, installation is successful!

### Check Examples

The examples should appear in:
**File → Examples → painlessMesh**

Available examples:
- basic - Simple mesh network
- bridge - Internet bridge mode
- startHere - Quick start template
- namedMesh - Named mesh networks
- And 15+ more examples

## Troubleshooting

### "Library not found" Error

**Problem**: Arduino IDE can't find the library after installation.

**Solutions**:
1. Restart Arduino IDE
2. Check installation directory:
   - Windows: `Documents\Arduino\libraries\`
   - macOS: `~/Documents/Arduino/libraries/`
   - Linux: `~/Arduino/libraries/`
3. Verify folder name is exactly `painlessMesh` (case-sensitive on Linux/macOS)
4. Check that `library.properties` exists in the painlessMesh folder

### "No such file or directory" Compilation Error

**Problem**: Missing dependency headers like `ArduinoJson.h` or `TaskScheduler.h`.

**Solution**:
Install dependencies via Library Manager:
1. **Sketch → Include Library → Manage Libraries...**
2. Search for:
   - **ArduinoJson** → Install version 6.21.x or 7.x
   - **TaskScheduler** → Install version 3.7.0+

### Version Mismatch

**Problem**: Old version still being used after installing new ZIP.

**Solution**:
1. Delete old version:
   - Go to `Arduino/libraries/` folder
   - Delete `painlessMesh` folder
2. Restart Arduino IDE
3. Install new ZIP file

### ZIP Structure Error

**Problem**: "Invalid library" error when importing ZIP.

**Cause**: Incorrect ZIP structure.

**Solution**:
Ensure ZIP contains:
```
painlessMesh-v1.8.3.zip
└── painlessMesh/           ← Must have this folder
    ├── src/
    ├── examples/
    └── library.properties  ← Must have this file
```

**Common mistake**: Zipping files directly without the `painlessMesh` folder.

## Comparing with Library Manager

| Feature | Manual ZIP Install | Library Manager |
|---------|-------------------|-----------------|
| **Latest Version** | ✅ Immediate | ⏱️ 24-48h delay |
| **Pre-release Testing** | ✅ Yes | ❌ No |
| **Custom Versions** | ✅ Yes | ❌ No |
| **Ease of Use** | ⚠️ Manual | ✅ Automatic |
| **Updates** | ⚠️ Manual | ✅ One-click |
| **Dependencies** | ⚠️ Manual | ✅ Auto-install |

## When to Use Each Method

### Use Manual ZIP Installation When:
- 🔧 Testing unreleased features from GitHub
- 🚀 Need the absolute latest version immediately
- 🐛 Testing a bug fix before official release
- 📦 Library Manager hasn't indexed latest version
- 🔬 Developing or contributing to painlessMesh

### Use Library Manager When:
- 📱 Installing for the first time
- 🔄 Need easy updates
- 👥 Stable production deployments
- 🎓 Learning or following tutorials
- 📚 Standard development workflow

## Automated Releases

The repository automatically creates proper ZIP files on each release:

1. **GitHub Actions** workflow builds ZIP on every release
2. **ZIP file** uploaded to GitHub Releases page
3. **Format** matches Arduino IDE requirements exactly
4. **Naming**: `painlessMesh-vX.X.X.zip`

Download from: https://github.com/Alteriom/painlessMesh/releases

## For Library Maintainers

### Creating Release ZIPs

The release workflow automatically creates proper ZIP files:

```yaml
# .github/workflows/release.yml
- name: Prepare library package
  run: |
    mkdir -p package/painlessMesh
    cp -r src examples library.properties README.md LICENSE package/painlessMesh/
    cd package
    zip -r ../painlessMesh-v${{ steps.version.outputs.version }}.zip painlessMesh/
```

### Script Usage

For local testing before release:

```bash
# Create ZIP for current version
./scripts/create-arduino-zip.sh

# Output: dist/painlessMesh-vX.X.X.zip
# Ready for testing in Arduino IDE
```

### Testing Installation

Before releasing:

1. Create ZIP with script
2. Install in fresh Arduino IDE
3. Test compilation of examples
4. Verify examples menu shows all sketches
5. Check dependency resolution

## Additional Resources

- 📚 [Full Installation Guide](../../website/docs/getting-started/installation.md)
- 🎯 [Quick Start Tutorial](../../website/docs/getting-started/quickstart.md)
- 🔧 [PlatformIO Installation](../../website/docs/getting-started/installation.md#platformio-installation)
- 🐛 [Troubleshooting Guide](../../website/docs/troubleshooting/common-issues.md)
- 📖 [API Reference](https://alteriom.github.io/painlessMesh/#/api/doxygen)

## Questions or Issues?

- 💬 [GitHub Discussions](https://github.com/Alteriom/painlessMesh/discussions)
- 🐛 [Report Issues](https://github.com/Alteriom/painlessMesh/issues)
- 📚 [Documentation](https://alteriom.github.io/painlessMesh/)
