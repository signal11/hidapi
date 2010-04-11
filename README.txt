HID API for Windows

About
------

HIDAPI is a library used to make it easier for an application connect to USB HID-Class devices on Windows. It is a single DLL (hidapi.dll) with a single header file (hidapi.h) which can be linked into any application or DLL. Its purpose is two-fold. The first is that it simplifies the Windows HID interface by wrapping it with a straight-forward API that handles the most common tasks a developer would want to do with a HID device. The second is that it allows a developer to work directly with a HID device without having to worry about the Windows Driver Kit (WDK).

Why is this Necessary?
-----------------------
Microsoft provides a library called hid.dll to access HID-Class Devices. Hid.dll contains a handful of functions which are useful in talking to HID-Class devices from within an application (for more information, search for HidD_GetAttributes() on MSDN). Unfortunately, the headers and import library for hid.dll are not included in the Platform SDK, but in the Windows Driver Kit (WDK). In the past it was sufficient to download the WDK and then set the include and linker paths in Visual Studio to include the WDK's include/ and lib/ paths, but unfortunately now with WDK version 7, this is impossible, as the header files in the WDK are not compatible with those that ship with Visual Studio and the Platform SDK.

While it is possible to tell Visual Studio to ignore the standard include paths and then add the WDK include paths, this does not promote best practices and in many cases will be incompatible with larger projects which rely on Visual Studio being configured in a more default manner.

HIDAPI also provides a cleaner interface to the HID device, making it easier to develop applications which communicate with USB HID devices without having to know the details of the Microsoft HID API.

How does it work then?
-----------------------
hidapi.dll is built using the WDK, using a Makefile as is standard practice when using the WDK. hidapi.dll links dynamically with hid.dll, and as a result, your application only has to link with hidapi.dll and not Microsoft's hid.dll. Applications using hidapi.dll can be built with Visual Studio in its default configuration.

What Does the API Look Like?
-----------------------------
The API wraps the functionality of the most commonly used of the hid.dll functions. The sample program, which communicates with the USB Generic HID sample which is part of the Microchip Application Library (in folder "Microchip Solutions\USB Device - HID - Custom Demos\Generic HID - Firmware" when the Microchip Application Framework is installed), looks like this (with error checking removed for simplicity):

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "hidapi.h"

#define MAX_STR 255

int main(int argc, char* argv[])
{
	int res;
	unsigned char buf[65];
	wchar_t wstr[MAX_STR];
	int handle;
	int i;

	// Open the device using the VID, PID,
	// and optionally the Serial number.
	handle = hid_open(0x4d8, 0x3f, NULL);

	// Read the Manufacturer String
	res = hid_get_manufacturer_string(handle, wstr, MAX_STR);
	wprintf(L"Manufacturer String: %s\n", wstr);

	// Read the Product String
	res = hid_get_product_string(handle, wstr, MAX_STR);
	wprintf(L"Product String: %s\n", wstr);

	// Read the Serial Number String
	res = hid_get_serial_number_string(handle, wstr, MAX_STR);
	wprintf(L"Serial Number String: (%d) %s\n", wstr[0], wstr);

	// Read Indexed String 1
	res = hid_get_indexed_string(handle, 1, wstr, MAX_STR);
	wprintf(L"Indexed String 1: %s\n", wstr);

	// Toggle LED (cmd 0x80). The first byte is the report number (0x0).
	buf[0] = 0x0;
	buf[1] = 0x80;
	res = hid_write(handle, buf, 65);

	// Request state (cmd 0x81). The first byte is the report number (0x0).
	buf[0] = 0x0;
	buf[1] = 0x81;
	res = hid_write(handle, buf, 65);

	// Read requested state
	hid_read(handle, buf, 65);

	// Print out the returned buffer.
	for (i = 0; i < 4; i++)
		printf("buf[%d]: %d\n", i, buf[i]);

	return 0;
}

License
--------
HIDAPI may be used by anyone for any reason so long as the coypright header in the source code remains intact.

Download
---------
It can be downloaded from github
	git clone git://github.com/signal11/hidapi.git

Build Instructions
-------------------
The zip file contains a pre-built copy of hidapi.dll, so it is not even necessary to download the Windows Driver Kit unless you wish to re-build hidapi.dll itself. Building an application using hidapi.dll can be done without the WDK.

Instructions for re-building hidapi.dll:

   1. Install the Windows Driver Kit (WDK) from Microsoft.
   2. From the Start menu, in the Windows Driver Kits folder, select Build Environments, then your operating system, then the x86 Free Build Environment (or one that is appropriate for your system).
   3. From the console, change directory to the hidapi directory, which is part of the HIDAPI distribution.
   4. Type build.
   5. You can find the output files (DLL and LIB) in a subdirectory created by the build system which is appropriate for your environment. On Windows XP, this directory is objfre_wxp_x86/i386.

To build the sample application:

   1. load one of the solution files in the hidtest directory. hidtest.sln is for Visual Studio 2008, and hidtest-vc8.vcproj is for visual studio 2005.

--------------------------------

Signal 11 Software - 2010-04-11

