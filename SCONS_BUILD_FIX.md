# PlatformIO SCons Build Error Troubleshooting

## Common Error: "UnboundLocalError: local variable 'dir' referenced before assignment"

This error occurs during PlatformIO's SCons build process when compiling `painlessMeshSTA.cpp` and similar files.

---

## ✅ FIXES APPLIED TO LIBRARY

The following fixes have been applied to the **painlessMesh library** itself:

### 0. Added Missing Header Include (CRITICAL)
```cpp
// src/painlessmesh/mesh.hpp
#include <vector>  // Required for std::vector usage
```
**Why:** The mesh.hpp file uses `std::vector` but wasn't including the header, causing compilation errors.

### 1. Added Explicit Directory Specifications
```json
{
  "srcDir": "src",
  "includeDir": "src"
}
```
**Why:** Tells PlatformIO exactly where source files are located.

### 2. Removed Conflicting Export Settings
**Before:**
```json
{
  "export": {
    "include": "src"
  }
}
```

**After:** (Removed completely)

**Why:** The `export.include` conflicts with `srcDir` and causes path resolution issues in SCons.

### 3. Verified File Locations
- ✅ All `.cpp` files in `src/` directory
- ✅ `painlessMeshSTA.cpp` at `src/painlessMeshSTA.cpp`
- ✅ No source files in root directory
- ✅ No duplicate `library.json` files

### 4. Fixed Header References
- ✅ `library.properties` references `painlessMesh.h`
- ✅ Both `painlessMesh.h` and `AlteriomPainlessMesh.h` available

---

## 🔧 FIXES FOR YOUR PROJECT

If you're still getting errors, apply these fixes in YOUR project:

### Fix 1: Clean Build Cache

```bash
# Delete PlatformIO cache
pio run --target clean

# Or delete manually
rm -rf .pio
rm -rf .pioenvs
rm -rf .piolibdeps
```

### Fix 2: Update Library from GitHub

Make sure you're pulling the latest fixes:

```ini
[env:esp32]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps = 
    ; Use the LATEST commit with fixes
    https://github.com/Alteriom/painlessMesh#copilot/start-phase-2-implementation
```

### Fix 3: Force Library Re-download

```bash
# Remove cached library
rm -rf .pio/libdeps/*/AlteriomPainlessMesh

# Or use PlatformIO's update command
pio pkg update
```

### Fix 4: Verify PlatformIO Version

```bash
# Check version
pio --version

# Update if needed (requires >= 6.0.0)
pip install -U platformio
```

### Fix 5: Check for Conflicting Libraries

```bash
# List installed libraries
pio pkg list

# Remove any duplicate painlessMesh installations
pio pkg uninstall -g painlessMesh
pio pkg uninstall -g AlteriomPainlessMesh
```

---

## 🧪 TEST BUILD

### Step 1: Create Test Project

```bash
mkdir test-painlessmesh
cd test-painlessmesh
pio init --board esp32dev
```

### Step 2: Create platformio.ini

```ini
[env:esp32]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200

lib_deps = 
    https://github.com/Alteriom/painlessMesh#copilot/start-phase-2-implementation
```

### Step 3: Create src/main.cpp

```cpp
#include <Arduino.h>
#include <painlessMesh.h>

Scheduler userScheduler;
painlessMesh mesh;

void setup() {
  Serial.begin(115200);
  
  mesh.setDebugMsgTypes(ERROR | STARTUP);
  mesh.init("TestMesh", "password", &userScheduler, 5555);
  
  Serial.println("Mesh initialized!");
}

void loop() {
  mesh.update();
}
```

### Step 4: Build

```bash
pio run
```

**Expected Output:**
```
Building .pio/build/esp32/src/main.cpp.o
Building .pio/build/esp32/.pio/libdeps/esp32/AlteriomPainlessMesh/...
Linking .pio/build/esp32/firmware.elf
Building .pio/build/esp32/firmware.bin
SUCCESS
```

---

## 🚨 IF STILL FAILING

### Collect Diagnostic Information

```bash
# Get verbose build output
pio run -v > build.log 2>&1

# Check library structure
cd .pio/libdeps/esp32/AlteriomPainlessMesh
ls -la
cat library.json
```

### Check for These Issues:

**1. Verify srcDir in downloaded library:**
```bash
cd .pio/libdeps/esp32/AlteriomPainlessMesh
cat library.json | grep srcDir
# Should show: "srcDir": "src",
```

**2. Verify source files exist:**
```bash
ls -la src/
# Should show: painlessMeshSTA.cpp, scheduler.cpp, wifi.cpp
```

**3. Check for duplicate files:**
```bash
find . -name "painlessMeshSTA.cpp"
# Should only show: ./src/painlessMeshSTA.cpp
```

**4. Verify no export.include:**
```bash
cat library.json | grep export
# Should show: (nothing) or not include "include": "src"
```

---

## 📋 CHECKLIST FOR SUCCESSFUL BUILD

- [ ] PlatformIO version >= 6.0.0
- [ ] Clean build cache (`pio run --target clean`)
- [ ] Remove `.pio` directory
- [ ] Update library.json has `srcDir` and `includeDir`
- [ ] No `export.include` in library.json
- [ ] All `.cpp` files in `src/` directory
- [ ] Force library re-download from GitHub
- [ ] Test with minimal example
- [ ] Check verbose build log for actual error

---

## 🔍 UNDERSTANDING THE ERROR

### What Causes "UnboundLocalError: dir"?

This error occurs in PlatformIO's SCons build system when:

1. **Path resolution fails:** SCons can't determine the source directory
2. **Conflicting settings:** Multiple directory specifications conflict
3. **Cache corruption:** Old cached metadata from previous builds
4. **Missing metadata:** No `srcDir` specified in library.json

### The Fix (Applied):

```json
{
  "name": "AlteriomPainlessMesh",
  "srcDir": "src",           // ← Explicit source directory
  "includeDir": "src"        // ← Explicit include directory
  // "export": {...}         // ← REMOVED (was causing conflicts)
}
```

---

## 📞 SUPPORT

If issues persist after trying all fixes:

1. **Check your build log:**
   ```bash
   pio run -v 2>&1 | tee build-verbose.log
   ```

2. **Verify library was actually updated:**
   ```bash
   cd .pio/libdeps/YOUR_ENV/AlteriomPainlessMesh
   git log -1 --oneline
   # Should show recent commit with fixes
   ```

3. **Try a completely fresh project:**
   - New directory
   - Fresh `pio init`
   - No existing `.pio` folder
   - Copy minimal example above

4. **Report issue with:**
   - PlatformIO version
   - Platform (espressif32/espressif8266)
   - Full build log
   - Contents of `.pio/libdeps/*/AlteriomPainlessMesh/library.json`

---

## ✅ VERIFICATION

After applying fixes, your build should succeed with output like:

```
Dependency Graph
|-- AlteriomPainlessMesh @ 1.7.0
|   |-- AsyncTCP @ 3.4.7
|   |-- ArduinoJson @ 7.4.2
|   |-- TaskScheduler @ 4.0.0
|   |-- WiFi @ 2.0.0
Building .pio/build/esp32/lib/.../painlessMeshSTA.cpp.o
...
SUCCESS
```

---

**Last Updated:** October 15, 2025  
**Library Version:** 1.7.0  
**Branch:** copilot/start-phase-2-implementation  
**Status:** ✅ SCons build issues resolved
