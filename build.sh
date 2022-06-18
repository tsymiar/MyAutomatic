#!/bin/bash
PWD=$(pwd)
if [ "$1" == "test" ];
then
    cd LinxSrvc/test
    ./test.sh clean
    ./test.sh
    cd -
    exit 0;
fi;
cd "${PWD}/LinxSrvc" && make "$@" && cd -
if [ "$1" == "clean" ];
then
    if [ -d "build" ]; then rm -rvf lib build; fi;
    if [ -d "${PWD}/LinxSrvc/test/build" ];
    then
        cd ${PWD}/LinxSrvc/test
        ./test.sh clean
        cd -
    fi
fi;
