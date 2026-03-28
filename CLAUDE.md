# CLAUDE.md — painlessMesh

C++ mesh networking library for ESP32/ESP8266 (PlatformIO). Alteriom fork of painlessMesh v1.9.20.

## Key Directories

- `src/painlessmesh/` — core headers (mesh, connection, router, gateway, protocol, etc.)
- `src/arduino/` — Arduino-specific implementation
- `src/plugin/` — plugin system
- `examples/alteriom/` — Alteriom-specific examples
- `test/catch/` — Catch2 unit tests
- `test/boost/` — Boost/TCP integration tests
- `bin/` — compiled test binaries

## Build

```bash
# Desktop (Catch2 tests)
cmake -G Ninja . && ninja

# Run all tests
for bin in bin/catch_*; do timeout 30 "$bin"; done

# Embedded — use PlatformIO
pio run -e <env>
```

## Gotchas

**ArduinoJson v7:**
- `JsonDocument doc(size)` does NOT work on ESP32 — use `JsonDocument doc;` (default constructor, grows dynamically)
- Never use internal namespaces like `ArduinoJson::V742PB22::` — use public types directly

**PlatformIO:**
- Only compiles `.cpp` files in `srcDir` root, not subdirectories
- `connection.cpp` must live in `src/` not `src/painlessmesh/` — it was moved there intentionally

**Package types:**
- 200-series: Alteriom custom (201 device announcement, 202 sensor data, 203 reserved, 204 gateway data, 205 MPPT data)
- `MpptPackage` uses type **205** (not 203 — easy to mix up)
- 400+: extended Alteriom
- 600+: mesh internal
- 610+: bridge

## Version

Current: **v1.9.20**
