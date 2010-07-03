
The Linux version of HIDAPI is designed to be dropped directly into your
application.  This is in contrast to the Windows version, which must
currently be built into a DLL.

See the Makefile in this directory, which builds a test program doing
exactly that.

libudev headers and libraries are required to build hidapi programs under
Linux.  To install libudev libraries on Ubuntu, and other Debian-based
systems, run:
	sudo apt-get install libudev-dev

On Redhat-based systems, run the following as root:
	yum install libudev-devel

Unfortunately, the hidraw driver, which the linux version of hidapi is based
on, contains bugs in kernel versions < 2.6.36, which the client application
should be aware of.

Alan.

Bugs:
On Kernel versions < 2.6.34, if your device uses numbered reports, an extra
byte will be returned at the beginning of all reports returned from read()
for hidraw devices. This is worked around in the libary. No action should be
necessary in the client library.

On Kernel versions < 2.6.35, reports will only be sent using a Set_Report
transfer on the CONTROL endpoint. No data will ever be sent on an Interrupt
Out endpoint if one exists. This is fixed in 2.6.35. In 2.6.35, OUTPUT
reports will be sent to the device on the first INTERRUPT OUT endpoint if it
exists; If it does not exist, OUTPUT reports will be sent on the CONTROL
endpoint.

On Kernel versions < 2.6.36, add an extra byte containing the report number
to sent reports if numbered reports are used, and the device does not
contain an INTERRPUT OUT endpoint for OUTPUT transfers.  For example, if
your device uses numbered reports and wants to send {0x2 0xff 0xff 0xff} to
the device (0x2 is the report number), you must send {0x2 0x2 0xff 0xff
0xff}. If your device has the optional Interrupt OUT endpoint, this does not
apply (but really on 2.6.35 only, because 2.6.34 won't use the interrupt
out endpoint).
