PWD = $(shell pwd)
obj-m += virtexdrv.o
ifeq ($(KERNELRELEASE),)
	KNLDIR ?= /lib/modules/$(shell uname -r)/build
endif
ARM_LINUX_KERNEL := ${KNLDIR}
KO = $(PWD)/../../kos
default:
	@if [ -d $(ARM_LINUX_KERNEL) ]; then \
		make -C $(ARM_LINUX_KERNEL) M=$(PWD) modules && \
		lsmod | grep -q virtexdrv || sudo insmod virtexdrv.ko && \
		[ -d $(KO) ] && cp virtexdrv.ko $(KO) && \
		echo "-- Built: \033[1mvirtexdrv\033[0m" || \
		echo "Warning: chsdev build break"; \
	else \
		echo "$(ARM_LINUX_KERNEL) doesn't exist"; \
	fi;
clean:
	@if [ -d $(ARM_LINUX_KERNEL) ]; then make -C $(ARM_LINUX_KERNEL) M=$(PWD) clean; fi;
	@if lsmod | grep -q virtexdrv; then sudo rmmod virtexdrv; fi;
