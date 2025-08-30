#!/bin/bash

# Watch src, test/startHere/startHere.ino, and CMakeLists.txt for changes
# and run the platformio command
find src test/startHere/startHere.ino CMakeLists.txt | entr -r platformio ci --lib="." --board=esp32dev test/startHere/startHere.ino -O "build_flags = -std=c++14" &
# Watch src, test, and CMakeLists.txt for changes
# and run the cmake/make commands
find src test CMakeLists.txt | entr -r sh -c 'rm -f bin/catch_*; cmake . -DCMAKE_CXX_FLAGS="-Wall -Werror"; make -j4; run-parts --regex catch_ bin/'
