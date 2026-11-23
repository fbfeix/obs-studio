#!/usr/bin/env bash
# Build OBS Studio for Linux inside a Docker container.
# Usage: build-aux/docker/build.sh [extra cmake args...]
# Output: build_docker/ (build tree) and build_docker/install (runnable install)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
IMAGE="obs-studio-build:ubuntu26.04"

# Forks often lack release tags; CMake derives the version from `git describe`.
if ! git -C "$ROOT" describe --tags >/dev/null 2>&1; then
  git -C "$ROOT" fetch --no-recurse-submodules https://github.com/obsproject/obs-studio.git 'refs/tags/*:refs/tags/*'
fi

git -C "$ROOT" submodule update --init plugins/obs-browser plugins/obs-websocket

docker build --network=host -t "$IMAGE" "$ROOT/build-aux/docker"

docker run --rm \
  --user "$(id -u):$(id -g)" \
  -e HOME=/tmp -e CCACHE_DIR=/src/.ccache \
  -v "$ROOT:/src" \
  "$IMAGE" bash -c '
    set -euo pipefail
    cmake -S . -B build_docker -G Ninja \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DCMAKE_INSTALL_PREFIX=/src/build_docker/install \
      -DENABLE_CCACHE=ON \
      -DENABLE_BROWSER=OFF \
      -DENABLE_VST3=OFF \
      -DENABLE_AJA=OFF \
      -DENABLE_WEBRTC=OFF \
      -DENABLE_TESTS=ON \
      "$@"
    cmake --build build_docker -j"$(nproc)"
    cmake --install build_docker
    ctest --test-dir build_docker --output-on-failure
  ' _ "$@"
