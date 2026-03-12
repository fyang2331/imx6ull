# Makefile for camera_to_lcd

ARCH ?= arm
CROSS_COMPILE ?= arm-linux-gnueabihf-
CC := $(CROSS_COMPILE)gcc

CFLAGS := -Wall -O2
LDFLAGS := -pthread
TARGET := camera_to_lcd
SRCS := camera_to_lcd.c
OBJS := $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)