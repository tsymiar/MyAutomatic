#!/bin/bash
BIN=$(pwd)/../bin
if [ "${1}" == "clean" ]; then
    rm -f "${BIN}"/gn1
else
    g++ main.cxx -o "${BIN}"/gn1 -std=c++11 -lpthread
fi
