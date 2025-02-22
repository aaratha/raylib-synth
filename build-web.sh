#!/bin/bash

# Check if raylib is installed via Homebrew
if ! brew list raylib &>/dev/null; then
    echo "Installing raylib via Homebrew..."
    brew install raylib
fi

# Remove any old build directory
rm -rf build-web

export CMAKE_GENERATOR=Ninja

# Configure and build project
emcmake cmake -S . -B build-web -DPLATFORM=Web
cmake --build build-web

echo "Build complete! Deploying to GitHub Pages..."

# Store repository URL
REPO_URL=$(git remote get-url origin)

# Create temporary directory and copy build files
TMP_DIR=$(mktemp -d)
cp -r build-web/* "$TMP_DIR"

# Deploy to gh-pages branch
cd "$TMP_DIR"
git init
git checkout -b gh-pages
git add .
git commit -m "Deploy to GitHub Pages"
git remote add origin "$REPO_URL"
git push -f origin gh-pages

# Clean up
cd -
rm -rf "$TMP_DIR"

echo "Deployment complete! Your site is live at: https://<your-github-username>.github.io/<repo-name>/"

# Optional: Start local server for testing
# python -m http.server 8000 --directory build-web/
