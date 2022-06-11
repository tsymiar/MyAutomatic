EXP =
CUR = `pwd`
GEN = $(CUR)/../bin
OBJ = $(CUR)/../obj
#CXX = $(shell bash -c "if [[ \"$(command -v nvcc >/dev/null 2>&1 || { echo g++; EXP=-Wno-c++11-narrowing; })\" == \"\" ]];then echo nvcc;else echo g++;fi;")
CXX = g++ #clang nvcc
EXP = -Wno-c++11-narrowing -Wno-unused-command-line-argument -Wno-multichar
FLAG = -g -std=c++11 -D_DEBUG -DFULLY_COMPILE
PTHD = -lpthread
IMLOCAL=../../WinNTKline/KlineUtil/IM
IMFILE=$(CUR)/IM.cc
CLIENTLOCAL=../../WinNTKline/IMclient
CLIENTFILES=$(IMLOCAL)/client.cc $(CLIENTLOCAL)/imclient.cpp
all: IM client Kai
IM:
	@if [ ! -d "$(GEN)" ]; then mkdir $(GEN); fi;
	$(CXX) $(FLAG) $(IMFILE) -o $(GEN)/IM.exe $(PTHD)
client:
	$(CXX) $(FLAG) -I$(IMLOCAL) $(CLIENTFILES) -o $(GEN)/Client.exe $(PTHD)
Kai:
	@if [ ! -d "$(OBJ)" ]; then mkdir $(OBJ); fi;
	$(CXX) -c $(CUR)/KaiSocket.cc  $(FLAG) $(PTHD) $(EXP)
	$(CXX) -g $(CUR)/KaiSocket.cc $(CUR)/KaiTest.cpp $(FLAG) $(PTHD) -o $(GEN)/kaics.exe $(EXP)
	@mv *.o $(OBJ)/
clean:
	@rm -rvf $(GEN)/*.exe* $(OBJ) bin build out
