lib_LTLIBRARIES = libhidapi-libusb.la
libhidapi_libusb_la_SOURCES = hid.c
libhidapi_libusb_la_LDFLAGS = -version-info 0:0:0
AM_CPPFLAGS = -I$(srcdir)/../hidapi $(CFLAGS_LIBUSB)
libhidapi_libusb_la_LIBADD = $(LIBS_LIBUSB)

hdrdir = $(includedir)/hidapi
hdr_HEADERS = $(srcdir)/../hidapi/hidapi.h
