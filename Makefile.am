

if OS_FREEBSD
pkgconfigdir=$(prefix)/libdata/pkgconfig
else
pkgconfigdir=$(libdir)/pkgconfig
echo $pkgconfigdir
endif

if OS_LINUX
pkgconfig_DATA=pc/hidapi-hidraw.pc pc/hidapi-libusb.pc
else
pkgconfig_DATA=pc/hidapi.pc
endif


SUBDIRS=

if OS_LINUX
SUBDIRS += linux libusb
endif

if OS_DARWIN
SUBDIRS += mac
endif

if OS_FREEBSD
SUBDIRS += libusb
endif

if BUILD_TESTGUI
SUBDIRS += testgui
endif
