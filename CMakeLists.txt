cmake_minimum_required (VERSION 3.10)
project (painlessMesh)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

FIND_PACKAGE(Boost CONFIG REQUIRED COMPONENTS system)
IF(Boost_FOUND)
  INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
  LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
ENDIF(Boost_FOUND)

FILE(GLOB TESTFILES test/catch/catch_*.cpp)
foreach(TESTFILE ${TESTFILES})
    get_filename_component(NAME ${TESTFILE} NAME_WE) 
    add_executable(${NAME} ${TESTFILE} test/catch/fake_serial.cpp src/scheduler.cpp src/connection.cpp)
    target_include_directories(${NAME} PUBLIC test/include/ test/catch/ test/ArduinoJson/src/ src/ test/TaskScheduler/src examples/)
endforeach()

# The OTA opt-out is a build-flag-only mechanism. Setting it in a single
# source file would give that translation unit a painlessmesh::Mesh with
# different members than the rest of the link -- an ODR violation with no
# required diagnostic -- so catch_ota_disabled gets it per target, for every
# TU in the target, exactly as SECURITY.md tells operators to set it.
if(TARGET catch_ota_disabled)
  target_compile_definitions(catch_ota_disabled PRIVATE PAINLESSMESH_DISABLE_OTA)
endif()

# NODE_TIMEOUT is the one macro operators are actually told to raise (see the
# gateway entry in CHANGELOG.md). Supplied per target rather than #defined in
# the source, for the same reason as above: the documented form is a build
# flag, and it has to reach every TU. 20s keeps the derived gateway budget
# inside the watchdog, which is the invariant the override must preserve.
if(TARGET catch_node_timeout_override)
  target_compile_definitions(catch_node_timeout_override PRIVATE
    "NODE_TIMEOUT=(20*TASK_SECOND)"
    "PAINLESSMESH_TEST_NODE_TIMEOUT_S=20")
endif()

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

add_executable(catch_tcp_integration test/boost/tcp_integration.cpp test/catch/fake_serial.cpp src/scheduler.cpp src/connection.cpp)
target_include_directories(catch_tcp_integration PUBLIC test/include/ test/boost/ test/ArduinoJson/src/ test/TaskScheduler/src/ src/)
TARGET_LINK_LIBRARIES(catch_tcp_integration ${Boost_LIBRARIES})

add_executable(catch_connection test/boost/connection.cpp test/catch/fake_serial.cpp src/scheduler.cpp src/connection.cpp)
target_include_directories(catch_connection PUBLIC test/include/ test/boost/ test/ArduinoJson/src/ test/TaskScheduler/src/ src/)
TARGET_LINK_LIBRARIES(catch_connection ${Boost_LIBRARIES})

# Note: Example validation tests have been moved to the external painlessMesh-simulator
# See examples/*/test/simulator/ directories and docs/SIMULATOR_TESTING.md for details
