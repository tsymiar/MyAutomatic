default_target: all
.PHONY : default_target
.SUFFIXES:
% : %,v
% : s.%
.SUFFIXES: .hpux_make_needs_suffix_list
$(VERBOSE)MAKESILENT = -s
#$(VERBOSE).SILENT:
all: IM Kai client socket_test
EXP =
CUR = `pwd`
GEN = $(CUR)/../bin
OBJ = $(CUR)/../im.o
#CXX = $(shell bash -c "if [[ \"$(command -v nvcc >/dev/null 2>&1 || { echo g++; EXP=-Wno-c++11-narrowing; })\" == \"\" ]];then echo nvcc;else echo g++;fi;")
CXX = g++ #clang nvcc
EXP = -Wno-c++11-narrowing -Wno-unused-command-line-argument -Wno-multichar -Wno-address-of-packed-member
FLAG = -g -std=c++11 -D_DEBUG -DFULLY_COMPILE
PTHD = -lpthread
IMLOCAL=../../WinNTKline/KlineUtil/IM
IMFILE=$(CUR)/IM.cc
CLIENTLOCAL=../../WinNTKline/IMclient
CLIENTFILES=$(IMLOCAL)/client.cc $(CLIENTLOCAL)/imclient.cpp
IM:
	@if [ ! -d "$(GEN)" ]; then mkdir $(GEN); fi;
	@$(CXX) $(FLAG) $(IMFILE) $(PTHD) -o $(GEN)/IM.exe
	@echo "-- Built: \033[1;32m$@\033[0m"
.PHONY: IM
Kai.o:
	@if [ ! -d "$(OBJ)" ]; then mkdir $(OBJ); fi;
	@$(CXX) -c $(CUR)/KaiSocket.cc $(FLAG) $(PTHD) $(EXP) -o Kai.o
	@if [ -d "$(GEN)" ]; then cp kaics.cfg $(GEN)/ ; fi;
	@mv *.o $(OBJ)/;
	@echo "-- Built: \033[1;32m$@\033[0m"
.PHONY: Kai.o
Kai:Kai.o
	@$(CXX) $(FLAG) $(OBJ)/Kai.o $(CUR)/KaiTest.cxx $(PTHD) -o $(GEN)/kaics.exe
	@echo "-- Built: \033[1;32m$@\033[0m"
client:
	@$(CXX) $(FLAG) -I$(IMLOCAL) $(CLIENTFILES) $(PTHD) -o $(GEN)/client.exe
	@echo "-- Built: \033[1;32m$@\033[0m"
socket_test:
	@$(CXX) $(FLAG) $(IMLOCAL)/TCPsocket_unix.cpp $(PTHD) -o $(GEN)/socket_test.exe
	@echo "-- Built: \033[1;32m$@\033[0m"
clean/fast:
	@if [ -d "$(GEN)/../lib" ]; then rm -rvf $(GEN)/../lib; fi;
	@rm -rvf $(GEN)/*.exe* $(GEN)/*.bin .*/ *.json $(OBJ) bin out build *.core
.PHONY: clean
