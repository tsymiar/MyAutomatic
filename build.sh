#!/bin/bash
PWD=$(pwd)
cd "${PWD}/LinxSrvc" && make "$1" && cd -
if [ -d "build" ] && [ "$1" == "clean" ]; then
    rm -rvf build;
fi;
