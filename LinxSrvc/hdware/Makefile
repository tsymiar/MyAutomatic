PWD = $(shell pwd)
GEN = $(PWD)/../bin
OS = $(shell uname -s)
#BUILD = ${PWD}/../../../kernel/linux
BUILD = /lib/modules/$(shell uname -r)/build
chigpio:
	@gcc -g -DGPIO test.c -o $(GEN)/chigpio
	@echo "-- Built: \033[1m$@\033[0m"
mes909:
	@gcc -std=gnu99 -g -DME9S test.c -o $(GEN)/mes909 -Wno-implicit-function-declaration
	@echo "-- Built: \033[1m$@\033[0m"
dirty:
	@gcc -g -pthread dirtyc0w.c -o $(GEN)/dirtyhack -Wno-int-conversion -Wno-pointer-to-int-cast
	@echo "-- Built: \033[1m$@\033[0m"
chsdev:
	@if [ -d $(BUILD) ]; then make -C chsdev KNLDIR=$(BUILD); fi;
	@echo "-- Built: \033[1m$@\033[0m"
virtex:
	@if [ -d $(BUILD) ]; then make -C virtex KNLDIR=$(BUILD); fi;
	@echo "-- Built: \033[1m$@\033[0m"
chstest:
	@gcc -g -DDRIVER test.c -o $(GEN)/chstest
	@echo "-- Built: \033[1m$@\033[0m"
image:
	@gcc -g -DSNAP test.c linuxsnap.c -o $(GEN)/imagesnap.exe
	@echo "-- Built: \033[1m$@\033[0m"
video:
	@gcc -g -DVIDEO test.c routine.c -lm -o $(GEN)/VideoCapture
	@echo "-- Built: \033[1m$@\033[0m"
pipefifo:
	@gcc -g -DPIPE_FIFO test.c -o $(GEN)/pipefifo
	@echo "-- Built: \033[1m$@\033[0m"
all:
	@make chigpio
	@make mes909
	@make dirty
ifneq ($(OS), )
	@make chsdev
	@make virtex
	@make chstest
ifeq ($(OS), Linux)
	@make image
	@make video
endif
endif
	@make pipefifo
clean:
	@rm -rvf $(GEN)/chigpio* $(GEN)/mes909* $(GEN)/dirtyhack*
	@rm -rvf $(GEN)/chstest* $(GEN)/VideoCapture*
	@rm -rvf $(GEN)/imagesnap* $(GEN)/pipefifo*
ifeq ($(shell uname), Linux)
	@make -C chsdev clean
	@make -C virtex clean
endif
