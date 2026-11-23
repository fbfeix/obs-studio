# Reproducible Linux build environment for OBS Studio (Ubuntu 26.04, matches upstream CI).
# Package list mirrors .github/scripts/utils.zsh/setup_ubuntu.
FROM ubuntu:26.04

RUN echo 'Acquire::ForceIPv4 "true";' > /etc/apt/apt.conf.d/99force-ipv4 \
  && apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
  build-essential cmake ninja-build git ca-certificates pkg-config ccache \
  libglib2.0-dev extra-cmake-modules lsb-release dh-cmake \
  libcurl4-openssl-dev \
  libavcodec-dev libavdevice-dev libavfilter-dev libavformat-dev libavutil-dev \
  libswresample-dev libswscale-dev \
  libjansson-dev libx11-xcb-dev libgles2-mesa-dev libwayland-dev \
  libpipewire-0.3-dev libpulse-dev libx264-dev libmbedtls-dev libgl1-mesa-dev \
  uthash-dev libsimde-dev libluajit-5.1-dev python3-dev \
  libx11-dev libxcb-randr0-dev libxcb-shm0-dev libxcb-xinerama0-dev \
  libxcb-composite0-dev libxinerama-dev libxcb1-dev libxcb-xfixes0-dev \
  swig libcmocka-dev libxss-dev libglvnd-dev \
  libxkbcommon-dev libatk1.0-dev libatk-bridge2.0-dev libxcomposite-dev libxdamage-dev \
  libasound2-dev libfdk-aac-dev libfontconfig-dev libfreetype6-dev libjack-jackd2-dev \
  libsndio-dev libspeexdsp-dev libudev-dev libv4l-dev libva-dev libvlc-dev \
  libpci-dev libdrm-dev \
  nlohmann-json3-dev libwebsocketpp-dev libasio-dev libqrcodegencpp-dev \
  libffmpeg-nvenc-dev librist-dev libsrt-openssl-dev \
  qt6-base-dev libqt6svg6-dev qt6-base-private-dev \
  libvpl-dev libvpl2 \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /src
