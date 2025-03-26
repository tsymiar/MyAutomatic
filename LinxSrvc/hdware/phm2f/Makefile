PWD = $(shell pwd)
obj-m += phy_mem.o
ifeq ($(KERNELRELEASE),)
	KNLDIR ?= /lib/modules/$(shell uname -r)/build
endif
ARM_LINUX_KERNEL := ${KNLDIR}
KO = $(PWD)/../../kos
default:
	@make -C $(ARM_LINUX_KERNEL) M=$(PWD) modules
	@if ! lsmod | grep -q phy_mem; then sudo insmod phy_mem.ko; fi;
	@if [ -d $(KO) ]; then cp phy_mem.ko $(KO); fi;
	@echo "-- Built: \033[1mphy_mem\033[0m"
clean:
	@if [ -d $(ARM_LINUX_KERNEL) ]; then make -C $(ARM_LINUX_KERNEL) M=$(PWD) clean; fi;
	@if lsmod | grep -q phy_mem; then sudo rmmod phy_mem; fi;
