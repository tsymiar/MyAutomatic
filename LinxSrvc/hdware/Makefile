GEN = `pwd`/../bin
gpio:
	gcc -g -DGPIO test.c -o $(GEN)/gpio
mes909:
	gcc -std=gnu99 -g -DME9S test.c -o $(GEN)/mes909
hack:
	gcc -g dirtyc0w.c -o $(GEN)/hack -Wno-int-conversion -Wno-void-pointer-to-int-cast
pipefifo:
	gcc -g -DPIPE_FIFO test.c -o $(GEN)/pipefifo
video:
	gcc -g -DVIDEO test.c routine.c -lm -o $(GEN)/VideoCapture
snap:
	gcc -g -DSNAP test.c linuxsnap.c -o $(GEN)/snap
all:
	make gpio
	make mes909
	make hack
	make pipefifo
ifeq ($(shell uname), Linux)
	make video
	make snap
endif
clean:
	rm -rvf $(GEN)/gpio* $(GEN)/me909* $(GEN)/hack* $(GEN)/pipefifo*
	rm -rvf  $(GEN)/VideoCapture* $(GEN)/snap*
