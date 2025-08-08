#!/bin/bash
set -e
if [ "${1}" == "clean" ]; then
    rm -rvf dist build
    rm -rvf *.egg-info
else
    python -m pip install --upgrade pip
    pip install build
    python -m build
fi
