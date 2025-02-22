#!/bin/bash

# Check if raylib is installed via Homebrew
if ! brew list raylib &>/dev/null; then
    echo "Installing raylib via Homebrew..."
    brew install raylib
fi

# Remove any old build directory (optional but helps clear cache issues)
rm -rf build-web

export CMAKE_GENERATOR=Ninja

# Configure and build project
emcmake cmake -S . -B build-web -DPLATFORM=Web
cmake --build build-web

echo "Build complete! Run ./build/trae_synth to start the application."

python -m http.server 8000 --directory build-web/

# Run the application
