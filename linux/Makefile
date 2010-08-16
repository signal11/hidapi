###########################################
# Simple Makefile for HIDAPI test program
#
# Alan Ott
# Signal 11 Software
# 2010-06-01
###########################################

all: hidtest

CC=gcc
CXX=g++
COBJS=hid-libusb.o
CPPOBJS=../hidtest/hidtest.o
OBJS=$(COBJS) $(CPPOBJS)
CFLAGS=-I../hidapi -Wall -g -c
LIBS=-ludev -lusb-1.0


hidtest: $(OBJS)
	g++ -Wall -g $^ $(LIBS) -o hidtest

$(COBJS): %.o: %.c
	$(CC) $(CFLAGS) $< -o $@

$(CPPOBJS): %.o: %.cpp
	$(CXX) $(CFLAGS) $< -o $@

clean:
	rm *.o hidtest

.PHONY: clean
