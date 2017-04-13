FLAG= -Wall -g
OBJ = ./Linux/obj/
SYS = sysstuts.o
WEB = sqlDB.o MyWeb.o
SOAP= soapC.o stdsoap2.o soapServer.o
SQL = -std=c++11 -L/usr/lib/mysql -lmysqlclient
PTHD= -std=c++11 -lpthread #-lgsoap++#-L/usr/local/gSOAP/lib/libsoap++
gSOAPverify:$(OUT)
	g++ $(FLAG) -c ./soap/soapC.cpp
	g++ -g -c ./soap/stdsoap2.cpp
	g++ $(FLAG) -c ./soap/soapServer.cpp
	g++ $(FLAG) -c ./sys/sysstuts.cc -std=c++11
	g++ $(FLAG) -c ./sql/sqlDB.cc $(SQL)
	g++ $(FLAG) -c ./web/MyWeb.cc $(PTHD)
	g++ $(FLAG) -o gSOAPverify $(SYS) $(SOAP) $(WEB) $(SQL) $(PTHD)
ifeq (Linux, $(wildcard Linux))
	@echo "dir <Linux> exists."
else
	mkdir Linux
endif
ifeq ($(OBJ), $(wildcard $(OBJ)))
	@echo "dir <obj> exists."
else
	mkdir $(OBJ)
endif
	mv ./*.o $(OBJ)
all:
	if[ `ps -fe| grep gSOAPverify | grep -v grep` -eq 0 ];then killall -9 gSOAPverify;else echo "making gSOAPverify.";fi
	make clear
	make
	./gSOAPverify 8080
install:
	make
	cp ./gSOAPverify /usr/sbin/
	cp ./gSOAPverify /etc/init.d/
	cd /etc/init.d/
	chmod +x gSOAPverify
	cd /etc/rc3.d/
	ln -sf ../init.d/gSOAPverify ./S77_SOAP
clean:	
	rm -f *.out core.* ./*.o $(OBJ)*.o
clear:
	rm -f ./*.o *.out core.* gSOAPverify `ls *.o | egrep -v soapC.o stdsoap2.o soapServer.o`
	rm -f /etc/init.d/gSOAPverify /usr/sbin/gSOAPverify
	`rm -rf ./Linux/`
	rm -rf S77_SOAP \
	y
