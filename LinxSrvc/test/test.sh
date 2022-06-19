#!/bin/bash
if [ "$1" == "clean" ]
then
    rm -rvf build && exit 0;
else if [ ! -d "build" ]
    then
        mkdir build;
    fi
fi
# build && genhtml
cd build
cmake ..
make -j
./test
lcov -d . -t unittest -o test.info -b . -c \
    --exclude '*/usr/include/*' \
    --exclude '/usr/lib/*' \
    --exclude '*/gtest*'
genhtml -o html test.info
