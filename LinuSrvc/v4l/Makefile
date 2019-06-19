CUR  = `pwd`
GEN  = $(CUR)/../bin
v4l:
	gcc -g v4l2.c -o $(GEN)/v4l2.exe
test:
	gcc -g sample.c -o $(GEN)/yuvtest.exe
all:
	make v4l
	make test
clean:
	rm -f $(GEN)/v4l2.exe $(GEN)/yuvtest.exe
