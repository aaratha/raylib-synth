#!/usr/bin/env sh

./scripts/build-web.sh
cd build-web
python -m http.server
cd ../
