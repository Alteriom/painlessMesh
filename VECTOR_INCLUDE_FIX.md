# Missing #include <vector> Fix

## Issue

Compilation errors when building projects that use AlteriomPainlessMesh:

```
error: 'vector' in namespace 'std' does not name a template type
   std::vector<ConnectionInfo> getConnectionDetails() {
        ^~~~~~
note: 'std::vector' is defined in header '<vector>'; did you forget to '#include <vector>'?
```

## Root Cause

The file `src/painlessmesh/mesh.hpp` uses `std::vector` but was missing the required include:

```cpp
#include <vector>
```

This caused compilation failures in any code that included `painlessMesh.h`.

## Files Affected

- `src/painlessmesh/mesh.hpp` - Uses `std::vector<ConnectionInfo>` at line 386
- `src/painlessmesh/mesh.hpp` - Uses `std::vector<uint32_t>` at line 594

## Fix Applied

Added `#include <vector>` to the top of `src/painlessmesh/mesh.hpp`:

```cpp
#ifndef _PAINLESS_MESH_MESH_HPP_
#define _PAINLESS_MESH_MESH_HPP_

#include <vector>  // ← ADDED THIS LINE

#include "painlessmesh/configuration.hpp"
// ... rest of includes
```

## Why This Wasn't Caught Earlier

This is a common C++ compilation issue where:
1. Sometimes the `<vector>` header gets transitively included by other headers
2. Works on some compilers/platforms but not others
3. Depends on which headers are included first in your project
4. May work in the library's test environment but fail in user projects

## Testing

After this fix, the following should compile successfully:

```cpp
#include <Arduino.h>
#include <painlessMesh.h>

Scheduler userScheduler;
painlessMesh mesh;

void setup() {
  Serial.begin(115200);
  mesh.init("TestMesh", "password", &userScheduler, 5555);
  
  // This function uses std::vector internally
  auto connections = mesh.getConnectionDetails();
}

void loop() {
  mesh.update();
}
```

## For Users

To get the fix:

1. **Clean your build:**
   ```bash
   pio run --target clean
   rm -rf .pio
   ```

2. **Update library:**
   ```bash
   pio pkg update
   ```

3. **Rebuild:**
   ```bash
   pio run
   ```

The library will be re-downloaded from GitHub with the fix included.

## Verification

Your build output should change from:

**Before (ERROR):**
```
error: 'vector' in namespace 'std' does not name a template type
*** [.pio/build/universal-sensor/libb89/AlteriomPainlessMesh/painlessMeshSTA.cpp.o] Error 1
```

**After (SUCCESS):**
```
Building .pio/build/universal-sensor/libb89/AlteriomPainlessMesh/painlessMeshSTA.cpp.o
Building .pio/build/universal-sensor/libb89/AlteriomPainlessMesh/wifi.cpp.o
Linking .pio/build/universal-sensor/firmware.elf
SUCCESS
```

## Related Fixes

This is part of a series of library structure fixes:
1. ✅ Added `srcDir` and `includeDir` to library.json
2. ✅ Removed conflicting `export.include` section
3. ✅ Fixed header references in library.properties
4. ✅ **Added missing `#include <vector>` to mesh.hpp** ← This fix

## Date

October 15, 2025

## Status

✅ FIXED - Ready for use
