#!/bin/bash

# Check if raylib is installed via Homebrew
if ! brew list raylib &>/dev/null; then
    echo "Installing raylib via Homebrew..."
    brew install raylib
fi

# Create and enter build directory
mkdir -p build
cd build

# Configure and build project
cmake .. -B build -DBUILDING_CORE=1  # Define BUILDING_CORE for exports
cmake --build build

echo "Build complete! Run ./build/trae_synth to start the application."

# Run the application
./trae_synth
