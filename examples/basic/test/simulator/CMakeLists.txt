cmake_minimum_required(VERSION 3.10)
project(BasicExampleSimulatorTests)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Path to the simulator (submodule)
set(SIMULATOR_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../test/simulator")

# Include simulator's CMake configuration
if(EXISTS "${SIMULATOR_ROOT}/CMakeLists.txt")
    # Add simulator as subdirectory
    add_subdirectory(${SIMULATOR_ROOT} ${CMAKE_CURRENT_BINARY_DIR}/simulator)
else()
    message(FATAL_ERROR "Simulator not found at ${SIMULATOR_ROOT}. Did you initialize the submodule?")
endif()

# Include painlessMesh library
set(PAINLESS_MESH_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../..")
include_directories(
    ${PAINLESS_MESH_ROOT}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/firmware
)

# Register our custom firmware with the simulator
# This allows the YAML scenarios to reference "basic_example" template
add_library(basic_example_firmware INTERFACE)
target_include_directories(basic_example_firmware INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/firmware
)

# Link with simulator
target_link_libraries(basic_example_firmware INTERFACE
    painlessmesh_simulator
)

# The simulator executable will be built by the simulator's CMakeLists.txt
# We just need to ensure our firmware headers are available
message(STATUS "Basic example firmware tests configured")
message(STATUS "Run tests with: ./painlessmesh-simulator --config scenarios/basic_mesh_test.yaml")
