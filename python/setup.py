from distutils.core import setup
from distutils.extension import Extension
from Cython.Distutils import build_ext
import os
os.environ['CFLAGS'] = "-I../hidapi -I/usr/include/libusb-1.0"
os.environ['LDFLAGS'] = "-L/usr/lib/i386-linux-gnu -lusb-1.0 -ludev -lrt"

setup(
    cmdclass = {'build_ext': build_ext},
    ext_modules = [Extension("hid", ["hid.pyx", "../libusb/hid.c"])]
)
