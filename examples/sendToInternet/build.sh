#!/bin/bash
#
# Build and Setup Script for PC Mesh Node
#
# This script helps set up dependencies and build the PC mesh node emulator
# for testing sendToInternet() from regular nodes through a bridge.
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "=========================================="
echo "PC Mesh Node - Build & Setup"
echo "=========================================="
echo ""

# Check if we're in the right directory
if [ ! -f "pc_mesh_node.cpp" ]; then
    echo "Error: pc_mesh_node.cpp not found!"
    echo "Please run this script from the examples/sendToInternet directory"
    exit 1
fi

# Function to check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Check for required tools
echo "Checking required tools..."

if ! command_exists cmake; then
    echo "❌ CMake not found!"
    echo "   Install: sudo apt-get install cmake (Linux)"
    echo "           brew install cmake (macOS)"
    exit 1
fi
echo "✓ CMake found"

if ! command_exists g++; then
    echo "❌ g++ not found!"
    echo "   Install: sudo apt-get install g++ (Linux)"
    echo "           brew install gcc (macOS)"
    exit 1
fi
echo "✓ g++ found"

# Check for Boost
echo ""
echo "Checking Boost libraries..."
if ! ldconfig -p 2>/dev/null | grep -q libboost_system; then
    echo "⚠️  Boost not found or not in standard location"
    echo "   Attempting to continue anyway..."
    echo "   If build fails, install with:"
    echo "   - Linux: sudo apt-get install libboost-dev libboost-system-dev"
    echo "   - macOS: brew install boost"
else
    echo "✓ Boost found"
fi

# Check for test dependencies
echo ""
echo "Checking test dependencies..."

if [ ! -d "../../test/ArduinoJson/src" ]; then
    echo "⚠️  ArduinoJson not found"
    echo "   Cloning ArduinoJson..."
    cd ../../test
    if [ -d "ArduinoJson" ]; then
        rm -rf ArduinoJson
    fi
    git clone https://github.com/bblanchon/ArduinoJson.git
    cd "$SCRIPT_DIR"
else
    echo "✓ ArduinoJson found"
fi

if [ ! -d "../../test/TaskScheduler/src" ]; then
    echo "⚠️  TaskScheduler not found"
    echo "   Cloning TaskScheduler..."
    cd ../../test
    if [ -d "TaskScheduler" ]; then
        rm -rf TaskScheduler
    fi
    git clone https://github.com/arkhipenko/TaskScheduler
    cd "$SCRIPT_DIR"
else
    echo "✓ TaskScheduler found"
fi

# Clean previous build
echo ""
echo "Cleaning previous build..."
rm -rf CMakeFiles CMakeCache.txt cmake_install.cmake Makefile pc_mesh_node
echo "✓ Clean complete"

# Configure with CMake
echo ""
echo "Configuring with CMake..."
if cmake . 2>&1 | grep -q "Configuring done"; then
    echo "✓ Configuration successful"
else
    echo "❌ Configuration failed!"
    echo "   Check the error messages above"
    exit 1
fi

# Build
echo ""
echo "Building..."
if make 2>&1; then
    echo ""
    echo "=========================================="
    echo "✓ Build successful!"
    echo "=========================================="
    echo ""
    echo "Executable: ./pc_mesh_node"
    echo ""
    echo "Usage:"
    echo "  ./pc_mesh_node <bridge_ip> <mesh_port>"
    echo ""
    echo "Example:"
    echo "  ./pc_mesh_node 192.168.1.100 5555"
    echo ""
    echo "Next steps:"
    echo "  1. Start mock HTTP server:"
    echo "     cd ../../test/mock-http-server && python3 server.py"
    echo ""
    echo "  2. Configure and upload bridge to ESP32/ESP8266"
    echo "     (see sendToInternet.ino with IS_BRIDGE_NODE=true)"
    echo ""
    echo "  3. Run PC mesh node:"
    echo "     ./pc_mesh_node <bridge_ip> 5555"
    echo ""
    echo "For detailed instructions, see PC_NODE_README.md"
    echo "=========================================="
else
    echo ""
    echo "❌ Build failed!"
    echo "   Check the error messages above"
    exit 1
fi
