KERNELDIR := 改成自己使用的内核路径
CURRENT_PATH := $(shell pwd)

obj-m := gt9147.o

build: kernel_modules

kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
