lib_LTLIBRARIES = libhidapi-hidraw.la
libhidapi_hidraw_la_SOURCES = hid.c
libhidapi_hidraw_la_LDFLAGS = -version-info 0:0:0
AM_CPPFLAGS = -I$(srcdir)/../hidapi/ $(CFLAGS_HIDRAW)
libhidapi_hidraw_la_LIBADD = $(LIBS_HIDRAW)

hdrdir = $(includedir)/hidapi
hdr_HEADERS = $(srcdir)/../hidapi/hidapi.h
