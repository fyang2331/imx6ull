ARCH ?= arm
CROSS_COMPILE ?= /opt/fsl-imx-x11/4.1.15-2.1.0/sysroots/x86_64-pokysdk-linux/usr/bin/arm-poky-linux-gnueabi/arm-poky-linux-gnueabi-
CC := $(CROSS_COMPILE)gcc

KERNELDIR := ../../linux_kernel
CURRENT_PATH := $(shell pwd)
obj-m := led.o

build: kernel_modules

kernel_modules:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
clean:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean