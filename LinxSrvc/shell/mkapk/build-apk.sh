#!/usr/bin/env bash
# Build Device2Device APK inside an emulated amd64 container (plan B).
#
# Every path is configurable through environment variables (see env.sh).
# The one you usually want:
#     APP_SRC=/path/to/checkout bash build-apk.sh
#
# Host prerequisites:
#   1) x86_64 binfmt registered:
#        docker run --rm --privileged tonistiigi/binfmt --install amd64
#   2) image built: bash /home/jetson/shenyrion/software/dkqm-build/build-image.sh
#
# Mounts:
#   gradle dist  : pure java -> reused from the host, no download
#   sdk_root     : x86_64 binaries inside, executed through qemu
#   project      : mounted rw so app/build stays incremental across runs
#                  (local.properties is saved before / restored after)

set -x

DKQM_THIS_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=env.sh
. "$DKQM_THIS_DIR/env.sh"

SRC=$APP_SRC
OUT=$DKQM_OUT
LOG=$DKQM_LOG
LP_HOST=$DKQM_LP_HOST
GRADLE_VER=$DKQM_GRADLE_VER

if [ ! -d "$SRC" ]; then
  echo "!! APP_SRC does not exist: $SRC" >&2
  echo "   Fix it with: APP_SRC=/real/path bash $(basename "$0")" >&2
  exit 1
fi
mkdir -p "$OUT"

# Keep the host copy safe; the container needs container-side paths.
# Do not overwrite a good backup with a poisoned file: if the previous run was
# killed before restoring, local.properties still holds /opt/android-sdk paths.
if [ ! -f "$LP_HOST" ] || ! grep -q '/opt/android-sdk' "$SRC/local.properties"; then
  cp -f "$SRC/local.properties" "$LP_HOST" 2>/dev/null || true
fi

# Show everything live on the terminal *and* keep a full copy in the log file.
# (The build used to be wrapped in `{ ... } > "$LOG" 2>&1`, which buffered all
#  output until the very end - nothing was visible while it ran.)
exec > >(tee "$LOG") 2>&1

echo "== config =="
echo "  APP_SRC       = $SRC"
echo "  DKQM_OUT       = $OUT"
echo "  DKQM_LOG       = $LOG"
echo "  DKQM_SDK       = $DKQM_SDK (ndk $DKQM_NDK_VER)"
echo "  DKQM_CMAKE_DIR = $DKQM_CMAKE_DIR"
echo "  DKQM_GRADLE    = $DKQM_GRADLE_DIST"
echo "  DKQM_IMAGE     = $DKQM_IMAGE"
echo "  DKQM_CACHE     = $DKQM_CACHE"

echo "== start $(date) =="
docker run --rm --platform linux/amd64 \
    --name "$DKQM_RUN" \
    -v "$SRC":/src \
    -v "$DKQM_GRADLE_DIST":/opt/gradle-$GRADLE_VER:ro \
    -v "$DKQM_SDK":/opt/android-sdk \
    -v "$DKQM_CACHE":/root/.gradle \
    -v "$OUT":/out \
    -e ANDROID_HOME=/opt/android-sdk \
    -e ANDROID_SDK_ROOT=/opt/android-sdk \
    -e ANDROID_NDK_HOME=/opt/android-sdk/ndk/"$DKQM_NDK_VER" \
    -e GRADLE_BIN=/opt/gradle-$GRADLE_VER/bin \
    "$DKQM_IMAGE" \
    bash -c '
      set -e
      cd /src
      # externalNativeBuild caches the CMake configuration; drop it whenever
      # CMakeLists.txt files change, otherwise additions are not picked up.
      rm -rf /src/app/.cxx /src/app/build/intermediates/cmake /src/app/build/intermediates/cxx
      printf "ndk.dir=%s\nsdk.dir=%s\n" "$ANDROID_NDK_HOME" "$ANDROID_SDK_ROOT" > local.properties
      export JAVA_HOME=/usr/lib/jvm/java-11-openjdk-amd64
      export PATH=$JAVA_HOME/bin:$GRADLE_BIN:$PATH
      java -version
      cmake --version | head -1
      gradle assembleDebug --no-daemon --stacktrace --console=plain
      mkdir -p /out
      cp -f app/build/outputs/apk/debug/*.apk /out/ || true
    '
rc=$?
echo "== end $(date) rc=$rc =="

# restore the host local.properties (deterministically, not from the backup:
# the backup itself can be poisoned by an interrupted previous run)
dkqm_host_local_properties > "$SRC/local.properties"

if [ "$rc" -ne 0 ]; then
  echo "!! BUILD FAILED, full log: $LOG"
  exit "$rc"
fi
ls -la "$OUT"
