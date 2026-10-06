#!/bin/bash
set -e

# ── Build FileTransfer.app from Swift Package Manager project ──

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
APP_NAME="FileTransferMac"
APP_BUNDLE="${APP_NAME}.app"
BUILD_DIR="${SCRIPT_DIR}/.build"

echo "==> Building ${APP_NAME} (release) ..."
cd "${SCRIPT_DIR}"
swift build -c release

# Locate the release binary
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
