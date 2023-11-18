#!/bin/bash
if [ "$1" == "clean" ]; then
    rm -f `pwd`/bin/gn
else
    g++ gn/main.cxx -o `pwd`/bin/gn -std=c++11 -lpthread
fi
