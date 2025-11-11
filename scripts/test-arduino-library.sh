#!/bin/bash

# Arduino Library Test Script
# Tests that the library can be properly installed and examples compile
# This simulates what happens when a user imports the library from ZIP

set -e

echo "=================================================="
echo "Arduino Library Import & Compilation Test"
echo "=================================================="
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Track results
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0
FAILED_EXAMPLES=()

# Function to print test result
test_result() {
    local name="$1"
    local passed="$2"
    
    ((TOTAL_TESTS++))
    if [ "$passed" = "true" ]; then
        echo -e "${GREEN}✓ PASS${NC} - $name"
        ((PASSED_TESTS++))
    else
        echo -e "${RED}✗ FAIL${NC} - $name"
        ((FAILED_TESTS++))
        FAILED_EXAMPLES+=("$name")
    fi
}

# Check if arduino-cli is available
if ! command -v arduino-cli &> /dev/null; then
    echo -e "${RED}Error: arduino-cli not found${NC}"
    echo "Please install arduino-cli first:"
    echo "  curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh"
    exit 1
fi

echo -e "${BLUE}Step 1: Setup Arduino CLI configuration${NC}"
CONFIG_DIR="test/ci"
mkdir -p "$CONFIG_DIR"

if [ ! -f "$CONFIG_DIR/arduino-cli.yaml" ]; then
    arduino-cli config init --config-dir "$CONFIG_DIR"
fi

echo -e "${BLUE}Step 2: Add ESP board manager URLs${NC}"
arduino-cli config add board_manager.additional_urls http://arduino.esp8266.com/stable/package_esp8266com_index.json --config-dir "$CONFIG_DIR" 2>/dev/null || true

echo -e "${BLUE}Step 3: Update board indexes${NC}"
arduino-cli core update-index --config-dir "$CONFIG_DIR"

echo -e "${BLUE}Step 4: Install ESP32 and ESP8266 cores${NC}"
if ! arduino-cli core list --config-dir "$CONFIG_DIR" | grep -q "esp32:esp32"; then
    echo "Installing ESP32 core..."
    arduino-cli core install esp32:esp32 --config-dir "$CONFIG_DIR"
fi

if ! arduino-cli core list --config-dir "$CONFIG_DIR" | grep -q "esp8266:esp8266"; then
    echo "Installing ESP8266 core..."
    arduino-cli core install esp8266:esp8266 --config-dir "$CONFIG_DIR"
fi

echo -e "${BLUE}Step 5: Install library dependencies${NC}"
echo "Installing ArduinoJson..."
arduino-cli lib install ArduinoJson --config-dir "$CONFIG_DIR" 2>/dev/null || true

echo "Installing TaskScheduler..."
arduino-cli lib install TaskScheduler --config-dir "$CONFIG_DIR" 2>/dev/null || true

echo "Installing AsyncTCP..."
arduino-cli lib install AsyncTCP --config-dir "$CONFIG_DIR" 2>/dev/null || true

echo "Installing ESPAsyncTCP..."
arduino-cli lib install ESPAsyncTCP --config-dir "$CONFIG_DIR" 2>/dev/null || true

echo -e "${BLUE}Step 6: Install painlessMesh library from source${NC}"
# Simulate ZIP import by copying to Arduino libraries folder
ARDUINO_LIB_DIR=~/Arduino/libraries/Alteriom_PainlessMesh
mkdir -p "$ARDUINO_LIB_DIR"

# Copy library files
echo "Copying library files..."
cp -r src "$ARDUINO_LIB_DIR/"
cp -r examples "$ARDUINO_LIB_DIR/"
cp library.properties "$ARDUINO_LIB_DIR/"
cp keywords.txt "$ARDUINO_LIB_DIR/" 2>/dev/null || true
cp README.md "$ARDUINO_LIB_DIR/" 2>/dev/null || true
cp LICENSE "$ARDUINO_LIB_DIR/" 2>/dev/null || true

echo -e "${BLUE}Step 7: Verify library.properties${NC}"
if grep -q "includes=painlessMesh.h,AlteriomPainlessMesh.h" library.properties; then
    test_result "library.properties includes both headers" "true"
elif grep -q "includes=AlteriomPainlessMesh.h,painlessMesh.h" library.properties; then
    test_result "library.properties includes both headers" "true"
else
    test_result "library.properties includes both headers" "false"
    echo -e "${YELLOW}Warning: library.properties should include both painlessMesh.h and AlteriomPainlessMesh.h${NC}"
fi

echo ""
echo -e "${BLUE}Step 8: Compile examples${NC}"
echo ""

# Test critical examples for ESP32
ESP32_EXAMPLES=(
    "examples/startHere/startHere.ino"
    "examples/basic/basic.ino"
    "examples/alteriom/alteriom.ino"
    "examples/alteriomSensorNode/alteriom_sensor_node.ino"
)

# Test critical examples for ESP8266
ESP8266_EXAMPLES=(
    "examples/startHere/startHere.ino"
    "examples/basic/basic.ino"
)

echo -e "${BLUE}Testing ESP32 examples:${NC}"
for example in "${ESP32_EXAMPLES[@]}"; do
    if [ -f "$example" ]; then
        example_name=$(basename $(dirname "$example"))
        echo -n "  Compiling $example_name for ESP32... "
        if arduino-cli compile --fqbn esp32:esp32:esp32 --config-dir "$CONFIG_DIR" "$example" > /tmp/compile_esp32_$example_name.log 2>&1; then
            test_result "$example_name (ESP32)" "true"
        else
            test_result "$example_name (ESP32)" "false"
            echo -e "${YELLOW}    Log: /tmp/compile_esp32_$example_name.log${NC}"
        fi
    fi
done

echo ""
echo -e "${BLUE}Testing ESP8266 examples:${NC}"
for example in "${ESP8266_EXAMPLES[@]}"; do
    if [ -f "$example" ]; then
        example_name=$(basename $(dirname "$example"))
        echo -n "  Compiling $example_name for ESP8266... "
        if arduino-cli compile --fqbn esp8266:esp8266:generic --config-dir "$CONFIG_DIR" "$example" > /tmp/compile_esp8266_$example_name.log 2>&1; then
            test_result "$example_name (ESP8266)" "true"
        else
            test_result "$example_name (ESP8266)" "false"
            echo -e "${YELLOW}    Log: /tmp/compile_esp8266_$example_name.log${NC}"
        fi
    fi
done

# Summary
echo ""
echo "=================================================="
echo -e "${BLUE}Test Summary${NC}"
echo "=================================================="
echo "Total tests: $TOTAL_TESTS"
echo -e "Passed: ${GREEN}$PASSED_TESTS${NC}"
echo -e "Failed: ${RED}$FAILED_TESTS${NC}"

if [ $FAILED_TESTS -eq 0 ]; then
    echo ""
    echo -e "${GREEN}✓ ALL TESTS PASSED${NC}"
    echo ""
    echo "Library is ready for Arduino Library Manager!"
    echo "Users can successfully import and use the library."
    exit 0
else
    echo ""
    echo -e "${RED}✗ SOME TESTS FAILED${NC}"
    echo ""
    echo "Failed examples:"
    for failed in "${FAILED_EXAMPLES[@]}"; do
        echo "  - $failed"
    done
    echo ""
    echo "Please fix the issues before releasing."
    exit 1
fi
