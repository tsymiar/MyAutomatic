PWD = $(shell pwd)
GEN = $(PWD)/../bin
KO = $(PWD)/../kos
OS = $(shell uname -s)
#BUILD = ${PWD}/../../../kernel/linux
BUILD = /lib/modules/$(shell uname -r)/build
chigpio:
	@gcc -g -DGPIO test.c -o $(GEN)/chigpio
	@echo "-- Built: \033[1m$@\033[0m"
mes909:
	@gcc -std=gnu99 -g -DME9S test.c -o $(GEN)/mes909 -Wno-implicit-function-declaration
	@echo "-- Built: \033[1m$@\033[0m"
dirty:
	@gcc -g -pthread dirtyc0w.c -o $(GEN)/dirtyhack -Wno-int-conversion -Wno-pointer-to-int-cast
	@echo "-- Built: \033[1m$@\033[0m"
chsdev:
	@if [ -d $(BUILD) ]; then make -C chsdev KNLDIR=$(BUILD); fi;
virtex:
	@if [ -d $(BUILD) ]; then make -C virtex KNLDIR=$(BUILD); fi;
RDMA:
	@if command -v apt >/dev/null 2>&1; then \
		echo "fetch RDMA dependencies..."; \
		sudo apt install -y rdma-core librdmacm-dev ibverbs-utils; \
	fi;
	@gcc -g ./rdma/server.c -lrdmacm -libverbs -o $(GEN)/rdma_server.exe
	@gcc -g ./rdma/client.c -lrdmacm -libverbs -o $(GEN)/rdma_client.exe
	@echo "-- Built: \033[1m$@\033[0m"
chstest:
	@gcc -g -DDRIVER test.c -o $(GEN)/chstest
	@echo "-- Built: \033[1m$@\033[0m"
phm2f:
	@if [ -d $(BUILD) ]; then make -C phy_mem KNLDIR=$(BUILD); fi;
phm2f_test:
	@gcc -g test_phm2f.c -o $(GEN)/test_phm2f
	@echo "-- Built: \033[1m$@\033[0m"
image:
	@gcc -g -DSNAP test.c linuxsnap.c -o $(GEN)/imagesnap.exe
	@echo "-- Built: \033[1m$@\033[0m"
video:
	@gcc -g -DVIDEO test.c routine.c -lm -o $(GEN)/VideoCapture
	@echo "-- Built: \033[1m$@\033[0m"
pipefifo:
	@gcc -g -DPIPE_FIFO test.c -o $(GEN)/pipefifo
	@echo "-- Built: \033[1m$@\033[0m"
all:
	@make chigpio
	@make mes909
	@make dirty
ifneq ($(OS), )
	@echo Making $(OS) drivers
	@make -C chsdev
	@make -C virtex
	@make RDMA
	@make -C phm2f
	@make chstest
	@make phm2f_test
ifeq ($(OS), Linux)
	@echo Making image/video
	@make image
	@make video
endif
endif
	@echo Making tools
	@make pipefifo
clean:
	@rm -rvf $(GEN)/chigpio* $(GEN)/mes909* $(GEN)/dirtyhack* $(GEN)/chstest*
	@rm -rvf $(GEN)/VideoCapture* $(GEN)/imagesnap* $(GEN)/pipefifo*
	@rm -rvf $(GEN)/rdma_*.exe
	@rm -rvf $(GEN)/test_*
ifeq ($(shell uname), Linux)
	@make -C chsdev clean
	@make -C virtex clean
	@make -C phm2f clean
endif
	@rm -rvf $(KO)
