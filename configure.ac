AC_PREREQ([2.65])
AC_INIT([hidapi], [0.8], [alan@signal11.us])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])
AC_CONFIG_MACRO_DIR([m4])
LT_INIT

AC_PROG_CC
AC_PROG_CXX
AC_PROG_OBJC
PKG_PROG_PKG_CONFIG

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

hidapi_lib_error() {
	echo ""
	echo "  Library $1 was not found on this system."
	echo "  Please install it and re-run ./configure"
	echo ""
	exit 1
}

AC_MSG_CHECKING([operating system])
AC_MSG_RESULT($host)
case $host in
*-linux*)
	AC_MSG_RESULT([ (Linux back-end)])
	AC_DEFINE(OS_LINUX, 1, [Linux implementations])
	AC_SUBST(OS_LINUX)
	backend="linux"

	# HIDAPI/hidraw libs
	PKG_CHECK_MODULES([libudev], [libudev])
	LIBS_HIDRAW_PR+=" $libudev_LIBS"
	CFLAGS_HIDRAW+=" $libudev_CFLAGS"
	AC_CHECK_LIB([rt], [clock_gettime], [LIBS_HIDRAW_PR+=" -lrt"])
	
	# HIDAPI/libusb libs
	PKG_CHECK_MODULES([libusb], [libusb-1.0])
	LIBS_LIBUSB_PRIVATE+=" $libusb_LIBS"
	CFLAGS_LIBUSB+=" $libusb_CFLAGS"
	AC_CHECK_LIB([rt], [clock_gettime], [LIBS_LIBUSB_PRIVATE+=" -lrt"])
	AC_CHECK_LIB([pthread], [pthread_create], [LIBS_LIBUSB_PRIVATE+=" -lpthread"])
	;;
*-darwin*)
	AC_MSG_RESULT([ (Mac OS X back-end)])
	AC_DEFINE(OS_DARWIN, 1, [Mac implementation])
	AC_SUBST(OS_DARWIN)
	backend="darwin"
	LIBS="${LIBS} -framework IOKit -framework CoreFoundation"
	;;
*-freebsd*)
	AC_MSG_RESULT([ (FreeBSD back-end)])
	AC_DEFINE(OS_FREEBSD, 1, [FreeBSD implementation])
	AC_SUBST(OS_FREEBSD)
	backend="freebsd"

	CFLAGS="$CFLAGS -I/usr/local/include"
	LDFLAGS="$LDFLAGS -L/usr/local/lib"
	LIBS="${LIBS}"
	AC_CHECK_LIB([usb], [libusb_init], [LIBS_LIBUSB_PRIVATE="${LIBS_LIBUSB_PRIVATE} -lusb"], [hidapi_lib_error libusb])
	AC_CHECK_LIB([pthread], [pthread_create], [LIBS_LIBUSB_PRIVATE="${LIBS_LIBUSB_PRIVATE} -lpthread"], [hidapi_lib_error libpthread])
	AC_CHECK_LIB([iconv], [iconv_open], [LIBS_LIBUSB_PRIVATE="${LIBS_LIBUSB_PRIVATE} -liconv"], [hidapi_lib_error libiconv])
	echo libs_priv: $LIBS_LIBUSB_PRIVATE
	;;
*-mingw*)
	AC_MSG_RESULT([ (Windows back-end, using MinGW)])
	backend="windows"
	win_implementation="mingw"
	;;
*-cygwin*)
	AC_MSG_RESULT([ (Windows back-end, using Cygwin)])
	backend="windows"
	win_implementation="cygwin"
	;;
*)
	AC_MSG_ERROR([HIDAPI is not supported on your operating system yet])
esac

LIBS_HIDRAW="${LIBS} ${LIBS_HIDRAW_PR}"
LIBS_LIBUSB="${LIBS} ${LIBS_LIBUSB_PRIVATE}"
AC_SUBST([LIBS_HIDRAW])
AC_SUBST([LIBS_LIBUSB])
AC_SUBST([CFLAGS_LIBUSB])
AC_SUBST([CFLAGS_HIDRAW])

if test "x$backend" = xwindows; then
	AC_DEFINE(OS_WINDOWS, 1, [Windows implementations])
	AC_SUBST(OS_WINDOWS)
	LDFLAGS="${LDFLAGS} -no-undefined"
	LIBS="${LIBS} -lsetupapi"
fi


# Test GUI
AC_ARG_ENABLE([testgui],
	[AS_HELP_STRING([--enable-testgui],
		[enable building of test GUI (default n)])],
	[testgui_enabled=$enableval],
	[testgui_enabled='no'])
AM_CONDITIONAL([BUILD_TESTGUI], [test "x$testgui_enabled" != "xno"])

if test "x$testgui_enabled" != "xno"; then
	if test "x$backend" = xdarwin; then
		# On Mac OS, don't use pkg-config.
		LIBS_TESTGUI+=`/opt/local/bin/fox-config --libs`
		LIBS_TESTGUI+=" -framework Cocoa"
		CFLAGS_TESTGUI+=`/opt/local/bin/fox-config --cflags`
		OBJCFLAGS+=" -x objective-c++"
	elif test "x$backend" = xwindows; then
		# On Windows, just set the paths for Fox toolkit
		if test "x$win_implementation" = xmingw; then
			CFLAGS_TESTGUI="-I\$(srcdir)/../../hidapi-externals/fox/include -g -c"
			LIBS_TESTGUI=" -mwindows \$(srcdir)/../../hidapi-externals/fox/lib/libFOX-1.6.a -lgdi32 -Wl,--enable-auto-import -static-libgcc -static-libstdc++ -lkernel32"
		else
			# Cygwin
			CFLAGS_TESTGUI="-DWIN32 -I\$(srcdir)/../../hidapi-externals/fox/include -g -c"
			LIBS_TESTGUI="\$(srcdir)/../../hidapi-externals/fox/lib/libFOX-cygwin-1.6.a -lgdi32 -Wl,--enable-auto-import -static-libgcc -static-libstdc++ -lkernel32"
		fi
	else
		# On Linux and FreeBSD platforms, use pkg-config to find fox.
		PKG_CHECK_MODULES([fox], [fox])
		LIBS_TESTGUI="${LIBS_TESTGUI} $fox_LIBS"
		if test "x$backend" = xfreebsd; then
			LIBS_TESTGUI="${LIBS_TESTGUI} -L/usr/local/lib"
		fi
		CFLAGS_TESTGUI="${CFLAGS_TESTGUI} $fox_CFLAGS"
	fi
fi
AC_SUBST([LIBS_TESTGUI])
AC_SUBST([CFLAGS_TESTGUI])

# OS info for Automake
AM_CONDITIONAL(OS_LINUX, test "x$backend" = xlinux)
AM_CONDITIONAL(OS_DARWIN, test "x$backend" = xdarwin)
AM_CONDITIONAL(OS_FREEBSD, test "x$backend" = xfreebsd)
AM_CONDITIONAL(OS_WINDOWS, test "x$backend" = xwindows)

AC_CONFIG_HEADERS([config.h])

if test "x$backend" = "xlinux"; then
	AC_CONFIG_FILES([pc/hidapi-hidraw.pc])
	AC_CONFIG_FILES([pc/hidapi-libusb.pc])
else
	AC_CONFIG_FILES([pc/hidapi.pc])
fi

AC_CONFIG_FILES([Makefile linux/Makefile libusb/Makefile mac/Makefile windows/Makefile hidtest/Makefile testgui/Makefile])
AC_OUTPUT
