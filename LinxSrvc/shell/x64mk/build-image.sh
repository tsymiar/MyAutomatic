#!/usr/bin/env bash
# Build an amd64 Android build-box image for the Jetson (aarch64) host.
# Uses plain docker run + docker commit (docker buildx is unusable here:
# ~/.docker is not writable by the jetson user).
#
# Image / container names come from env.sh, override with:
#   DKQM_IMAGE=my-builder:1.0 bash build-image.sh
#
# Host prerequisites: x86_64 binfmt registered via
#   docker run --rm --privileged tonistiigi/binfmt --install amd64

set -x

DKQM_THIS_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=env.sh
. "$DKQM_THIS_DIR/env.sh"

LOG=$DKQM_HOME/build.log
mkdir -p "$DKQM_HOME/home"
export HOME=$DKQM_HOME/home

{
  echo "== start $(date) =="
  echo "  DKQM_HOME  = $DKQM_HOME"
  echo "  DKQM_BOX   = $DKQM_BOX"
  echo "  DKQM_IMAGE = $DKQM_IMAGE"

  docker rm -f "$DKQM_BOX" >/dev/null 2>&1

  docker run -d --name "$DKQM_BOX" --platform linux/amd64 ubuntu:22.04 sleep infinity

  docker exec "$DKQM_BOX" bash -c 'apt-get update -qq \
      && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
           ca-certificates curl wget unzip zip git pkg-config \
           openjdk-11-jdk-headless cmake ninja-build make \
      && rm -rf /var/lib/apt/lists/*'

  docker commit \
      -c 'ENV JAVA_HOME=/usr/lib/jvm/java-11-openjdk-amd64' \
      -c "ENV PATH=/usr/lib/jvm/java-11-openjdk-amd64/bin:/opt/gradle-$DKQM_GRADLE_VER/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin" \
      -c 'ENV ANDROID_HOME=/opt/android-sdk' \
      -c 'ENV ANDROID_SDK_ROOT=/opt/android-sdk' \
      -c "ENV ANDROID_NDK_HOME=/opt/android-sdk/ndk/$DKQM_NDK_VER" \
      -c 'ENV LANG=C.UTF-8' \
      -c 'ENV GRADLE_OPTS=-Dorg.gradle.jvmargs=-Xmx1500m -XX:MaxMetaspaceSize=512m -Dfile.encoding=UTF-8' \
      -c 'WORKDIR /src' \
      "$DKQM_BOX" "$DKQM_IMAGE"

  echo "== done $(date) =="
} > "$LOG" 2>&1
