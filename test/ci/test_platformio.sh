
#!/bin/bash

# Define the examples directory
EXAMPLES_DIR="examples"

# Check if the examples directory exists
if [ ! -d "$EXAMPLES_DIR" ]; then
    echo "Error: '$EXAMPLES_DIR' directory not found."
    exit 1
fi

# Usage function
usage() {
    echo "Usage: $0 [--example EXAMPLE_NAME]"
    echo "  --example EXAMPLE_NAME: Build only the specified example"
    exit 1
}

# Parse command line arguments
while [[ "$#" -gt 0 ]]; do
    case $1 in
        --example) EXAMPLE="$2"; shift ;;
        *) echo "Unknown parameter: $1"; usage ;;
    esac
    shift
done

# Function to build a single example
build_example() {
    local dir="$1"
    echo "Entering directory: $dir"
    cd "$dir" || { echo "Error: Failed to enter directory $dir"; exit 1; }

    echo "Building for esp32 in $dir"
    if ! pio run -e esp32; then
        echo "Error: Build failed for esp32 in $dir"
        exit 1
    fi

    echo "Building for esp8266 in $dir"
    if ! pio run -e esp8266; then
        echo "Error: Build failed for esp8266 in $dir"
        exit 1
    fi

    cd - > /dev/null || { echo "Error: Failed to return to original directory"; exit 1; }
}

# Build all examples or just the specified one
if [ -z "$EXAMPLE" ]; then
    # Build all examples
    for dir in "$EXAMPLES_DIR"/*/; do
        build_example "$dir"
    done
else
    # Build only the specified example
    dir="$EXAMPLES_DIR/$EXAMPLE"
    if [ ! -d "$dir" ]; then
        echo "Error: Example '$EXAMPLE' not found in $EXAMPLES_DIR"
        exit 1
    fi
    build_example "$dir"
fi

echo "All builds completed successfully."
exit 0
