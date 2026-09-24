#!/usr/bin/env bash
# Shared configuration for the dkqm (Docker + QEMU) build scripts.
# Source it, do not execute it:
#     . "$(dirname "${BASH_SOURCE[0]}")/env.sh"
#
# Precedence (highest first):
#   1. environment variables you export before running the scripts
#        APP_SRC=/path/to/other/checkout bash build-apk.sh
#      or persistently:
#        export APP_SRC=/path/to/checkout      # in ~/.bashrc
#   2. $DKQM_HOME/env.local  (ignored by git, per-user overrides)
#   3. the defaults below
#
# Available variables:
#   DKQM_HOME        scripts + logs + out/ root          (default: this dir)
#   APP_SRC         Android project source dir          (default: the repo)
#   DKQM_TOOLS       host tool root holding sdk/gradle   (default: /home/jetson/build-tools)
#   DKQM_SDK         Android SDK root                    (default: $DKQM_TOOLS/sdk_root)
#   DKQM_NDK_VER     NDK version inside the SDK          (default: 23.0.7599858)
#   DKQM_CMAKE_DIR   cmake.dir written to local.properties
#   DKQM_GRADLE_VER  gradle distribution version         (default: 7.0.2)
#   DKQM_OUT         APK output dir                      (default: $DKQM_HOME/out)
#   DKQM_LOG         build log file                      (default: $DKQM_HOME/build-apk.log)
#   DKQM_IMAGE       build image                         (default: dkqm-android:22.04-amd64)
#   DKQM_CACHE       gradle cache volume name            (default: dkqm-gradle-cache)
#   DKQM_RUN         build container name                (default: dkqm-build-run)
#   DKQM_BOX         image-bootstrap container name      (default: dkqm-box)

DKQM_HOME="${DKQM_HOME:-$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)}"

# Snapshot the DKQM_* variables the caller exported, so that env.local cannot
# silently override them (an explicit env var always wins).
_dkqm_preset=$(env | grep -E '^(DKQM_|APP_)' || true)

# per-user overrides, optional
if [ -f "$DKQM_HOME/env.local" ]; then
  # shellcheck disable=SC1091
  . "$DKQM_HOME/env.local"
fi

# restore the caller's values on top of whatever env.local did
if [ -n "$_dkqm_preset" ]; then
  while IFS='=' read -r _k _v; do
    [ -n "$_k" ] && export "$_k=$_v"
  done <<< "$_dkqm_preset"
  unset _k _v
fi
unset _dkqm_preset

: "${APP_SRC:=/home/jetson/shenyrion/git/Device2Device}"
: "${DKQM_TOOLS:=/home/jetson/build-tools}"
: "${DKQM_SDK:=$DKQM_TOOLS/sdk_root}"
: "${DKQM_NDK_VER:=23.0.7599858}"
: "${DKQM_CMAKE_DIR:=$DKQM_TOOLS/cmake-3.10.2/cmake}"
: "${DKQM_GRADLE_VER:=7.0.2}"
: "${DKQM_OUT:=$DKQM_HOME/out}"
: "${DKQM_LOG:=$DKQM_HOME/build-apk.log}"
: "${DKQM_IMAGE:=dkqm-android:22.04-amd64}"
: "${DKQM_CACHE:=dkqm-gradle-cache}"
: "${DKQM_RUN:=dkqm-build-run}"
: "${DKQM_BOX:=dkqm-box}"

# derived
DKQM_GRADLE_DIST="${DKQM_GRADLE_DIST:-$DKQM_TOOLS/gradle-$DKQM_GRADLE_VER}"
DKQM_LP_HOST="${DKQM_LP_HOST:-$DKQM_HOME/local.properties.host}"

# lines written to the project's local.properties on the host side
dkqm_host_local_properties() {
  printf '%s\n' \
    "cmake.dir=$DKQM_CMAKE_DIR" \
    "ndk.dir=$DKQM_SDK/ndk/$DKQM_NDK_VER" \
    "sdk.dir=$DKQM_SDK"
}
