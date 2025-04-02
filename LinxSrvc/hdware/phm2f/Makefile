PWD = $(shell pwd)
obj-m += phy_mem.o
ifeq ($(KERNELRELEASE),)
	KNLDIR ?= /lib/modules/$(shell uname -r)/build
endif
ARM_LINUX_KERNEL := ${KNLDIR}
default:
	@if [ -d $(ARM_LINUX_KERNEL) ]; then \
		make -C $(ARM_LINUX_KERNEL) M=$(PWD) modules && \
		lsmod | grep -q phy_mem || sudo insmod phy_mem.ko && \
		[ -d $(KO) ] && cp phy_mem.ko $(KO) && \
		echo "Warning: chsdev build break"; \
	else \
		echo "$(ARM_LINUX_KERNEL) doesn't exist"; \
	fi;
	@if [ "$(OS)" = "Linux" ]; then \
		gcc -g ./test.c -o $(GEN)/test_phm2f.exe; \
	fi
clean:
	@if [ -d $(ARM_LINUX_KERNEL) ]; then make -C $(ARM_LINUX_KERNEL) M=$(PWD) clean; fi;
	@if lsmod | grep -q phy_mem; then sudo rmmod phy_mem; fi;
