PWD = $(shell pwd)
GEN = $(PWD)/../bin
KO = $(PWD)/../kos
OS = $(shell uname -s)
#BUILD = ${PWD}/../../../kernel/linux
BUILD = /lib/modules/$(shell uname -r)/build
define Built
  @echo "-- Built: \033[1m$1\033[0m"
endef
export GEN KO Built OS
null:
	@echo "\033[7;41m$(shell uname -a)\033[0m"
chigpio:
	@gcc -g -DGPIO test.c -o $(GEN)/chigpio
	$(call Built,$@)
mes909:
	@gcc -std=gnu99 -g -DME9S test.c -o $(GEN)/mes909 -Wno-implicit-function-declaration
	$(call Built,$@)
dirty:
	@gcc -g -pthread dirtyc0w.c -o $(GEN)/dirtyhack -Wno-int-conversion -Wno-pointer-to-int-cast
	$(call Built,$@)
chsdev:
	@if [ -d $(BUILD) ]; then make -C chsdev KNLDIR=$(BUILD); fi;
	$(call Built,$@)
virtex:
	@if [ -d $(BUILD) ]; then make -C virtex KNLDIR=$(BUILD); fi;
	$(call Built,$@)
RDMA:
	@make -C rdma env
	@make -C rdma all -j4
	$(call Built,$@)
chstest:
	@gcc -g -DDRIVER test.c -o $(GEN)/chstest
	$(call Built,$@)
phm2f:
	@if [ -d $(BUILD) ]; then make -C phy_mem KNLDIR=$(BUILD); fi;
	$(call Built,$@)
image:
	@gcc -g -DSNAP test.c linuxsnap.c -o $(GEN)/imagesnap.exe
	$(call Built,$@)
video:
	@gcc -g -DVIDEO test.c routine.c -lm -o $(GEN)/VideoCapture
	$(call Built,$@)
pipefifo:
	@gcc -g -DPIPE_FIFO test.c -o $(GEN)/pipefifo
	$(call Built,$@)
all:
	@make null
	@make chigpio
	@make mes909
	@make dirty
ifneq ($(OS), )
	@echo "\033[4;3mMaking $(OS) driver\033[0m"
	@make -C chsdev
	@make -C virtex
	@make RDMA
	@make -C phm2f || true
	@make chstest
ifeq ($(OS), Linux)
	@echo "\033[4;3mMaking image/video\033[0m"
	@make image
	@make video
endif
endif
	@echo "\033[4;3mMaking tools\033[0m"
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
	@make -C rdma clean
endif
	@rm -rvf $(KO)
