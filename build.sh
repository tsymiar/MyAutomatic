#!/bin/bash
echo "-------- Begin 'LinxSrvc' building ... --------"
PWD=$(pwd)
if [ "$1" == "test" ]
then
    cd "${PWD}/LinxSrvc/test"
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
else
    cd "${PWD}/LinxSrvc";
    if [ "$1" != "clean" ]
    then
        if [ ! -d bin ]; then mkdir bin; fi;
        if [ ! -d gen ]; then mkdir gen; fi;
        if [ ! -d out ]; then mkdir out; fi;
    fi
    make "$@"
    if [ "$1" == "clean" ]
    then
        if [ -d "build" ]; then rm -rvf lib build; fi;
        if [ -d "test/build" ]; then cd test && ./test.sh clean; fi;
        if [ -d "../build" ]; then rm -rvf ../lib ../build; fi;
    fi
fi
echo "-------- All '$1' build progress(es) finish --------"
exit 0;
