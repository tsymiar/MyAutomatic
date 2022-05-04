GEN = `pwd`/../bin
gpio:
	gcc -g -DGPIO test.c -o $(GEN)/gpio.exe
me909:
	gcc -std=gnu99 -g -DME9S test.c -o $(GEN)/me909.exe
fifo:
	gcc -g -DFIFO test.c -o $(GEN)/fifo.exe
snap:
	gcc -g -DSNAP test.c linuxsnap.c -o $(GEN)/snap.exe
capture:
	gcc -g -DCAPTURE test.c routine.c -lm -o $(GEN)/capture.exe
all:
	make gpio
	make me909
	make fifo
ifeq ($(shell uname), Linux)
	make snap
	make capture
endif
clean:
	rm -vf $(GEN)/gpio.exe $(GEN)/me909.exe $(GEN)/fifo.exe
	rm -vf  $(GEN)/capture.exe $(GEN)/snap.exe
