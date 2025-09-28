#!/bin/bash

# Watch src, test/startHere/startHere.ino, and CMakeLists.txt for changes
# and run the platformio command
find src CMakeLists.txt | entr -r bash test/ci/test_platformio.sh --random &
# Watch src, test, and CMakeLists.txt for changes
# and run the cmake/make commands
find src test CMakeLists.txt | entr -r sh -c 'rm -f bin/catch_*; cmake . -DCMAKE_CXX_FLAGS="-Wall -Werror"; make -j4; run-parts --regex catch_ bin/'
