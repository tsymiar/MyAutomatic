CUR = `pwd`
GEN = $(CUR)/../bin
OBJ = $(CUR)/../obj
FLAG = -g -std=c++11 -D_DEBUG 
PTHD = -lpthread
IMLOCAL=../../WinNTKline/KlineUtil/IM
IMFILE=$(IMLOCAL)/IM.cc
CLIENTLOCAL=../../WinNTKline/IMclient
CLIENTFILES=$(IMLOCAL)/IMclient.cpp $(CLIENTLOCAL)/client.cpp
all: IM client Kai
IM:
	[ ! -d "$(GEN)" ] && mkdir $(GEN)
	g++ $(FLAG) $(IMFILE) -o $(GEN)/IM.exe $(PTHD)
client:
	g++ $(FLAG) -I$(IMLOCAL) $(CLIENTFILES) -o $(GEN)/client.exe $(PTHD)
Kai:
	[ ! -d "$(OBJ)" ] && mkdir $(OBJ)
	g++ -c KaiSocket.cc -std=c++11 -lpthread
	mv *.o $(OBJ)/
clean:
	rm -f $(GEN)/IM.exe $(GEN)/client.exe $(OBJ)/*.o
