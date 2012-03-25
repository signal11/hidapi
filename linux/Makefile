###########################################
# Simple Makefile for HIDAPI test program
#
# Alan Ott
# Signal 11 Software
# 2010-06-01
###########################################

all: hidtest

libs: libhidapi-libusb.so libhidapi-hidraw.so

CC       ?= gcc
CFLAGS   ?= -Wall -g

CXX      ?= g++
CXXFLAGS ?= -Wall -g

COBJS     = hid-libusb.o
CPPOBJS   = ../hidtest/hidtest.o
OBJS      = $(COBJS) $(CPPOBJS)
LIBS_USB  = `pkg-config libusb-1.0 --libs`
LIBS_UDEV = `pkg-config libudev --libs`
LIBS      = $(LIBS_USB) $(LIBS_UDEV)
INCLUDES ?= -I../hidapi `pkg-config libusb-1.0 --cflags`


hidtest: $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(LIBS) -o hidtest

libhidapi-libusb.so: ../hidapi/hidapi.h hid-libusb.c
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) $(LIBS_USB) -shared -fpic -Wl,-soname,$@.0 $^ -o $@

libhidapi-hidraw.so: ../hidapi/hidapi.h hid.c
	$(CC) $(CFLAGS) -I../hidapi $(LDFLAGS) $(LIBS_UDEV) -shared -fpic -Wl,-soname,$@.0 $^ -o $@

$(COBJS): %.o: %.c
	$(CC) $(CFLAGS) -c $(INCLUDES) $< -o $@

$(CPPOBJS): %.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $(INCLUDES) $< -o $@

clean:
	rm -f $(OBJS) hidtest libhidapi-libusb.so libhidapi-hidraw.so

.PHONY: clean libs
