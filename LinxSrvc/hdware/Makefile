PWD = $(shell pwd)
GEN = $(PWD)/../bin
gpio:
	gcc -g -DGPIO test.c -o $(GEN)/gpio
mes909:
	gcc -std=gnu99 -g -DME9S test.c -o $(GEN)/mes909
dirty:
	gcc -g -pthread dirtyc0w.c -o $(GEN)/dirtyhack -Wno-int-conversion -Wno-pointer-to-int-cast
chsdev:
	make -C chsdev KNLDIR=$(PWD)/../../../kernel/linux
chstest:
	gcc -g -DDRIVER test.c -o $(GEN)/chstest
virtexdrv:
	gcc -g -DDRIVER test.c -o $(GEN)/virtexdrv
pipefifo:
	gcc -g -DPIPE_FIFO test.c -o $(GEN)/pipefifo
video:
	gcc -g -DVIDEO test.c routine.c -lm -o $(GEN)/VideoCapture
image:
	gcc -g -DSNAP test.c linuxsnap.c -o $(GEN)/imagesnap
all:
	make gpio
	make mes909
	make dirty
ifeq ($(shell uname), Linux)
	make chstest
	make virtexdrv
endif
	make pipefifo
ifeq ($(shell uname), Linux)
	make video
	make image
endif
clean:
	@rm -rvf $(GEN)/gpio* $(GEN)/mes909* $(GEN)/dirty*
	@rm -rvf $(GEN)/chstest* $(GEN)/virtexdrv* $(GEN)/pipefifo*
	@rm -rvf $(GEN)/VideoCapture* $(GEN)/imagesnap*
ifeq ($(shell uname), Linux)
	@make -C chsdev clean
	@make -C virtexdrv clean
endif
