#!/bin/bash

# Create directories
mkdir -p external/miniaudio

# Download miniaudio.h if it doesn't exist
if [ ! -f external/miniaudio/miniaudio.h ]; then
    echo "Downloading miniaudio.h..."
    curl -o external/miniaudio/miniaudio.h https://raw.githubusercontent.com/mackron/miniaudio/master/miniaudio.h
fi

echo "miniaudio setup complete!"