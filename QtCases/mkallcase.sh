#!/bin/bash
if [ -f "Makefile" ]; then
	make clean
fi
sudo apt install libpng++-dev freeglut3-dev libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev -y
qmake -o Makefile QtCases.pro
for ui in $(ls -- *.ui)
do
	uic "$ui" -o "ui_${ui%.ui}.h"
done
make
