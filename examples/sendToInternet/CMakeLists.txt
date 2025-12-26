cmake_minimum_required(VERSION 3.10)
project(PCMeshNode)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Find Boost (compatible with different CMake versions and platforms)
FIND_PACKAGE(Boost 1.66 COMPONENTS system)

IF(Boost_FOUND)
  INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
  LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
  message(STATUS "Boost found: ${Boost_VERSION}")
  message(STATUS "Boost include dirs: ${Boost_INCLUDE_DIRS}")
ELSE()
  message(FATAL_ERROR "Boost not found! Please install Boost development libraries:\n"
                      "  Ubuntu/Debian: sudo apt-get install libboost-dev libboost-system-dev\n"
                      "  macOS:         brew install boost\n"
                      "  Windows:       See https://www.boost.org/")
ENDIF()

# Set compiler flags
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

# Add executable
add_executable(pc_mesh_node 
    pc_mesh_node.cpp 
    ../../test/catch/fake_serial.cpp 
    ../../src/scheduler.cpp
)

# Include directories
target_include_directories(pc_mesh_node PUBLIC 
    ../../test/include/
    ../../test/boost/
    ../../test/ArduinoJson/src/
    ../../test/TaskScheduler/src/
    ../../src/
)

# Link libraries
TARGET_LINK_LIBRARIES(pc_mesh_node ${Boost_LIBRARIES})

# Print instructions
message(STATUS "")
message(STATUS "===========================================")
message(STATUS "PC Mesh Node - Build Configuration")
message(STATUS "===========================================")
message(STATUS "Build with: cmake . && make")
message(STATUS "Run with:   ./pc_mesh_node <bridge_ip> <bridge_port>")
message(STATUS "Example:    ./pc_mesh_node 192.168.1.100 5555")
message(STATUS "===========================================")
message(STATUS "")
