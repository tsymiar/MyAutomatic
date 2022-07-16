PWD = $(shell pwd)
obj-m += chsdev.o
ifeq ($(KERNELRELEASE),)
	KNLDIR ?= /lib/modules/$(shell uname -r)/build
endif
ARM_LINUX_KERNEL := ${KNLDIR}
default:
	make -C $(ARM_LINUX_KERNEL) M=$(PWD) modules # insmod chsdev.ko
clean:
	make -C $(ARM_LINUX_KERNEL) M=$(PWD) clean
