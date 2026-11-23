#!/usr/bin/env bash
# Start the Docker-built OBS Studio GUI on the host's X11 display.
# Requires a prior build-aux/docker/build.sh run.
# Settings persist in build_docker/config (separate from a system OBS install).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
IMAGE="obs-studio-build:ubuntu26.04"
mkdir -p "$ROOT/build_docker/config/.config/obs-studio"

# Use the NVIDIA GPU (OpenGL + NVENC) when nvidia-container-toolkit is set up.
GPU_ARGS=()
if docker info --format '{{json .Runtimes}}' 2>/dev/null | grep -q '"nvidia"'; then
  GPU_ARGS=(--runtime=nvidia -e NVIDIA_VISIBLE_DEVICES=all -e NVIDIA_DRIVER_CAPABILITIES=all)
fi

docker run --rm "${GPU_ARGS[@]}" \
  --user "$(id -u):$(id -g)" \
  --group-add video --group-add "$(getent group render | cut -d: -f3 || echo video)" \
  -e HOME=/src/build_docker/config \
  -e LANG=C.UTF-8 \
  -e DISPLAY="${DISPLAY:-:0}" \
  -e XDG_RUNTIME_DIR=/tmp/xdg \
  -e PULSE_SERVER="unix:${XDG_RUNTIME_DIR}/pulse/native" \
  -e LD_LIBRARY_PATH=/src/build_docker/install/lib \
  -v /tmp/.X11-unix:/tmp/.X11-unix \
  -v /etc/machine-id:/etc/machine-id:ro \
  -v "${XDG_RUNTIME_DIR}/pulse:${XDG_RUNTIME_DIR}/pulse" \
  -e PIPEWIRE_RUNTIME_DIR="${XDG_RUNTIME_DIR}" \
  -v "${XDG_RUNTIME_DIR}/pipewire-0:${XDG_RUNTIME_DIR}/pipewire-0" \
  --device /dev/dri \
  --ipc=host \
  -v "$ROOT:/src" \
  -w /src/build_docker/install/bin \
  "$IMAGE" ./obs "$@"
