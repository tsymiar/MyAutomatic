GEN = `pwd`/../bin
OBJ = `pwd`/../out
pthdpool:
	@gcc -g -c pthdpool.c -pthread -Wno-pointer-to-int-cast
	@echo "-- Built: \033[1m$@\033[0m"
pthdtest:
	@gcc -g pthdtest.c pthdpool.o -pthread -o $(GEN)/pthdtest.exe
	@if [ -d $(OBJ) ]; then mv *.o $(OBJ); fi;
	@echo "-- Built: \033[1m$@\033[0m"
socktest:
	@g++ -g -std=c++11 socktest.cpp -lpthread -Wno-vla-cxx-extension -o $(GEN)/socket_server
	@g++ -g -std=c++11 socktest.cpp -lpthread -DCLIENT -Wno-vla-cxx-extension -o $(GEN)/socket_client
	@echo "-- Built: \033[1m$@\033[0m"
all:
	@make pthdpool
	@make pthdtest
	@make socktest
clean:
	@rm -vrf *.o $(OBJ)/pthdpool.o $(GEN)/pthdtest.exe* $(GEN)/socket_*
