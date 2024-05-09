PWD = $(shell pwd)
GEN = $(PWD)/../bin
#BUILD = ${PWD}/../../../kernel/linux
BUILD = /lib/modules/$(shell uname -r)/build
chigpio:
	gcc -g -DGPIO test.c -o $(GEN)/chigpio
mes909:
	gcc -std=gnu99 -g -DME9S test.c -o $(GEN)/mes909
dirty:
	gcc -g -pthread dirtyc0w.c -o $(GEN)/dirtyhack -Wno-int-conversion -Wno-pointer-to-int-cast
chsdev:
	@if [ -d $(BUILD) ]; then make -C chsdev KNLDIR=$(BUILD); fi;
virtex:
	@if [ -d $(BUILD) ]; then make -C virtex KNLDIR=$(BUILD); fi;
chstest:
	gcc -g -DDRIVER test.c -o $(GEN)/chstest
video:
	gcc -g -DVIDEO test.c routine.c -lm -o $(GEN)/VideoCapture
image:
	gcc -g -DSNAP test.c linuxsnap.c -o $(GEN)/imagesnap.exe
pipefifo:
	gcc -g -DPIPE_FIFO test.c -o $(GEN)/pipefifo
all:
	make chigpio
	make mes909
	make dirty
ifeq ($(shell uname), Linux)
	make chstest
	make video
	make image
	make chsdev
	make virtex
endif
	make pipefifo
clean:
	@rm -rvf $(GEN)/chigpio* $(GEN)/mes909* $(GEN)/dirtyhack*
	@rm -rvf $(GEN)/chstest* $(GEN)/VideoCapture*
	@rm -rvf $(GEN)/imagesnap* $(GEN)/pipefifo*
ifeq ($(shell uname), Linux)
	@make -C chsdev clean
	@make -C virtex clean
endif
