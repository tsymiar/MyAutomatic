WEB = sqlDB.o MyWeb.o
FLAG= -Wall -g
SOAP= soapC.o stdsoap2.o soapServer.o
SQL = -std=c++11 -L/usr/lib/mysql -lmysqlclient
PTHD= -std=c++11 -lpthread #-lgsoap++#-L/usr/local/gSOAP/lib/libsoap++
gSOAPverify:$(OUT)
	g++ -c ./soap/soapC.cpp
	g++ -c ./soap/stdsoap2.cpp
	g++ -c ./soap/soapServer.cpp
	g++ $(FLAG) -c ./sql/sqlDB.cc $(SQL)
	g++ $(FLAG) -c ./MyWeb.cc $(PTHD)
	g++ $(FLAG) -o gSOAPverify $(SOAP) $(WEB) $(SQL) $(PTHD)
all:
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
	rm -f *.out gSOAPverify `ls *.o | egrep -v soapC.o stdsoap2.o soapServer.o`
clear:
	rm -f *.o *.out gSOAPverify
	rm -f /etc/init.d/gSOAPverify /usr/sbin/gSOAPverify
	rm -rf S77_SOAP
	y
