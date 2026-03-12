# Makefile for v4l2_test/test01

ARCH ?= arm
CROSS_COMPILE ?= arm-linux-gnueabihf-
CC := $(CROSS_COMPILE)gcc

CFLAGS := -Wall -O2
LDFLAGS := -pthread
TARGET := test01
SRCS := main.c
OBJS := $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)