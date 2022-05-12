GEN = `pwd`/../bin
gpio:
	gcc -g -DGPIO test.c -o $(GEN)/gpio
me909:
	gcc -std=gnu99 -g -DME9S test.c -o $(GEN)/me909
tmpfifo:
	gcc -g -DFIFO test.c -o $(GEN)/tmpfifo
snap:
	gcc -g -DSNAP test.c linuxsnap.c -o $(GEN)/snap
video:
	gcc -g -DVIDEO test.c routine.c -lm -o $(GEN)/VideoCapture
all:
	make gpio
	make me909
	make tmpfifo
ifeq ($(shell uname), Linux)
	make snap
	make video
endif
clean:
	rm -rvf $(GEN)/gpio* $(GEN)/me909* $(GEN)/tmpfifo*
	rm -rvf  $(GEN)/VideoCapture* $(GEN)/snap*
