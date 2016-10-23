WEB = MyWeb.o
SOAP= soapC.o stdsoap2.o soapServer.o
SQL = -std=c++11 -L/usr/lib/mysql -lmysqlclient
PTHD= -std=c++11 -lpthread #-lgsoap++#-L/usr/local/gSOAP/lib/libsoap++
gSOAPverify:$(OUT)
	g++ -c ./soap/soapC.cpp
	g++ -c ./soap/stdsoap2.cpp
	g++ -c ./soap/soapServer.cpp
	g++ -c ./sql/sqlDB.cc $(SQL)
	g++ -c ./MyWeb.cc $(PTHD)
	g++ -o gSOAPverify $(SOAP) sqlDB.o $(WEB) $(SQL) $(PTHD)
clean:
	rm -f *.o *.out gSOAPverify
