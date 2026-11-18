#!/usr/bin/env bash
# Build the web demo on Netlify (see netlify.toml). Netlify runs this on every
# push to main and for every pull request (as a preview link).
# It installs the Emscripten SDK, then builds web/ exactly like CI does.
set -euo pipefail

EMSDK_VERSION="${EMSDK_VERSION:-4.0.10}"
EMSDK_DIR="${HOME}/emsdk"

if [ ! -d "$EMSDK_DIR" ]; then
    git clone --depth 1 https://github.com/emscripten-core/emsdk.git "$EMSDK_DIR"
fi
"$EMSDK_DIR/emsdk" install "$EMSDK_VERSION"
"$EMSDK_DIR/emsdk" activate "$EMSDK_VERSION"
# shellcheck disable=SC1091
source "$EMSDK_DIR/emsdk_env.sh"

if ! command -v cmake >/dev/null 2>&1; then
    python3 -m pip install --user --quiet cmake
    export PATH="$HOME/.local/bin:$PATH"
fi

emcmake cmake -S web -B build-web
cmake --build build-web
test -s build-web/site/seam_carving.js
echo "Web demo built in build-web/site"
