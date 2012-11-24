A hid Python extension module
=============================

.. contents::

Description
-----------

This directory contains files that provide the ability to create a hid
extension module for Python on OSX, Linux and windows platforms. The 
extension module is implemented using Cython to wrap the hid C object. 
Cython provides a convenient and easy method for creating extensions
for the Python programming lanuage.

This work is based on cython-hidapi interface to hidapi originally 
developed by gbishop (https://github.com/gbishop/cython-hidapi).


This extension has previously been tested with:

* the PIC18F4550 on the development board from CCS with their example program. 
* the Fine Offset WH3081 Weather Station.

This extension does not make use of the hidraw driver implementation.


Software Dependencies
---------------------

* hidapi (https://github.com/signal11/hidapi)
* Python (http://python.org/download/)
* Cython (http://cython.org/#download)


Instructions
------------

1. Download hidapi archive::

    $ git clone https://github.com/signal11/hidapi.git
    $ cd python

2. Build the hid Python extension module for your platform::

    $ python setup[-mac|-windows].py build

3. Install cython-hidapi module into your Python distribution::
  
    $ [sudo] python setup[-mac|-windows].py install
    
3. Test install::

    $ python
    >>> import hid
    >>>
    
5. Try example script::

    $ python try.py

