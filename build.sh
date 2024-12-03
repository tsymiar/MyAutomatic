#!/bin/bash
echo "-------- Begin '$(uname)' building ... --------"
PWD=$(pwd)
if [ "$1" == "test" ]
then
    cd "${PWD}/LinxSrvc/test";
    # rm -rvf build/*;
    if [ ! $(whereis lcov | awk '{print $2}') ]
    then
        if [ -d lcov ] && [ $(ls lcov/* | wc -l) -le 0 ]
        then
            git submodule update --init --recursive
            git pull
        else
            mkdir lcov
        fi
        cd lcov && make install && cd -
    fi
    if [ $(ls googletest/* | wc -l) -le 0 ]
    then
        git clone https://github.com/google/googletest.git
        git pull
    fi
    ./test.sh
else
    if [[ "$1" == "Qt"* ]]; then
        if [ -d "$1" ]; then
            cd "$1"
            if [ -x /usr/bin/qmake ]; then
                proj=$(echo "${1}" | sed 's/\/*$//')
                qmake "${proj}.pro"
                make all -j4
            else
                echo Not support "$1" build!
            fi
            cd -
        fi
        exit 0
    fi
    cd "${PWD}/LinxSrvc"
    if [ "$1" != "clean" ]
    then
        if [ ! -d bin ]; then mkdir bin; fi;
        if [ ! -d gen ]; then mkdir gen; fi;
        if [ ! -d out ]; then mkdir out; fi;
    fi
    make "$@"
    if [ "$1" == "clean" ]
    then
        shopt -s nullglob
        files=$(ls ../build* 2> /dev/null | wc -l)
        if [ "${#files[@]}" != "0" ]; then
            rm -rvf ../build
            # find ./build* ! -name 'build.sh' -exec rm -rvf {} +
        fi
        if [ -d "build" ]; then rm -rvf lib build; fi;
        ARR_WIN=($(ls -d ../*/ | awk '{gsub("\\.\\./", "", $1); print $1}' | tr '/\n' ' '))
        for i in "${ARR_WIN[@]}"; do
            cd "../$i"
            if [[ "$i" == "Qt"* ]]; then
                which qmake >/dev/null 2>&1
                if [ $? -eq 0 ]; then
                    if [ -f "./Makefile" ]; then
                        make clean
                    fi
                else
                    rm -rvf GeneratedFiles* *.stash *.user* *.qtvscr *.TMP
                fi
            fi
            ARR_SUB=(Debug */*Debug MFC cache build ./*.o .vs)
            for j in "${ARR_SUB[@]}";
            do
                rm -rvf "$j"
            done
            cd -
        done
        if [ -d "test/build" ]; then
            cd test; ./test.sh clean; cd -;
        fi
    fi
fi
echo "-------- All '$1' build progress(es) finish --------"
exit 0;
