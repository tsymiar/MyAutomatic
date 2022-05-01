GEN  = `pwd`/../bin
pthdpool:
	gcc -g -c pthdpool.cc -pthread
pthdtest:
	gcc -g pthdtest.cc pthdpool.o -pthread -o $(GEN)/pthdtest
all:
	make pthdpool
	make pthdtest
clean:
	rm -vf *.o $(GEN)/pthdtest
