default_target: all
.PHONY : default_target
.SUFFIXES:
% : %,v
% : s.%
.SUFFIXES: .hpux_make_needs_suffix_list
$(VERBOSE)MAKESILENT = -s
#$(VERBOSE).SILENT:
all: IM Kai Client
EXP =
CUR = `pwd`
GEN = $(CUR)/../bin
OBJ = $(CUR)/../im.o
#CXX = $(shell bash -c "if [[ \"$(command -v nvcc >/dev/null 2>&1 || { echo g++; EXP=-Wno-c++11-narrowing; })\" == \"\" ]];then echo nvcc;else echo g++;fi;")
CXX = g++ #clang nvcc
EXP = -Wno-c++11-narrowing -Wno-unused-command-line-argument -Wno-multichar
FLAG = -g -std=c++11 -D_DEBUG -DFULLY_COMPILE
PTHD = -lpthread
IMLOCAL=../../WinNTKline/KlineUtil/IM
IMFILE=$(CUR)/IM.cc
CLIENTLOCAL=../../WinNTKline/IMclient
CLIENTFILES=$(IMLOCAL)/client.cc $(CLIENTLOCAL)/imclient.cpp
IM:
	@if [ ! -d "$(GEN)" ]; then mkdir $(GEN); fi;
	$(CXX) $(FLAG) $(IMFILE) $(PTHD) -o $(GEN)/IM.exe
.PHONY: IM
Kai.o:
	@if [ ! -d "$(OBJ)" ]; then mkdir $(OBJ); fi;
	@$(CXX) -c $(CUR)/KaiSocket.cc $(FLAG) $(PTHD) $(EXP) -o Kai.o
	@if [ -d "$(GEN)" ]; then cp kaics.cfg $(GEN)/ ; fi;
	@mv *.o $(OBJ)/;
.PHONY: Kai.o
Kai:Kai.o
	$(CXX) $(FLAG) $(OBJ)/Kai.o $(CUR)/KaiTest.cxx $(PTHD) -o $(GEN)/kaics.exe
Client:
	$(CXX) $(FLAG) -I$(IMLOCAL) $(CLIENTFILES) $(PTHD) -o $(GEN)/client.exe
clean/fast:
	@rm -rvf $(GEN)/*.exe* .*/ *.json $(OBJ) bin out build *.core
.PHONY: clean
