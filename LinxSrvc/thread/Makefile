GEN = `pwd`/../bin
OBJ = `pwd`/../out
pthdpool:
	@gcc -g -c pthdpool.c -pthread -Wno-pointer-to-int-cast
	@echo "-- Built: \033[1m$@\033[0m"
pthdtest:
	@gcc -g pthdtest.c pthdpool.o -pthread -o $(GEN)/pthdtest.exe
	@if [ -d $(OBJ) ]; then mv *.o $(OBJ); fi;
	@echo "-- Built: \033[1m$@\033[0m"
all:
	@make pthdpool
	@make pthdtest
clean:
	@rm -vf *.o $(OBJ)/pthdpool.o $(GEN)/pthdtest.exe
