CUR  = `pwd`
GEN  = $(CUR)/../bin
v4l:
	gcc -g -DV4L2 test.c -o $(GEN)/v4l2.exe
gpio:
	gcc -g -DGPIO test.c -o $(GEN)/gpio.exe
me909:
	gcc -std=gnu99 -g -DME9S test.c -o $(GEN)/me909.exe
routine:
	gcc -g routine.c -o $(GEN)/routine.exe
all:
	make v4l
	make gpio
	make me909
#	make test
clean:
	rm -f $(GEN)/v4l2.exe $(GEN)/gpio.exe $(GEN)/me909.exe $(GEN)/routine.exe
