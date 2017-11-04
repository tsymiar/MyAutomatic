GEN = `pwd`/bin
OBJ = ../obj/
PRJ = $(OBJ)gSOAPverify/
FILE= ./soap/myweb.wsdl
FLAG= -Wall -g
SYS = sysstuts.o
WEB = sqlDB.o mainSoap.o
SOAP= soapC.o stdsoap2.o soapServer.o
SQL = -std=c++11 -L/usr/lib/mysql -lmysqlclient
PTHD= -std=c++11 -lpthread #-lgsoap++#-L/usr/local/gSOAP/lib/libsoap++
gSOAPverify:$(OUT)
	g++ $(FLAG) -c ./soap/soapC.cpp
	g++ -g -c ./soap/stdsoap2.cpp
	g++ $(FLAG) -c ./soap/soapServer.cpp
	g++ $(FLAG) -c ./sys/sysstuts.cc -std=c++11
	g++ $(FLAG) -c ./sql/sqlDB.cc $(SQL)
	g++ $(FLAG) -c ./web/mainSoap.cc $(PTHD)
	[ -z "$(ls $(GEN) &> /dev/null)" ] && mkdir $(GEN) || echo "exist"
	g++ $(FLAG) -o $(GEN)/gSOAPverify $(SYS) $(SOAP) $(WEB) $(SQL) $(PTHD)
	$(shell if [ -f $(FILE) ]; then cp $(FILE) $(GEN)/ ;else @echo "file <$(FILE)> don't exist.";fi;)
	$(shell if [ -d $(PRJ) ]; then rm -rf $(OBJ);fi;)
	mkdir $(OBJ) $(PRJ)
	mv ./*.o $(PRJ)
	@echo make OK
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
	rm -f ./*.o $(GEN)/*.a $(GEN)/*.out $(GEN)/core.* $(PRJ)*.o
clear:
	rm -f ./*.o $(GEN)/*.a $(GEN)/*.out $(GEN)/core.* $(PRJ)*.o $(GEN)/gSOAPverify `ls *.o | egrep -v soapC.o stdsoap2.o soapServer.o`
	rm -f /etc/init.d/gSOAPverify /usr/sbin/gSOAPverify
	rm -rf $(GEN) $(OBJ)
	rm -rf S77_SOAP \
	y
