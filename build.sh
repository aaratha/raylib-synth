#!/bin/bash

# Check if raylib is installed via Homebrew
if ! brew list raylib &>/dev/null; then
    echo "Installing raylib via Homebrew..."
    brew install raylib
fi

# Remove any old build directory (optional but helps clear cache issues)
rm -rf build

# Configure and build project
cmake -B build
cmake --build build

echo "Build complete! Run ./build/trae_synth to start the application."

# Run the application
./build/trae_synth
