---
applies_to:
  - "test/**/*.cpp"
  - "test/**/*.hpp"
---

# Testing Instructions for painlessMesh

When working with test files in this repository, follow these guidelines:

## Test Framework
- Uses Catch2 for unit testing
- Tests are in `test/catch/catch_*.cpp`
- Integration tests use Boost.Asio in `test/boost/`

## Writing Tests

### Test File Structure
```cpp
#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"
#include "Arduino.h"              // Always include first
#include "catch_utils.hpp"
#include "examples/alteriom/alteriom_sensor_package.hpp"

using namespace alteriom;
using namespace painlessmesh;

// Declare logger for test environment
painlessmesh::logger::LogClass Log;

SCENARIO("Description of what you're testing") {
    GIVEN("Initial setup") {
        // Setup code
        
        WHEN("Action being tested") {
            // Test action
            
            THEN("Expected outcome") {
                REQUIRE(/* assertion */);
            }
        }
    }
}
```

### Testing Alteriom Packages
Always test serialization/deserialization:

```cpp
SCENARIO("Package serialization works correctly") {
    GIVEN("A package with test data") {
        auto pkg = YourPackage();
        pkg.from = 12345;
        pkg.yourField = 42;
        pkg.yourText = "test";
        
        REQUIRE(pkg.type == EXPECTED_TYPE_ID);
        
        WHEN("Converting to and from Variant") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<YourPackage>();
            
            THEN("Values should be preserved") {
                REQUIRE(pkg2.from == pkg.from);
                REQUIRE(pkg2.yourField == pkg.yourField);
                REQUIRE(pkg2.yourText == pkg.yourText);
                REQUIRE(pkg2.type == pkg.type);
            }
        }
    }
}
```

## Running Tests

### Build and Run Single Test
```bash
cmake -G Ninja .
ninja
./bin/catch_your_test_name
```

### Run All Tests
```bash
ninja
run-parts --regex catch_ bin/
```

### Run Specific Test Case
```bash
./bin/catch_your_test_name "Test case name"
```

## Common Testing Patterns

### Test Data Validation
```cpp
REQUIRE(pkg.temperature > -50.0);
REQUIRE(pkg.temperature < 100.0);
REQUIRE(pkg.sensorId != 0);
```

### Test Error Handling
```cpp
REQUIRE_THROWS_AS(invalidOperation(), std::runtime_error);
```

### Test JSON Serialization
```cpp
DynamicJsonDocument doc(1024);
JsonObject obj = doc.to<JsonObject>();
obj = pkg.addTo(std::move(obj));

REQUIRE(obj["field"] == expectedValue);
REQUIRE(obj["type"] == expectedType);
```

## Important Notes

1. **Use TSTRING not String**: In test environment, always use `TSTRING` type
2. **Include Arduino.h first**: Must be before other includes
3. **Declare Log**: Tests need `painlessmesh::logger::LogClass Log;`
4. **Test both directions**: Serialize → Deserialize → Verify
5. **Auto-discovery**: Name tests `catch_*.cpp` for automatic CMake inclusion

## Debugging Failed Tests

### Compilation Errors
- Check that all headers are included in correct order
- Verify `TSTRING` is used instead of `String`
- Ensure test dependencies (ArduinoJson, TaskScheduler) are cloned

### Runtime Failures
- Add debug output: `Log(GENERAL, "Value: %d\n", value);`
- Check JSON buffer sizes (may need to increase)
- Verify type IDs match between package definition and test

### Integration Test Issues
- Ensure Boost is installed: `sudo apt-get install libboost-dev`
- Check network timeouts aren't too aggressive
- Verify async operations complete before assertions

## Test Coverage Goals

For new Alteriom packages, ensure tests cover:
- [ ] Package construction
- [ ] Serialization to JSON
- [ ] Deserialization from JSON
- [ ] Round-trip conversion (package → JSON → package)
- [ ] All field values preserved correctly
- [ ] Type ID is correct
- [ ] Edge cases (empty strings, zero values, max values)
