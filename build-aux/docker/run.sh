#!/usr/bin/env bash
# Start the Docker-built OBS Studio GUI on the host's X11 display.
# Requires a prior build-aux/docker/build.sh run.
# Settings persist in build_docker/config (separate from a system OBS install).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
IMAGE="obs-studio-build:ubuntu26.04"
mkdir -p "$ROOT/build_docker/config/.config/obs-studio"

docker run --rm \
  --user "$(id -u):$(id -g)" \
  --group-add video --group-add "$(getent group render | cut -d: -f3 || echo video)" \
  -e HOME=/src/build_docker/config \
  -e LANG=C.UTF-8 \
  -e DISPLAY="${DISPLAY:-:0}" \
  -e XDG_RUNTIME_DIR=/tmp/xdg \
  -e PULSE_SERVER="unix:${XDG_RUNTIME_DIR}/pulse/native" \
  -e LD_LIBRARY_PATH=/src/build_docker/install/lib \
  -v /tmp/.X11-unix:/tmp/.X11-unix \
  -v "${XDG_RUNTIME_DIR}/pulse:${XDG_RUNTIME_DIR}/pulse" \
  --device /dev/dri \
  --ipc=host \
  -v "$ROOT:/src" \
  -w /src/build_docker/install/bin \
  "$IMAGE" ./obs "$@"
