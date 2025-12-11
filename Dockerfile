# Dockerfile for painlessMesh Testing
# Provides a consistent build environment with all required tools

FROM ubuntu:24.04

# Prevent interactive prompts during installation
ENV DEBIAN_FRONTEND=noninteractive

# Install build essentials and dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    wget \
    curl \
    python3 \
    python3-pip \
    clang \
    libboost-all-dev \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /workspace

# Set C++ compiler to g++ (clang has issues with complex templates)
ENV CXX=g++
ENV CC=gcc

# Copy project files
COPY . /workspace/

# Clone test dependencies if they don't exist
RUN if [ ! -f /workspace/test/ArduinoJson/src/ArduinoJson.h ]; then \
    cd /workspace/test && \
    rm -rf ArduinoJson && \
    git clone --depth 1 https://github.com/bblanchon/ArduinoJson.git; \
fi

RUN if [ ! -f /workspace/test/TaskScheduler/src/TaskScheduler.h ]; then \
    cd /workspace/test && \
    rm -rf TaskScheduler && \
    git clone --depth 1 https://github.com/arkhipenko/TaskScheduler.git; \
fi

# Create build script
RUN echo '#!/bin/bash\n\
set -e\n\
echo "================================="\n\
echo "painlessMesh Build & Test System"\n\
echo "================================="\n\
echo ""\n\
echo "Configuring build..."\n\
cmake -G Ninja .\n\
echo ""\n\
echo "Building tests..."\n\
ninja\n\
echo ""\n\
echo "Running tests..."\n\
echo ""\n\
for test in bin/catch_*; do\n\
  if [ -x "$test" ]; then\n\
    echo "Running: $(basename $test)"\n\
    echo "-----------------------------------"\n\
    $test\n\
    echo ""\n\
  fi\n\
done\n\
echo "================================="\n\
echo "All tests completed successfully!"\n\
echo "================================="\n\
' > /workspace/run-tests.sh && chmod +x /workspace/run-tests.sh

# Default command
CMD ["/workspace/run-tests.sh"]
