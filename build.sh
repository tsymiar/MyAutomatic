#!/bin/bash
echo "-------- Begin '$(uname)' building ... --------"
PWD=$(pwd)
if [ "$1" == "test" ]
then
    cd "${PWD}/LinxSrvc/test";
    rm -rvf build/*;
    if [ ! $(whereis lcov | awk '{print $2}') ]
    then
        if [ -d lcov ] && [ $(ls lcov/* | wc -l) -le 0 ]
        then
            git submodule update --init --recursive
            git pull
        else
            mkdir lcov
        fi
        cd lcov && make install
        cd - && rm -rvf lcov
    fi
    if [ $(ls googletest/* | wc -l) -le 0 ]
    then
        git clone https://github.com/google/googletest.git
        git pull
    fi
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
        shopt -s nullglob
        files=$(ls ../build* 2> /dev/null | wc -l)
        if [ "$files" != "0" ]; then
            rm -rvf ../lib
            find ../build* ! -name 'build.sh' -exec rm -rvf {} +
        fi
        cd -
        if [ -f "QtCases/Makefile" ]; then cd QtCases && make clean && cd -; fi;
    fi
fi
echo "-------- All '$1' build progress(es) finish --------"
exit 0;
