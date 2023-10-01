#!/bin/bash
PWD=$(pwd)
if [ "$1" == "test" ]
then
    cd LinxSrvc/test
    if [ ! $(whereis lcov | awk '{print $2}') ]
    then
        if [ $(ls lcov/* | wc -l) -le 0 ]
        then
            git submodule update --init --recursive
            git pull
        fi
        cd lcov && make install
        cd - && rm -rvf lcov
    fi
    if [ $(ls googletest/* | wc -l) -le 0 ]
    then
        git clone https://github.com/google/googletest.git
        git pull
    fi
    ./test.sh clean
    ./test.sh
    cd "${PWD}"
    exit 0;
else
    cd "${PWD}/LinxSrvc";
    if [ ! -d bin ]; then mkdir bin; fi;
    if [ ! -d out ]; then mkdir out; fi;
    make "$@" && cd -;
fi
if [ "$1" == "clean" ]
then
    if [ -d "build" ]; then rm -rvf lib build; fi;
    if [ -d "${PWD}/LinxSrvc/test/build" ];
    then
        cd "${PWD}/LinxSrvc/test"
        ./test.sh clean
        cd -
    fi
fi
