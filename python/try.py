
"""
This module performs a simple test of the Python hid extension.
"""

import hid
import time

print "Attempting to enumerate USB devices"
for d in hid.enumerate(0, 0):
    keys = d.keys()
    keys.sort()
    for key in keys:
        print "%s : %s" % (key, d[key])
    print ""
print "Completed enumerating USB devices"

# Below is an demonstration of how you might use the hid extension.
#
#try:
#    print "Opening device"
#    h = hid.device(0x461, 0x20)
#    #h = hid.device(0x1941, 0x8021) # Fine Offset USB Weather Station
#
#    print "Manufacturer: %s" % h.get_manufacturer_string()
#    print "Product: %s" % h.get_product_string()
#    print "Serial No: %s" % h.get_serial_number_string()
#
#    # try non-blocking mode by uncommenting the next line
#    #h.set_nonblocking(1)
#
#    # try writing some data to the device
#    for k in range(10):
#        for i in [0, 1]:
#            for j in [0, 1]:
#                h.write([0x80, i, j])
#                d = h.read(5)
#                if d:
#                    print d
#                time.sleep(0.05)
#
#    print "Closing device"
#    h.close()
#
#except IOError, ex:
#    print ex
#    print "You probably don't have the hard coded test hid. Update the hid.device line"
#    print "in this script with one from the enumeration list output above and try again."

print "Done"




