#!/bin/bash

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

echo "Deployment complete! Your site is live at: https://aaratha.github.io/raylib-synth/"
