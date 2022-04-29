GEN = `pwd`/../bin
v4l:
	gcc -g -DV4L2 test.c -o $(GEN)/v4l2.exe
gpio:
	gcc -g -DGPIO test.c -o $(GEN)/gpio.exe
me909:
	gcc -std=gnu99 -g -DME9S test.c -o $(GEN)/me909.exe
routine:
	gcc -g routine.c -o $(GEN)/routine.exe
all:
ifeq ($(shell uname), Linux)
	make v4l
endif
	make gpio
	make me909
clean:
	rm -vf $(GEN)/v4l2.exe $(GEN)/gpio.exe
	rm -vf $(GEN)/me909.exe $(GEN)/routine.exe
