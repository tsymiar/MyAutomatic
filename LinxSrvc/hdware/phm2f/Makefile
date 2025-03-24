PWD = $(shell pwd)
obj-m += phy_mem.o
ifeq ($(KERNELRELEASE),)
	KNLDIR ?= /lib/modules/$(shell uname -r)/build
endif
ARM_LINUX_KERNEL := ${KNLDIR}
default:
	@make -C $(ARM_LINUX_KERNEL) M=$(PWD) modules
	sudo insmod phy_mem.ko
clean:
	@if [ -d $(ARM_LINUX_KERNEL) ]; then make -C $(ARM_LINUX_KERNEL) M=$(PWD) clean; fi;
ifeq ($(shell lsmod | grep phy_mem | awk '{print $1}'), phy_mem)
	@rmmod phy_mem
endif
