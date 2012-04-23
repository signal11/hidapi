###########################################
# Simple Makefile for HIDAPI test program
#
# Alan Ott
# Signal 11 Software
# 2010-06-01
###########################################

all: hidtest-hidraw hidtest-libusb libs

libs: libhidapi-libusb.so libhidapi-hidraw.so

CC       ?= gcc
CFLAGS   ?= -Wall -g -fpic

CXX      ?= g++
CXXFLAGS ?= -Wall -g -fpic

LDFLAGS  ?= -Wall -g

COBJS_LIBUSB = hid-libusb.o
COBJS_HIDRAW = hid.o
COBJS = $(COBJS_LIBUSB) $(COBJS_HIDRAW)
CPPOBJS   = ../hidtest/hidtest.o
OBJS      = $(COBJS) $(CPPOBJS)
LIBS_HIDAPI = -lrt
LIBS_USB  = `pkg-config libusb-1.0 --libs`
LIBS_UDEV = `pkg-config libudev --libs`
LIBS      = $(LIBS_USB) $(LIBS_UDEV)
INCLUDES ?= -I../hidapi `pkg-config libusb-1.0 --cflags`
SOVERSION = 0


# Console Test Program
hidtest-hidraw: $(COBJS_HIDRAW) $(CPPOBJS)
	$(CXX) $(LDFLAGS) $^ $(LIBS_UDEV) -o $@

hidtest-libusb: $(COBJS_LIBUSB) $(CPPOBJS)
	$(CXX) $(LDFLAGS) $^ $(LIBS_HIDAPI) $(LIBS_USB) -o $@

# Shared Libs
libhidapi-libusb.so: $(COBJS_LIBUSB)
	$(CC) $(LDFLAGS) $(LIBS_USB) $(LIBS_HIDAPI) -shared -fpic -Wl,-soname,$@.$(SOVERSION) $^ -o $@.$(SOVERSION)
	ln -s $@.$(SOVERSION) $@

libhidapi-hidraw.so: $(COBJS_HIDRAW)
	$(CC) $(LDFLAGS) $(LIBS_UDEV) -shared -fpic -Wl,-soname,$@.$(SOVERSION) $^ -o $@.$(SOVERSION)
	ln -s $@.$(SOVERSION) $@

# Objects
$(COBJS): %.o: %.c
	$(CC) $(CFLAGS) -c $(INCLUDES) $< -o $@

$(CPPOBJS): %.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $(INCLUDES) $< -o $@


clean:
	rm -f $(OBJS) hidtest-libusb hidtest-hidraw libhidapi-libusb.so libhidapi-hidraw.so ../hidtest/hidtest.o

.PHONY: clean libs
