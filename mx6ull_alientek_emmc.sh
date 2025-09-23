#!/bin/bash
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- distclean
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- mx6ull_alientek_emmc_defconfig
make V=1 ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -j12
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- u-boot.bin
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- u-boot.imx