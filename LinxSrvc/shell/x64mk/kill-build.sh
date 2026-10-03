#!/usr/bin/env bash
# Stop everything belonging to the emulated amd64 build:
#   - build containers        : $DKQM_RUN, $DKQM_BOX, and any leftover
#                               container started from $DKQM_IMAGE
#   - host-side processes     : docker CLI / wrapper shells launched by
#                               build-apk.sh or build-image.sh
#   - lingering qemu-x86_64   : only with --qemu (they may belong to other jobs)
#
# It also restores the host local.properties. build-apk.sh swaps it to
# container-side paths and restores it only on normal exit, so killing the build
# mid-way would otherwise leave the repo pointing at /opt/android-sdk.
#
# Paths come from env.sh / environment variables, e.g.
#   APP_SRC=/path/to/checkout bash kill-build.sh
#
# Usage:
#   bash /home/jetson/shenyrion/software/dkqm-build/kill-build.sh [--qemu]

DKQM_THIS_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=env.sh
. "$DKQM_THIS_DIR/env.sh"

SRC=$APP_SRC
IMAGE=$DKQM_IMAGE
KILL_QEMU=0
[ "${1:-}" = "--qemu" ] && KILL_QEMU=1

echo "== stop requested by $(whoami) at $(date) =="
echo "  APP_SRC   = $SRC"
echo "  DKQM_IMAGE = $IMAGE"

# ---------------------------------------------------------------- containers
for c in "$DKQM_RUN" "$DKQM_BOX"; do
  if docker ps -a --format '{{.Names}}' 2>/dev/null | grep -qx "$c"; then
    echo "-- stopping container $c"
    docker stop -t 5 "$c" >/dev/null 2>&1 || true
    docker rm -f "$c" >/dev/null 2>&1 || true
  fi
done

# any other container started from our image
leftover=$(docker ps -a --filter "ancestor=$IMAGE" --format '{{.Names}}' 2>/dev/null)
for c in $leftover; do
  echo "-- removing leftover container $c"
  docker stop -t 5 "$c" >/dev/null 2>&1 || true
  docker rm -f "$c" >/dev/null 2>&1 || true
done

# ----------------------------------------------------------- host processes
for pat in 'build-apk\.sh' 'build-image\.sh' "docker run.*--platform linux/amd64.*${IMAGE}"; do
  hits=$(pgrep -f "$pat" | tr '\n' ' ' 2>/dev/null)
  [ -z "$hits" ] && continue
  echo "-- killing host processes matching /$pat/ : $hits"
  pkill -f "$pat" 2>/dev/null || true
done

# ---------------------------------------------------------------- qemu
qemu_count=$(pgrep -f 'qemu-x86_64' 2>/dev/null | wc -l | tr -dc '0-9')
[ -z "$qemu_count" ] && qemu_count=0
if [ "$qemu_count" -gt 0 ]; then
  if [ "$KILL_QEMU" -eq 1 ]; then
    echo "-- killing $qemu_count lingering qemu-x86_64 process(es)"
    pkill -f 'qemu-x86_64' 2>/dev/null || true
  else
    echo "!! $qemu_count qemu-x86_64 process(es) still alive; they may belong to"
    echo "   other workloads. Re-run with --qemu to force kill them."
  fi
fi

# ------------------------------------------------- restore local.properties
# Deterministic restore: never trust the backed-up copy blindly, a run that was
# interrupted before restoring could have left container-side (/opt/android-sdk)
# paths in both the repo file and the backup.
if [ -d "$SRC" ]; then
  echo "-- restoring host local.properties"
  dkqm_host_local_properties > "$SRC/local.properties"
  cat "$SRC/local.properties"
else
  echo "!! APP_SRC not found, local.properties not touched: $SRC"
fi

# ------------------------------------------------------------------ report
remaining=$(docker ps -a --filter "ancestor=$IMAGE" --format '{{.Names}} [{{.Status}}]' 2>/dev/null)
echo "-- remaining containers from $IMAGE: ${remaining:-none}"
echo "-- done =="
