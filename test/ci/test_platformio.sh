#!/bin/bash

# platformio ci --lib="." --board=nodemcuv2 examples/mqttBridge/mqttBridge.ino &&
platformio ci --lib="." --board=nodemcuv2 examples/basic/basic.ino -O "build_flags = -Werror" &&
platformio ci --lib="." --board=nodemcuv2 examples/startHere/startHere.ino -O "build_flags = -Wall -Wextra -Wno-unused-parameter -Werror" &&
platformio ci --lib="." --board=esp32dev examples/startHere/startHere.ino -O "build_flags = -std=c++17 -Wno-incompatible-pointer-types" &&
platformio ci --lib="." --board=nodemcuv2 examples/bridge/bridge.ino -O "build_flags = -Werror" &&
platformio ci --lib="." --board=nodemcuv2 examples/logServer/logServer.ino -O "build_flags = -Werror" &&
platformio ci --lib="." --board=nodemcuv2 examples/logClient/logClient.ino -O "build_flags = -Werror" &&
platformio ci --lib="." --lib="./examples/namedMesh/" --board=esp32dev examples/namedMesh/namedMesh.ino -O ""


