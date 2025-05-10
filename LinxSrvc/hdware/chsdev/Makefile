PWD = $(shell pwd)
obj-m += chsdev.o
ifeq ($(KERNELRELEASE),)
	KNLDIR ?= /lib/modules/$(shell uname -r)/build
endif
ARM_LINUX_KERNEL := ${KNLDIR}
default:
	@if [ -d $(ARM_LINUX_KERNEL) ]; then \
		make -C $(ARM_LINUX_KERNEL) M=$(PWD) modules && \
		lsmod | grep -q chsdev || { \
			if [ "$$(id -u)" -ne 0 ]; then \
				echo "\033[33mWarning\033[0m: Must be superuser."; \
			fi; \
			sudo insmod chsdev.ko; \
		} && \
		[ -d $(KO) ] && cp chsdev.ko $(KO) && \
		echo "-- Built: \033[1mchsdev\033[0m" || \
		echo "\033[33mWarning\033[0m: chsdev build break!"; \
	else \
		echo "$(ARM_LINUX_KERNEL) doesn't exist!"; \
	fi;
clean:
	@if [ -d $(ARM_LINUX_KERNEL) ]; then make -C $(ARM_LINUX_KERNEL) M=$(PWD) clean; fi;
	@if lsmod | grep -q chsdev; then sudo rmmod chsdev; fi;
