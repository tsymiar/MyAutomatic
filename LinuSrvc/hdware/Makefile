CUR  = `pwd`
GEN  = $(CUR)/../bin
v4l:
	gcc -g v4l2.c -o $(GEN)/v4l2.exe
test:
	gcc -g sample.c -o $(GEN)/yuvtest.exe
me909:
	gcc -g me909s.c -o $(GEN)/me909.exe
all:
	make v4l
	make test
	make me909
clean:
	rm -f $(GEN)/v4l2.exe $(GEN)/yuvtest.exe $(GEN)/me909.exe
