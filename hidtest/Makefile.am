
AM_CPPFLAGS = -I$(srcdir)/../hidapi/

## Linux
if OS_LINUX
noinst_PROGRAMS = hidtest-libusb hidtest-hidraw

hidtest_hidraw_SOURCES = hidtest.cpp
hidtest_hidraw_LDADD = $(srcdir)/../linux/libhidapi-hidraw.la

hidtest_libusb_SOURCES = hidtest.cpp
hidtest_libusb_LDADD = $(srcdir)/../libusb/libhidapi-libusb.la
else

# Other OS's
noinst_PROGRAMS = hidtest

hidtest_SOURCES = test.cpp
hidtest_LDADD = $(srcdir)/../windows/libhidapi.la

endif
