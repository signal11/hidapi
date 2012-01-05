###########################################
# Simple Makefile for HIDAPI test program
#
# Alan Ott
# Signal 11 Software
# 2010-06-01
###########################################

all: hidtest

INCLUDES ?= -I../hidapi `pkg-config libusb-1.0 --cflags`

CC       ?= gcc
CFLAGS   += -Wall -g $(INCLUDES)

CXX      ?= g++
CXXFLAGS += -Wall -g $(INCLUDES)

COBJS     = hid-libusb.o
CPPOBJS   = ../hidtest/hidtest.o
OBJS      = $(COBJS) $(CPPOBJS)
LIBS      = `pkg-config libusb-1.0 libudev --libs`


hidtest: $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(LIBS) -o hidtest

clean:
	rm -f $(OBJS) hidtest

.PHONY: clean
