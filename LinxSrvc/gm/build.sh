#!/bin/bash
if [ "$1" == "clean" ]; then
    rm -f `pwd`/bin/gm
else
    g++ gm/main.cxx -o `pwd`/bin/gm -std=c++11 -lpthread
fi
