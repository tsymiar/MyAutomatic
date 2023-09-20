PWD = $(shell pwd)
GEN = $(PWD)/../bin
gpio:
	gcc -g -DGPIO test.c -o $(GEN)/gpio
mes909:
	gcc -std=gnu99 -g -DME9S test.c -o $(GEN)/mes909
dirty:
	gcc -g -pthread dirtyc0w.c -o $(GEN)/dirtyhack -Wno-int-conversion -Wno-pointer-to-int-cast
chsdev:
	@if [ -d $(PWD)/../../../kernel/linux ]; then make -C chsdev KNLDIR=$(PWD)/../../../kernel/linux; fi;
virtexdrv:
	@if [ -d $(PWD)/../../../kernel/linux ]; then make -C virtexdrv KNLDIR=$(PWD)/../../../kernel/linux; fi;
chstest:
	gcc -g -DDRIVER test.c -o $(GEN)/chstest
video:
	gcc -g -DVIDEO test.c routine.c -lm -o $(GEN)/VideoCapture
image:
	gcc -g -DSNAP test.c linuxsnap.c -o $(GEN)/imagesnap
pipefifo:
	gcc -g -DPIPE_FIFO test.c -o $(GEN)/pipefifo
all:
	make gpio
	make mes909
	make dirty
ifeq ($(shell uname), Linux)
	make chstest
	make virtexdrv
	make video
	make image
endif
	make pipefifo
clean:
	@rm -rvf $(GEN)/gpio* $(GEN)/mes909* $(GEN)/dirty*
	@rm -rvf $(GEN)/chstest* $(GEN)/virtexdrv*
	@rm -rvf $(GEN)/VideoCapture* $(GEN)/imagesnap* $(GEN)/pipefifo*
ifeq ($(shell uname), Linux)
	@make -C chsdev clean
	@make -C virtexdrv clean
endif
