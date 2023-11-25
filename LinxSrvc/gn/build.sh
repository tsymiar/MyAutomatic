#!/bin/bash
BIN=$(pwd)/../bin
if [ "${1}" == "clean" ]; then
    rm -f "${BIN}"/gn
else
    g++ main.cxx -o "${BIN}"/gn -std=c++11 -lpthread
fi
