CUR = `pwd`
OBJ = ../obj
GEN = $(CUR)/bin
PRJ = $(OBJ)/gSOAPverify
FILE= ./soap/myweb.wsdl
FLAG= -Wall -g
SYS = sysstatus.o
WEB = sqlDB.o mainSoap.o
SOAP= soapC.o stdsoap2.o soapServer.o
BIT = $(shell if [ `getconf LONG_BIT` -eq '64' ];then echo 64;fi;)
SQL = -std=c++11 -L/usr/lib$(BIT)/mysql -lmysqlclient
PTHD= -std=c++11 -lpthread #-lgsoap++#-L/usr/local/gSOAP/lib/libsoap++
FLAG += -DMY_DEBUG
gSOAPverify:$(OUT)
	g++ $(FLAG) -c ./soap/soapC.cpp
	g++ -g -c ./soap/stdsoap2.cpp
	g++ $(FLAG) -c ./soap/soapServer.cpp
	g++ $(FLAG) -c ./sys/sysstatus.cc -std=c++11
	g++ $(FLAG) -c ./sql/sqlDB.cc $(SQL)
	g++ $(FLAG) -c ./web/mainSoap.cc $(PTHD)
	[ -z "$(ls $(GEN) &> /dev/null)" ] && mkdir $(GEN) || echo "exist"
	g++ $(FLAG) -o $(GEN)/gSOAPverify $(SYS) $(SOAP) $(WEB) $(SQL) $(PTHD)
	$(shell if [ -f $(FILE) ]; then if [ -d $(GEN) ]; then cp $(FILE) $(GEN)/;fi;else @echo "file <$(FILE)> don't exist.";fi;)
	$(shell if [ -d $(PRJ) ]; then rm -rf $(OBJ);fi;)
	mkdir $(OBJ) $(PRJ)
	mv ./*.o $(PRJ)
	echo make OK
all: 
	if[ `ps -fe| grep gSOAPverify | grep -v grep` -eq 0 ];then killall -9 gSOAPverify;else echo "making gSOAPverify.";fi
	make clear
	make
	$(GEN)/gSOAPverify 8080
install:
	make
	cp $(GEN)/gSOAPverify /usr/sbin/
	cp $(GEN)/gSOAPverify /etc/init.d/
	cd /etc/init.d/
	chmod +x gSOAPverify
	cd /etc/rc3.d/
	ln -sf ../init.d/gSOAPverify ./S77_SOAP
clean:	
	rm -f ./*.o ./core.* $(GEN)/*.a $(GEN)/*.out $(GEN)/core.* $(PRJ)/*.o
clear:
	rm -f ./*.o ./core.* $(PRJ)/*.o `ls *.o | egrep -v soapC.o stdsoap2.o soapServer.o`
	rm -rf $(GEN) $(OBJ)
	rm -f /etc/init.d/gSOAPverify /usr/sbin/gSOAPverify
	rm -rf S77_SOAP \
	y
