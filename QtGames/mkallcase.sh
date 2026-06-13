#!/bin/bash
set -e

find_qmake() {
    for q in qmake qmake6 qmake-qt5 qmake-qt6; do
        if command -v "$q" &>/dev/null; then
            echo "$q"
            return 0
        fi
    done
    return 1
}

install_deps() {
    sudo apt install -y \
        cmake build-essential \
        qtbase5-dev libqt5opengl5-dev \
        libpng-dev freeglut3-dev \
        libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev
}

build_qmake() {
    local qmake_bin="$1"
    echo ">>> Using qmake: ${qmake_bin}"

    mkdir -p "build"
    cd "build"
    "${qmake_bin}" ../QtGames.pro
    make -j$(nproc)
    echo "Build complete (qmake): build/QtGames"
}

build_cmake() {
    echo ">>> qmake not found, falling back to CMake"

    mkdir -p "build"
    cd "build"
    cmake .. -DCMAKE_BUILD_TYPE=Debug
    make -j$(nproc)
    echo "Build complete (cmake): build/QtGames"
}

case "${1}" in
    install)
        install_deps
        ;;
    clean)
        rm -rf "build"
        echo "Cleaned build/"
        ;;
    *)
        QMAKE_BIN=$(find_qmake)
        if [ -n "${QMAKE_BIN}" ]; then
            build_qmake "${QMAKE_BIN}"
        else
            build_cmake
        fi
        ;;
esac
