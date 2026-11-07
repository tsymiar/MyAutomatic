#!/usr/bin/env bash
# Ensure qmake exists and points to Qt5, installing/pinning on demand if needed.
# Shared by azure-pipelines.yml and .github/workflows/cmake-multi-platform.yml.
set -e

if command -v qmake >/dev/null 2>&1 && qmake --version | grep -q 'Qt version 5'; then
    qmake --version
    exit 0
fi

export DEBIAN_FRONTEND=noninteractive
sudo apt-get update
sudo apt-get install -y --no-install-recommends \
    qtbase5-dev \
    qt5-qmake \
    qtbase5-dev-tools \
    libqt5opengl5-dev \
    qtmultimedia5-dev \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    freeglut3-dev \
    libpng-dev \
    libsdl2-dev \
    libsdl2-image-dev \
    libsdl2-ttf-dev
sudo ln -sf "$(dpkg -L qt5-qmake | grep -m1 '/qt5/bin/qmake$')" /usr/bin/qmake
qmake --version
