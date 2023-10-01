CUR = `pwd`
GEN = $(CUR)/../bin
OBJ = $(CUR)/../out
PRJ = $(OBJ)/gSOAPverify
WSDL= ./soap/myweb.wsdl
FLAG= -Wall -g
STAT = status.o
MAIN = sqlDbReq.o mainSoap.o
SOAP= soapC.o stdsoap2.o soapServer.o
BIT = $(shell if [ `getconf LONG_BIT` -eq '64' ];then echo 64;fi;)
SQL = -std=c++11 -L/usr/lib$(BIT)/mysql -lmysqlclient
PTHD= -std=c++11 -lpthread #-lgsoap++#-L/usr/local/gSOAP/lib/libsoap++
FLAG += -DNS_DEBUG
gSOAPverify:
	g++ $(FLAG) -c soap/soapC.cpp
	g++ $(FLAG) -c soap/stdsoap2.cpp
	g++ $(FLAG) -c soap/soapServer.cpp
	g++ $(FLAG) -c sys/status.cc -std=c++11
#	bash -c '`command -v apt >/dev/null 2>&1 || { echo >&2 "-";}` == "-"'
#	if [ ! -d "/usr/include/mysql" ]; then if [ `command -v apt` == "" ]; then sudo yum install mariadb-devel -y; else sudo apt-get install libmysql++-dev -y; fi; fi;
	@echo "### To install mysql develop library\n### REHL: 'sudo yum install mariadb-devel -y';\n### Debain: 'sudo apt install libmysql++-dev -y';"
	@[ ! -d ${GEN} ] && mkdir ${GEN} || echo ${GEN}
ifneq ($(shell if [ ! -d /usr/lib/mysql ] && [ `ls /usr/lib/*/libmysql*.so | wc -l` -le 0 ]; then echo false; fi;), false)
	g++ $(FLAG) -c ./sql/sqlDbReq.cc $(SQL)
	g++ $(FLAG) -I.. -c ./main/mainSoap.cc $(PTHD)
	g++ $(FLAG) -o $(GEN)/gSOAPverify $(STAT) $(SOAP) $(MAIN) $(SQL) $(PTHD)
else
	@echo Built gSOAPverify without linking mysql library
endif
	$(shell if [ -f $(WSDL) ];then if [ -d $(GEN) ];then cp $(WSDL) $(GEN)/;fi;else @echo "file <$(WSDL)> don't exist.";fi;)
	$(shell if [ -d $(PRJ) ];then rm -rf $(OBJ);fi;)
	@mkdir -p $(PRJ)
	@mv $(CUR)/../*.o ${OBJ}
	@mv ./*.o $(PRJ)
	@echo make SUCCESS
all:
	@if [ `ps -fe| grep gSOAPverify | grep -v grep` -eq 0 ]; then killall -9 gSOAPverify; else echo "making gSOAPverify."; fi;
	make clear
	make -j4
	$(GEN)/gSOAPverify 8080
install:
	make -j4
	cp $(GEN)/gSOAPverify /usr/sbin/
	cp $(GEN)/gSOAPverify /etc/init.d/
	cd /etc/init.d/
	chmod +x gSOAPverify
	cd /etc/rc3.d/
	@ln -sf ../init.d/gSOAPverify ./S77_SOAP
	@cp -R ./readme/* /var/www/html
clean:
	@rm -f ./*.o ./core.* $(GEN)/*.a $(GEN)/*.out $(GEN)/core.* $(PRJ)/*.o
clear:
	@rm -f ./*.o ./core.* $(PRJ)/*.o `ls *.o | egrep -v soapC.o stdsoap2.o soapServer.o`
	rm -rvf $(GEN) $(OBJ)
	rm -f /etc/init.d/gSOAPverify /usr/sbin/gSOAPverify
	rm -rvf S77_SOAP \
	y
