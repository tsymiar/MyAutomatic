#!/bin/bash
g++ -c ../MFCKline/mygl/SDL_text.c -lGL -lSDL
qmake -o Makefile QtKline.pro
for ui in `ls *.ui`
do
	uic $ui -o ui_${ui%.ui}.h
done
make 
