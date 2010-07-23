HIDAPI Test GUI

This test GUI application uses Fox Toolkit. On Windows, it can be downloaded
as part of the hidapi-externals.zip file located on the GitHub project page
where HIDAPI is downloaded:
	http://github.com/signal11/hidapi/downloads

On Windows:
	Copy ..\hidapi\objfre_wxp_x86\i386\hidapi.dll to this folder

On Linux:
	Install Fox-Toolkit from apt-get or yum:
		On Debian or Ubuntu:
			sudo apt-get install libfox-1.6-dev
		On Red Hat or Fedora:
			yum install libfox-1.6-devel

On Mac:
	Install Fox-Toolkit from ports:
		sudo port install fox

Once Fox-Toolkit is installed, Linux and Mac users can simply type 'make'.
On Windows, HIDAPI can be built from the .vcproj file in this directory.

Alan Ott
Signal 11 Software
2010-07-22
