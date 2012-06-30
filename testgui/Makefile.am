
AM_CPPFLAGS = -I$(srcdir)/../hidapi/ $(CFLAGS_TESTGUI)

## Linux
if OS_LINUX
bin_PROGRAMS = hidapi-hidraw-testgui hidapi-libusb-testgui

hidapi_hidraw_testgui_SOURCES = test.cpp
hidapi_hidraw_testgui_LDADD = $(srcdir)/../linux/libhidapi-hidraw.la $(LIBS_TESTGUI)

hidapi_libusb_testgui_SOURCES = test.cpp
hidapi_libusb_testgui_LDADD = $(srcdir)/../libusb/libhidapi-libusb.la $(LIBS_TESTGUI)
endif

## Mac OS X
if OS_DARWIN
bin_PROGRAMS = hidapi-testgui

hidapi_testgui_SOURCES = test.cpp mac_support_cocoa.m
hidapi_testgui_LDADD = $(srcdir)/../mac/libhidapi.la $(LIBS_TESTGUI)

# Rules for copying the binary and its dependencies into the app bundle.
TestGUI.app/Contents/MacOS/hidapi-testgui$(EXEEXT): hidapi-testgui$(EXEEXT)
	./copy_to_bundle.sh

all: all-am TestGUI.app/Contents/MacOS/hidapi-testgui$(EXEEXT)

endif

## FreeBSD
if OS_FREEBSD
bin_PROGRAMS = hidapi-testgui

hidapi_testgui_SOURCES = test.cpp
hidapi_testgui_LDADD = $(srcdir)/../libusb/libhidapi.la $(LIBS_TESTGUI)
endif
