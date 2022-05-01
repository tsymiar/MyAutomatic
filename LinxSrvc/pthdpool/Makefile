GEN = `pwd`/../bin
OBJ = `pwd`/../obj
pthdpool:
	gcc -g -c pthdpool.c -pthread
pthdtest:
	gcc -g pthdtest.c pthdpool.o -pthread -o $(GEN)/pthdtest
	if [ -d "$(OBJ)" ]; then mv *.o $(OBJ); fi;
all:
	make pthdpool
	make pthdtest
clean:
	rm -vf *.o $(OBJ)/*.o $(GEN)/pthdtest
