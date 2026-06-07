CUR  = $(shell pwd)
GEN  = $(CUR)/../bin
OBJ  = $(CUR)/../out
PRJ  = $(OBJ)/gSOAPverify
WSDL = ./soap/myweb.wsdl

FLAG  = -Wall -g -DNS_DBG -std=c++11
SOAP  = soapC.o stdsoap2.o soapServer.o
STAT  = status.o
MAIN  = sqlDbReq.o mainSoap.o

BIT   = $(shell if [ `getconf LONG_BIT` -eq '64' ]; then echo 64; fi)
SQL   = -I/usr/lib$(BIT)/mysql -lmysqlclient
PTHD  = -lpthread

gSOAPverify:
	@g++ $(FLAG) -c soap/soapC.cpp
	@g++ $(FLAG) -c soap/stdsoap2.cpp
	@g++ $(FLAG) -c soap/soapServer.cpp
	@g++ $(FLAG) -c sys/status.cc
#	bash -c '`command -v apt >/dev/null 2>&1 || { echo >&2 "-";}` == "-"'
#	if [ ! -d "/usr/include/mysql" ]; then if [ `command -v apt` == "" ]; then sudo yum install mariadb-devel -y; else sudo apt-get install libmysql++-dev -y; fi; fi;
	@echo "### To install MySQL development library:"
	@echo "###  \033[1mREHL:\033[0m   sudo yum install mariadb-devel -y"
	@echo "###  \033[1mDebian:\033[0m  sudo apt install libmysql++-dev -y"
	@[ ! -d ${GEN} ] && mkdir ${GEN} || echo "-- [$(abspath ${GEN})] exists!"
ifneq ($(shell if [ ! -d /usr/lib/mysql ] && [ `ls /usr/lib/*/libmysql*.so 2>/dev/null | wc -l` -le 0 ]; then echo false; fi;), false)
	@g++ $(FLAG) -c ./sql/sqlDbReq.cc $(SQL)
	@g++ $(FLAG) -I.. -c ./main/mainSoap.cc $(PTHD)
	@g++ $(FLAG) -o $(GEN)/gSOAPverify $(STAT) $(SOAP) $(MAIN) $(SQL) $(PTHD)
	@echo "-- Built: \033[1m$@\033[0m"
else
	@echo "[100%] Built gSOAPverify without MySQL library"
endif
	$(shell if [ -f $(WSDL) ]; then if [ -d $(GEN) ]; then cp $(WSDL) $(GEN)/; fi; else echo "file <$(WSDL)> doesn't exist."; fi)
	$(shell if [ -d $(PRJ) ]; then rm -rf $(OBJ); fi)
	@mkdir -p $(PRJ)
	@mv $(CUR)/../*.o ${OBJ} 2>/dev/null || true
	@mv ./*.o $(PRJ) 2>/dev/null || true
	@echo "-- make gSOAPverify SUCCESS"

all:
	@if ps -fe | grep -q '[g]SOAPverify'; then killall -9 gSOAPverify; else echo "making gSOAPverify."; fi
	make clear
	make -j4
	$(GEN)/gSOAPverify 8800

install:
	make -j4
	cp $(GEN)/gSOAPverify /usr/sbin/
	cp $(GEN)/gSOAPverify /etc/init.d/
	chmod +x /etc/init.d/gSOAPverify
	cd /etc/rc3.d/ && ln -sf ../init.d/gSOAPverify ./S77_SOAP
	@cp -R ./readme/* /var/www/html 2>/dev/null || true

clean:
	@rm -f ./*.o ./core.* $(GEN)/*.a $(GEN)/*.out $(GEN)/core.* $(PRJ)/*.o $(GEN)/gSOAPverify

clear:
	@rm -f ./*.o ./core.* $(PRJ)/*.o
	@rm -f `ls *.o 2>/dev/null | egrep -v 'soapC.o|stdsoap2.o|soapServer.o'`
	@rm -rvf $(GEN) $(OBJ)
	@rm -f /etc/init.d/gSOAPverify /usr/sbin/gSOAPverify
	@rm -f S77_SOAP
