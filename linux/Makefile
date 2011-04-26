###########################################
# Simple Makefile for HIDAPI test program
#
# Alan Ott
# Signal 11 Software
# 2010-06-01
###########################################

all: hidtest lib

CC=gcc
CXX=g++
COBJS=hid-libusb.o
CPPOBJS=../hidtest/hidtest.o
OBJS=$(COBJS) $(CPPOBJS)
LDFLAGS+=-Wall -fpic $(shell pkg-config libusb-1.0 libudev --libs)
CFLAGS+=-I../hidapi -Wall -g $(shell pkg-config libusb-1.0 --cflags)

lib: libhidapi.so

libhidapi.so: hid-libusb.c ../hidapi/hidapi.h
	$(CC) $(CFLAGS) $(LDFLAGS) -shared -fpic $^ -o libhidapi.so

hidtest: $(OBJS)
	g++ -Wall -g $^ $(LDFLAGS) -o hidtest

$(COBJS): %.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(CPPOBJS): %.o: %.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) hidtest libhidapi.so

.PHONY: clean lib

