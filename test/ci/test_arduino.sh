#!/bin/bash

# TODO: Move the install part to separate script (here and gitlab-ci.yml)
export PATH=$PATH:./bin

wget https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh
sh install.sh
rm install.sh

arduino-cli core install esp32:esp32 --config-dir test/ci/
# arduino-cli core install esp8266:esp8266

# Install painlessmesh
git archive --format=zip --prefix=painlessmesh/ --output=repo.zip HEAD
arduino-cli lib install --zip-path repo.zip --config-dir test/ci/

# Install other dependencies
arduino-cli lib install ArduinoJson --config-dir test/ci/
arduino-cli lib install TaskScheduler --config-dir test/ci/
arduino-cli lib install AsyncTCP --config-dir test/ci/

arduino-cli compile --fqbn esp32:esp32:esp32 examples/startHere/startHere.ino --config-dir test/ci/ &&
arduino-cli compile --fqbn esp32:esp32:esp32 examples/namedMesh/namedMesh.ino --config-dir test/ci/ &&
arduino-cli compile --fqbn esp32:esp32:esp32 examples/otaReceiver/otaReceiver.ino --config-dir test/ci/ &&
arduino-cli compile --fqbn esp32:esp32:esp32 examples/otaSender/otaSender.ino --config-dir test/ci/
