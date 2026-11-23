#!/usr/bin/env bash
# Build the third-party plugins in plugins/third-party (git submodules pinned to
# the versions used in production) against this OBS build.
# Runs inside the build container after OBS was installed into PREFIX.
set -euo pipefail

PREFIX="${1:?install prefix required}"
SRC=/src/plugins/third-party
BUILD=/src/build_docker/third-party
JOBS="$(nproc)"

cmake_plugin() {
  local name="$1"
  shift
  cmake -S "$SRC/$name" -B "$BUILD/$name" -G Ninja \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_PREFIX_PATH="$PREFIX" \
    -DCMAKE_INSTALL_PREFIX="$PREFIX" \
    "$@"
  cmake --build "$BUILD/$name" -j"$JOBS"
  cmake --install "$BUILD/$name"
}

# PipeWire audio capture (GPL-2.0)
cmake_plugin obs-pipewire-audio-capture

# Background removal (GPL-3.0), system OpenCV + ONNX Runtime
cmake_plugin obs-backgroundremoval \
  -DUSE_PKGCONFIG=ON \
  -DUSE_SYSTEM_ONNXRUNTIME=ON \
  -DENABLE_FRONTEND_API=ON \
  -DENABLE_QT=ON

# DroidCam (GPL-2.0), plain Makefile; built out of tree to keep the submodule clean.
# Its link line lists libraries before objects, so --as-needed must be disabled
# ahead of them (LDD_DIRS precedes LDD_LIBS on that line).
mkdir -p "$BUILD/droidcam-obs"
make -C "$SRC/droidcam-obs" -j"$JOBS" \
  BUILD_DIR="$BUILD/droidcam-obs" \
  LIBOBS_INCLUDES="$PREFIX/include/obs" \
  FFMPEG_INCLUDES=/usr/include/x86_64-linux-gnu \
  LIBUSBMUXD=libusbmuxd-2.0 \
  LIBIMOBILEDEV=libimobiledevice-1.0 \
  LDD_DIRS="-L$PREFIX/lib -Wl,--no-as-needed"
install -Dm755 "$BUILD/droidcam-obs/droidcam-obs.so" "$PREFIX/lib/obs-plugins/droidcam-obs.so"
mkdir -p "$PREFIX/share/obs/obs-plugins/droidcam-obs"
cp -R "$SRC/droidcam-obs/data/." "$PREFIX/share/obs/obs-plugins/droidcam-obs/"
