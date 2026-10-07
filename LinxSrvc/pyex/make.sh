#!/bin/bash
set -e

if [ "${1}" == "clean" ]; then
    rm -rvf dist build
    rm -rvf ./*.egg-info
    exit 0
fi

PY="${PYTHON:-$(command -v python3 || command -v python)}"
if [ -z "${PY}" ]; then
    echo "no python interpreter found (tried python3 / python)" >&2
    exit 1
fi

# PEP 668: distros mark the system interpreter as externally managed,
# so --break-system-packages has to be passed explicitly.
PIP_FLAGS=()
if "${PY}" -c 'import os,sys,sysconfig; sys.exit(0 if os.path.exists(os.path.join(sysconfig.get_path("stdlib"), "EXTERNALLY-MANAGED")) else 1)'; then
    PIP_FLAGS+=(--break-system-packages)
fi

pip_install() {
    # Old pip does not know --break-system-packages: retry without the flag on failure
    if [ ${#PIP_FLAGS[@]} -gt 0 ]; then
        "${PY}" -m pip install --upgrade "${PIP_FLAGS[@]}" "$@" && return 0
        echo "pip rejected ${PIP_FLAGS[*]}, retrying without it" >&2
    fi
    "${PY}" -m pip install --upgrade "$@"
}

pip_install pip build
"${PY}" -m build
