###########################################
# Simple Makefile for HIDAPI test program
#
# Alan Ott
# Signal 11 Software
# 2010-06-01
###########################################

all: hidtest-hidraw libs

libs: libhidapi-hidraw.so

INCLUDES ?= -I../hidapi `pkg-config libusb-1.0 --cflags`

CC       ?= gcc
CFLAGS   ?= -Wall -g -fpic $(INCLUDES)

CXX      ?= g++
CXXFLAGS ?= -Wall -g -fpic $(INCLUDES)

LDFLAGS  ?= -Wall -g

COBJS_HIDRAW = hid.o
CPPOBJS   = ../hidtest/hidtest.o
OBJS      = $(COBJS) $(CPPOBJS)
LIBS_UDEV = `pkg-config libudev --libs` -lrt
LIBS      = $(LIBS_UDEV)

# Console Test Program
hidtest-hidraw: $(COBJS_HIDRAW) $(CPPOBJS)
	$(CXX) $(LDFLAGS) $^ $(LIBS_UDEV) -o $@

# Shared Libs
libhidapi-hidraw.so: $(COBJS_HIDRAW)
	$(CC) $(LDFLAGS) $(LIBS_UDEV) -shared -fpic -Wl,-soname,$@.0 $^ -o $@

# Objects
$(COBJS): %.o: %.c
	$(CC) $(CFLAGS) -c $(INCLUDES) $< -o $@

$(CPPOBJS): %.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $(INCLUDES) $< -o $@

clean:
	rm -f $(OBJS) hidtest-hidraw libhidapi-hidraw.so ../hidtest/hidtest.o

.PHONY: clean libs
