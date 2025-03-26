PWD = $(shell pwd)
obj-m += chsdev.o
ifeq ($(KERNELRELEASE),)
	KNLDIR ?= /lib/modules/$(shell uname -r)/build
endif
ARM_LINUX_KERNEL := ${KNLDIR}
KO = $(PWD)/../../kos
default:
	@make -C $(ARM_LINUX_KERNEL) M=$(PWD) modules
	@if ! lsmod | grep -q chsdev; then sudo insmod chsdev.ko; fi;
	@if [ -d $(KO)  ]; then cp chsdev.ko $(KO) ; fi;
	@echo "-- Built: \033[1mchsdev\033[0m"
clean:
	@if [ -d $(ARM_LINUX_KERNEL) ]; then make -C $(ARM_LINUX_KERNEL) M=$(PWD) clean; fi;
	@if lsmod | grep -q chsdev; then sudo rmmod chsdev; fi;
