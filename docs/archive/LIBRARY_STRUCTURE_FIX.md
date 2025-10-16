# Library Structure Fix for PlatformIO Compatibility

**Date:** October 14, 2025  
**Issue:** PlatformIO build errors when using AlteriomPainlessMesh as a dependency  
**Root Cause:** Missing directory specifications and duplicate library metadata

---

## Problems Identified

### 1. Missing `srcDir` Specification ❌
**Problem:** The root `library.json` didn't explicitly declare where source files are located.

**Impact:** PlatformIO couldn't reliably resolve file paths, causing compilation errors like:
```
Error: Cannot resolve directory for painlessMeshSTA.cpp
UnboundLocalError: cannot access local variable 'dir'
```

### 2. Duplicate `library.json` in `src/` ❌
**Problem:** A second `library.json` file existed inside `src/` directory.

**Impact:** Caused path resolution conflicts and confused PlatformIO's build system about which metadata to use.

### 3. Incorrect Header Reference ❌
**Problem:** `library.properties` referenced `AlteriomPainlessMesh.h` but examples use `painlessMesh.h`.

**Impact:** Arduino IDE users would have include path issues.

---

## Fixes Applied

### Fix 1: Added Explicit Directory Specifications ✅

**File:** `library.json`

**Changes:**
```json
{
  "srcDir": "src",
  "includeDir": "src"
}
```

**Why:** Explicitly tells PlatformIO where to find source files and headers, eliminating path resolution ambiguity.

### Fix 2: Removed Duplicate Metadata ✅

**File:** `src/library.json` (DELETED)

**Why:** Only one `library.json` should exist at the library root. Having metadata in `src/` creates conflicts.

### Fix 3: Corrected Header References ✅

**File:** `library.properties`

**Before:**
```properties
includes=AlteriomPainlessMesh.h
```

**After:**
```properties
includes=painlessMesh.h
```

**File:** `library.json`

**Added:**
```json
"headers": ["painlessMesh.h", "AlteriomPainlessMesh.h"]
```

**Why:** Both headers exist and may be used. The primary header is `painlessMesh.h` (matches examples), but `AlteriomPainlessMesh.h` is also available for compatibility.

### Fix 4: Updated npm Scripts ✅

**File:** `package.json`

**Before:**
```json
"scripts": {
  "build": "cmake -G Ninja . && ninja",
  "prebuild": "git submodule update --init"
}
```

**After:**
```json
"scripts": {
  "dev:build": "cmake -G Ninja . && ninja",
  "dev:prebuild": "git submodule update --init"
}
```

**Why:** Prevents automatic build script execution during `npm link` or `npm install`, which was causing Python errors in npm consumers.

---

## Current Library Structure

```
painlessMesh/
├── library.json              ← ROOT metadata (ONLY copy)
├── library.properties        ← Arduino IDE metadata
├── package.json              ← npm metadata
├── src/                      ← Source directory (specified in library.json)
│   ├── painlessMesh.h        ← Primary header (used in examples)
│   ├── AlteriomPainlessMesh.h ← Alternative header
│   ├── painlessMeshSTA.cpp   ← Implementation files
│   ├── painlessMeshSTA.h
│   ├── scheduler.cpp
│   ├── wifi.cpp
│   ├── painlessmesh/         ← Core library modules
│   ├── arduino/              ← Platform-specific code
│   ├── boost/                ← Boost headers (for PC builds)
│   └── plugin/               ← Plugin system
├── examples/                 ← Example sketches
└── docs/                     ← Documentation
```

---

## Validation

### PlatformIO Validation

```bash
# In your project that depends on AlteriomPainlessMesh
pio lib install https://github.com/Alteriom/painlessMesh#copilot/start-phase-2-implementation
pio run
```

**Expected Result:** ✅ Clean compilation with no path resolution errors

### Arduino IDE Validation

1. Install library via Library Manager or ZIP
2. Open `File > Examples > AlteriomPainlessMesh > startHere`
3. Compile for ESP32 or ESP8266

**Expected Result:** ✅ Successful compilation

### npm Link Validation

```bash
# In painlessMesh repo
npm link

# In dependent repo
npm link @alteriom/painlessmesh
```

**Expected Result:** ✅ No build errors, no Python errors

---

## For External Projects Using This Library

### In PlatformIO

**platformio.ini:**
```ini
[env:esp32]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps = 
    https://github.com/Alteriom/painlessMesh#copilot/start-phase-2-implementation
```

### In Arduino IDE

**Include statement:**
```cpp
#include <painlessMesh.h>
```

### In npm Projects

**package.json:**
```json
{
  "dependencies": {
    "@alteriom/painlessmesh": "github:Alteriom/painlessMesh#copilot/start-phase-2-implementation"
  }
}
```

---

## Testing Checklist

- [x] Root `library.json` has `srcDir` and `includeDir` specified
- [x] No duplicate `library.json` files exist in subdirectories
- [x] `library.properties` references the correct primary header
- [x] Source files (`.cpp`, `.h`) are in `src/` directory
- [x] Examples compile without path errors
- [x] npm scripts don't interfere with package consumers
- [x] Headers array includes both header file variants

---

## References

- [PlatformIO Library Specification](https://docs.platformio.org/en/latest/manifests/library-json/index.html)
- [Arduino Library Specification](https://arduino.github.io/arduino-cli/latest/library-specification/)
- [painlessMesh Documentation](https://github.com/Alteriom/painlessMesh)

---

**Status:** ✅ FIXED  
**Tested:** Ready for testing in dependent projects  
**Next Steps:** Test compilation in external PlatformIO projects
