ARCH ?= arm
CROSS_COMPILE ?= arm-linux-gnueabihf-

KERNELDIR := ../../linux_kernel
CURRENT_PATH := $(shell pwd)
obj-m := chrdevbase.o

build: kernel_modules

kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) clean