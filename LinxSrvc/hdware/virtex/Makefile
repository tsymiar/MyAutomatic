PWD = $(shell pwd)
obj-m += virtexdrv.o
ifeq ($(KERNELRELEASE),)
	KNLDIR ?= /lib/modules/$(shell uname -r)/build
endif
ARM_LINUX_KERNEL := ${KNLDIR}
default:
	make -C $(ARM_LINUX_KERNEL) M=$(PWD) modules
	insmod virtexdrv.ko
clean:
	@if [ -d $(ARM_LINUX_KERNEL) ]; then make -C $(ARM_LINUX_KERNEL) M=$(PWD) clean; fi;
ifeq ($(shell lsmod | grep virtexdrv | awk '{print $1}'), virtexdrv)
	@rmmod virtexdrv
endif
