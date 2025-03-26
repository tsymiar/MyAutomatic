PWD = $(shell pwd)
obj-m += virtexdrv.o
ifeq ($(KERNELRELEASE),)
	KNLDIR ?= /lib/modules/$(shell uname -r)/build
endif
ARM_LINUX_KERNEL := ${KNLDIR}
KO = $(PWD)/../../kos
default:
	@make -C $(ARM_LINUX_KERNEL) M=$(PWD) modules
	@if ! lsmod | grep -q virtexdrv; then sudo insmod virtexdrv.ko; fi;
	@if [ -d $(KO) ]; then cp virtexdrv.ko $(KO); fi;
	@echo "-- Built: \033[1mvirtexdrv\033[0m"
clean:
	@if [ -d $(ARM_LINUX_KERNEL) ]; then make -C $(ARM_LINUX_KERNEL) M=$(PWD) clean; fi;
	@if lsmod | grep -q virtexdrv; then sudo rmmod virtexdrv; fi;
