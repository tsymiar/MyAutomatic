GEN  = `pwd`/../bin
pthdpool:
	gcc -g -c pthdpool.cc
pthdtest:
	gcc -g pthdtest.cc pthdpool.o -o $(GEN)/pthdtest
all:
	make pthdpool
	make pthdtest
clean:
	rm -vf *.o $(GEN)/pthdtest
