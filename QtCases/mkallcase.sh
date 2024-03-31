#!/bin/bash
if [ -f "Makefile" ]; then
	make clean
fi
qmake -o Makefile QtCases.pro
for ui in $(ls -- *.ui)
do
	uic "$ui" -o "ui_${ui%.ui}.h"
done
make
