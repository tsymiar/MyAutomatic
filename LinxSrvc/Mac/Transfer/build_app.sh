#!/bin/bash
set -e

# ── Build script for the Transfer project ────────────────────────────
#   ./build_app.sh          # auto: macOS -> FileTransfer.app, Linux -> transfer
#   ./build_app.sh app      # force macOS app build (needs macOS + Swift)
#   ./build_app.sh test     # force transfer build (macOS/Linux, C++ core only)
#   ./build_app.sh clean    # remove build outputs (.build, FileTransferMac.app)

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CORE_DIR="${SCRIPT_DIR}/Sources/FileTransferCore"
TEST_SRC="${SCRIPT_DIR}/test/main.cpp"
TEST_OUT_DIR="${SCRIPT_DIR}/.build/test"

# ── macOS app: FileTransfer.app from Swift Package Manager ──────────
build_app() {
    if [ "$(uname -s)" != "Darwin" ]; then
        echo "ERROR: 'app' target requires macOS (Swift + AppKit)"
        exit 1
    fi

    local APP_NAME="FileTransferMac"
    local APP_BUNDLE="${APP_NAME}.app"
    local BUILD_DIR="${SCRIPT_DIR}/.build"

    echo "==> Building ${APP_NAME} ..."
    cd "${SCRIPT_DIR}"
    swift build -c release

    # Locate the release binary
    local BINARY
    BINARY=$(find "${BUILD_DIR}" -name "${APP_NAME}" -path "*/release/*" -type f ! -path "*.dSYM*" | head -1)
    if [ -z "${BINARY}" ]; then
        echo "ERROR: release binary not found"
        exit 1
    fi
    echo "    Binary: ${BINARY}"

    # Create .app bundle structure
    echo "==> Creating ${APP_BUNDLE} ..."
    rm -rf "${SCRIPT_DIR}/${APP_BUNDLE}"
    mkdir -p "${SCRIPT_DIR}/${APP_BUNDLE}/Contents/MacOS"
    mkdir -p "${SCRIPT_DIR}/${APP_BUNDLE}/Contents/Resources"

    cp "${BINARY}" "${SCRIPT_DIR}/${APP_BUNDLE}/Contents/MacOS/${APP_NAME}"

    # Generate Info.plist
    cat > "${SCRIPT_DIR}/${APP_BUNDLE}/Contents/Info.plist" << 'PLIST'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleDevelopmentRegion</key>
    <string>en</string>
    <key>CFBundleExecutable</key>
    <string>FileTransferMac</string>
    <key>CFBundleIdentifier</key>
    <string>com.myautomatic.filetransfer</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
    <key>CFBundleName</key>
    <string>FileTransfer</string>
    <key>CFBundleDisplayName</key>
    <string>FileTransfer</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0</string>
    <key>CFBundleVersion</key>
    <string>1</string>
    <key>LSMinimumSystemVersion</key>
    <string>13.0</string>
    <key>NSHighResolutionCapable</key>
    <true/>
    <key>NSLocalNetworkUsageDescription</key>
    <string>FileTransfer needs access to the local network to send and receive files with other devices on your LAN.</string>
</dict>
</plist>
PLIST

    # ── Ad-hoc sign ──────────────────────────────────────────────
    # Gives the bundle a stable code identity so macOS can remember the
    # Local Network permission grant. Without it, LAN transfers fail with
    # EHOSTUNREACH ("No route to host") while 127.0.0.1 keeps working.
    if command -v codesign >/dev/null 2>&1; then
        echo "==> Ad-hoc signing ${APP_BUNDLE} ..."
        codesign --force --deep --sign - "${SCRIPT_DIR}/${APP_BUNDLE}" \
            || echo "    WARNING: codesign failed, continuing unsigned"
    else
        echo "    WARNING: codesign not found, skipping signature"
    fi

    echo "==> Done: ${SCRIPT_DIR}/${APP_BUNDLE}"
    du -sh "${SCRIPT_DIR}/${APP_BUNDLE}"
}

# ── transfer: command-line server, reuses the C++ core ───────
build_test() {
    echo "==> Building transfer ..."
    mkdir -p "${TEST_OUT_DIR}"

    local CXX_BIN="${CXX:-c++}"
    local LD=()
    # Linux needs -lpthread; the engine is POSIX C++ and compiles on both
    if [ "$(uname -s)" != "Darwin" ]; then
        LD+=(-lpthread)
    fi

    "$CXX_BIN" -std=c++17 -O2 -Wall \
        -I "${CORE_DIR}" -I "${CORE_DIR}/include" \
        -DLOG_TAG='"TransferTest"' \
        "${TEST_SRC}" \
        "${CORE_DIR}/TransferEngine.cpp" \
        "${CORE_DIR}/TransferBridge.cpp" \
        -o "${TEST_OUT_DIR}/transfer" \
        "${LD[@]+"${LD[@]}"}"

    echo "==> Done: ${TEST_OUT_DIR}/transfer"
}

clean_all() {
    echo "==> Cleaning Transfer build outputs ..."
    rm -rf "${SCRIPT_DIR}/.build" "${SCRIPT_DIR}/FileTransferMac.app"
}

TARGET="${1:-auto}"
case "${TARGET}" in
    app)    build_app ;;
    test)   build_test ;;
    clean)  clean_all ;;
    auto)
        if [ "$(uname -s)" = "Darwin" ]; then
            build_app
        else
            build_test
        fi
        ;;
    *)
        echo "Usage: $0 [app|test|clean]"
        exit 1
        ;;
esac
