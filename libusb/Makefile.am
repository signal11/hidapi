AM_CPPFLAGS = -I$(srcdir)/../hidapi $(CFLAGS_LIBUSB)

if OS_LINUX
lib_LTLIBRARIES = libhidapi-libusb.la
libhidapi_libusb_la_SOURCES = hid.c
libhidapi_libusb_la_LDFLAGS = -version-info 0:0:0
libhidapi_libusb_la_LIBADD = $(LIBS_LIBUSB)
endif

if OS_FREEBSD
lib_LTLIBRARIES = libhidapi.la
libhidapi_la_SOURCES = hid.c
libhidapi_la_LDFLAGS = -version-info 0:0:0
libhidapi_la_LIBADD = $(LIBS_LIBUSB)
endif

hdrdir = $(includedir)/hidapi
hdr_HEADERS = $(srcdir)/../hidapi/hidapi.h
