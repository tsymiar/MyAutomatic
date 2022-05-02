GEN = `pwd`/../bin
gpio:
	gcc -g -DGPIO test.c -o $(GEN)/gpio.exe
me909:
	gcc -std=gnu99 -g -DME9S test.c -o $(GEN)/me909.exe
fifo:
	gcc -g -DFIFO test.c -o $(GEN)/fifo.exe
v4l2:
	gcc -g -DV4L2 test.c linuxvideo.c -o $(GEN)/v4l2.exe
routine:
	gcc -g -DCAPTURE test.c routine.c -lm -o $(GEN)/routine.exe
all:
	make gpio
	make me909
	make fifo
ifeq ($(shell uname), Linux)
	make v4l2
	make routine
endif
clean:
	rm -vf $(GEN)/gpio.exe $(GEN)/me909.exe $(GEN)/fifo.exe
	rm -vf $(GEN)/v4l2.exe $(GEN)/routine.exe
