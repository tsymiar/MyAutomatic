PWD = $(shell pwd)
obj-m += chsdev.o
ifeq ($(KERNELRELEASE),)
	KNLDIR ?= /lib/modules/$(shell uname -r)/build
endif
ARM_LINUX_KERNEL := ${KNLDIR}
default:
	make -C $(ARM_LINUX_KERNEL) M=$(PWD) modules
	insmod chsdev.ko
clean:
	@if [ -d $(ARM_LINUX_KERNEL) ]; then make -C $(ARM_LINUX_KERNEL) M=$(PWD) clean; fi;
ifeq ($(shell lsmod | grep chsdev | awk '{print $1}'), chsdev)
	@rmmod chsdev
endif
