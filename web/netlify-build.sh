#!/usr/bin/env bash
# Build the web demo on Netlify (see netlify.toml). Netlify runs this on every
# push to main and for every pull request (as a preview link).
# It installs the Emscripten SDK, then builds web/ exactly like CI does.
set -euo pipefail

EMSDK_VERSION="${EMSDK_VERSION:-4.0.10}"
CMAKE_VERSION="${CMAKE_VERSION:-3.31.6}"
EMSDK_DIR="${HOME}/emsdk"
CMAKE_DIR="${HOME}/cmake-${CMAKE_VERSION}"

echo "== tools: $(uname -m), $(python3 --version 2>&1), make $(make --version 2>/dev/null | head -1)"

if [ ! -d "$EMSDK_DIR" ]; then
    git clone --depth 1 https://github.com/emscripten-core/emsdk.git "$EMSDK_DIR"
fi
"$EMSDK_DIR/emsdk" install "$EMSDK_VERSION"
"$EMSDK_DIR/emsdk" activate "$EMSDK_VERSION"
# shellcheck disable=SC1091
source "$EMSDK_DIR/emsdk_env.sh"

# Netlify's build image has no CMake. Download the official portable build
# (no pip, no root needed) and put it on PATH.
if ! command -v cmake >/dev/null 2>&1; then
    if [ ! -x "$CMAKE_DIR/bin/cmake" ]; then
        echo "== downloading CMake ${CMAKE_VERSION}"
        python3 - "$CMAKE_VERSION" "$HOME" <<'PY'
import sys, tarfile, urllib.request, io
ver, home = sys.argv[1], sys.argv[2]
url = f"https://github.com/Kitware/CMake/releases/download/v{ver}/cmake-{ver}-linux-x86_64.tar.gz"
data = urllib.request.urlopen(url, timeout=120).read()
with tarfile.open(fileobj=io.BytesIO(data), mode="r:gz") as t:
    t.extractall(home)
PY
        mv "$HOME/cmake-${CMAKE_VERSION}-linux-x86_64" "$CMAKE_DIR"
    fi
    export PATH="$CMAKE_DIR/bin:$PATH"
fi
echo "== $(cmake --version | head -1), $(emcc --version | head -1)"

emcmake cmake -S web -B build-web
cmake --build build-web
test -s build-web/site/seam_carving.js
echo "Web demo built in build-web/site"
