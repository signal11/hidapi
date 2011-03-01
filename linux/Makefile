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
COBJS=hid.o
CPPOBJS=../hidtest/hidtest.o
OBJS=$(COBJS) $(CPPOBJS)
CFLAGS+=-I../hidapi -Wall -fPIC -shared -g -c
LIBS=`pkg-config libudev --libs`


hidtest: $(OBJS)
	g++ -Wall -g $^ $(LIBS) -o hidtest

libhidapi: $(COBJS) 
	gcc -fPIC -shared -Wall -g $^ $(LIBS) -o libhidapi.so

$(COBJS): %.o: %.c
	$(CC) $(CFLAGS) $< -o $@

$(CPPOBJS): %.o: %.cpp
	$(CXX) $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJS) hidtest libhidapi.so

.PHONY: clean
