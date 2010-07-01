/*******************************************************
 Windows HID simplification

 Alan Ott
 Signal 11 Software

 8/22/2009
 Linux Version - 6/2/2009

 Copyright 2009, All Rights Reserved.
 
 This software may be used by anyone for any reason so
 long as this copyright notice remains intact.
********************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "/usr/include/libudev.h"

#include "hidapi.h"

struct Device {
		int valid;
		int device_handle;
		int blocking;
};


#define MAX_DEVICES 64
static struct Device devices[MAX_DEVICES];
static int devices_initialized = 0;

static void register_error(struct Device *device, const char *op)
{

}

struct hid_device  HID_API_EXPORT *hid_enumerate(unsigned short vendor_id, unsigned short product_id)
{
	struct udev *udev;
	struct udev_enumerate *enumerate;
	struct udev_list_entry *devices, *dev_list_entry;
	struct udev_device *dev;
	
	struct hid_device *root = NULL; // return object
	struct hid_device *cur_dev = NULL;

	/* Create the udev object */
	udev = udev_new();
	if (!udev) {
		printf("Can't create udev\n");
		exit(1);
	}

	/* Create a list of the devices in the 'hidraw' subsystem. */
	enumerate = udev_enumerate_new(udev);
	udev_enumerate_add_match_subsystem(enumerate, "hidraw");
	udev_enumerate_scan_devices(enumerate);
	devices = udev_enumerate_get_list_entry(enumerate);
	/* For each item enumerated, print out its information.
	   udev_list_entry_foreach is a macro which expands to
	   a loop. The loop will be executed for each member in
	   devices, setting dev_list_entry to a list entry
	   which contains the device's path in /sys. */
	udev_list_entry_foreach(dev_list_entry, devices) {
		const char *path;
		const char *str;
		size_t len;
		struct hid_device *tmp;
		
		/* Get the filename of the /sys entry for the device
		   and create a udev_device object (dev) representing it */
		path = udev_list_entry_get_name(dev_list_entry);
		dev = udev_device_new_from_syspath(udev, path);

		/* usb_device_get_devnode() returns the path to the device node
		   itself in /dev. */
		printf("Device Node Path: %s\n", udev_device_get_devnode(dev));
		
		tmp = malloc(sizeof(struct hid_device));
		if (cur_dev) {
			cur_dev->next = tmp;
		}
		else {
			root = tmp;
		}
		cur_dev = tmp;
		
		cur_dev->next = NULL;
		str = udev_device_get_devnode(dev);
		if (str) {
			len = strlen(str);
			cur_dev->path = calloc(len+1, sizeof(char));
			strncpy(cur_dev->path, str, len+1);
			cur_dev->path[len] = '\0';
		}
		else
			cur_dev->path = NULL;

		/* The device pointed to by dev contains information about
		   the hidraw device. In order to get information about the
		   USB device, get the parent device with the
		   subsystem/devtype pair of "usb"/"usb_device". This will
		   be several levels up the tree, but the function will find
		   it.*/
		dev = udev_device_get_parent_with_subsystem_devtype(
		       dev,
		       "usb",
		       "usb_device");
		if (!dev) {
			printf("Unable to find parent usb device.");
			exit(1);
		}

		str = udev_device_get_sysattr_value(dev,"idVendor");
		cur_dev->vendor_id = (str)? strtol(str, NULL, 16): 0x0;
		
		str = udev_device_get_sysattr_value(dev, "idProduct");
		cur_dev->product_id = (str)? strtol(str, NULL, 16): 0x0;


		/* From here, we can call get_sysattr_value() for each file
		   in the device's /sys entry. The strings passed into these
		   functions (idProduct, idVendor, serial, etc.) correspond
		   directly to the files in the /sys directory which
		   represents the USB device. Note that USB strings are
		   Unicode, UCS2 encoded, but the strings returned from
		   udev_device_get_sysattr_value() are UTF-8 encoded. */
		printf("  VID/PID: %s %s\n",
			udev_device_get_sysattr_value(dev,"idVendor"),
			udev_device_get_sysattr_value(dev, "idProduct"));
		printf("  %s\n  %s\n",
			udev_device_get_sysattr_value(dev,"manufacturer"),
			udev_device_get_sysattr_value(dev,"product"));
		printf("  serial: %s\n",
			 udev_device_get_sysattr_value(dev, "serial"));
		udev_device_unref(dev);
	}
	/* Free the enumerator object */
	udev_enumerate_unref(enumerate);

	udev_unref(udev);
	
	return root;
	
}

void  HID_API_EXPORT hid_free_enumeration(struct hid_device *devs)
{

}

int HID_API_EXPORT hid_open(unsigned short vendor_id, unsigned short product_id, wchar_t *serial_number)
{
  	int i;
	int handle = -1;
	struct Device *dev = NULL;

	// Initialize the Device array if it hasn't been done.
	if (!devices_initialized) {
		int i;
		for (i = 0; i < MAX_DEVICES; i++) {
			devices[i].valid = 0;
			devices[i].device_handle = -1;
			devices[i].blocking = 1;
		}
		devices_initialized = 1;
	}

	// Find an available handle to use;
	for (i = 0; i < MAX_DEVICES; i++) {
		if (!devices[i].valid) {
			devices[i].valid = 1;
			handle = i;
			dev = &devices[i];
			break;
		}
	}

	if (handle < 0) {
		return -1;
	}

	// If we have a good handle, return it.
	if (dev->device_handle > 0) {
		return handle;
	}
	else {
		// Unable to open any devices.
		dev->valid = 0;
		return -1;
	}
}

int HID_API_EXPORT hid_write(int device, const unsigned char *data, size_t length)
{
	struct Device *dev = NULL;
	int bytes_written;
	int res;

	// Get the handle 
	if (device < 0 || device >= MAX_DEVICES)
		return -1;
	if (devices[device].valid == 0)
		return -1;
	dev = &devices[device];

	return bytes_written;
}


int HID_API_EXPORT hid_read(int device, unsigned char *data, size_t length)
{
	struct Device *dev = NULL;
	int bytes_read;
	int res;

	// Get the handle 
	if (device < 0 || device >= MAX_DEVICES)
		return -1;
	if (devices[device].valid == 0)
		return -1;
	dev = &devices[device];

	return bytes_read;
}

int HID_API_EXPORT hid_set_nonblocking(int device, int nonblock)
{
	struct Device *dev = NULL;

	// Get the handle 
	if (device < 0 || device >= MAX_DEVICES)
		return -1;
	if (devices[device].valid == 0)
		return -1;
	dev = &devices[device];
	
	dev->blocking = !nonblock;
	return 0; /* Success */
}

void HID_API_EXPORT hid_close(int device)
{
	struct Device *dev = NULL;

	// Get the handle 
	if (device < 0 || device >= MAX_DEVICES)
		return;
	if (devices[device].valid == 0)
		return;
	dev = &devices[device];

	dev->valid = 0;
}

int HID_API_EXPORT_CALL hid_get_manufacturer_string(int device, wchar_t *string, size_t maxlen)
{
	struct Device *dev = NULL;
	int res;

	// Get the handle 
	if (device < 0 || device >= MAX_DEVICES)
		return -1;
	if (devices[device].valid == 0)
		return -1;
	dev = &devices[device];


	return 0;
}

int HID_API_EXPORT_CALL hid_get_product_string(int device, wchar_t *string, size_t maxlen)
{
	struct Device *dev = NULL;
	int res;

	// Get the handle 
	if (device < 0 || device >= MAX_DEVICES)
		return -1;
	if (devices[device].valid == 0)
		return -1;
	dev = &devices[device];


	return 0;
}

int HID_API_EXPORT_CALL hid_get_serial_number_string(int device, wchar_t *string, size_t maxlen)
{
	struct Device *dev = NULL;
	int res;

	// Get the handle 
	if (device < 0 || device >= MAX_DEVICES)
		return -1;
	if (devices[device].valid == 0)
		return -1;
	dev = &devices[device];

	return 0;
}

int HID_API_EXPORT_CALL hid_get_indexed_string(int device, int string_index, wchar_t *string, size_t maxlen)
{
	struct Device *dev = NULL;
	int res;

	// Get the handle 
	if (device < 0 || device >= MAX_DEVICES)
		return -1;
	if (devices[device].valid == 0)
		return -1;
	dev = &devices[device];


	return 0;
}


HID_API_EXPORT const char * HID_API_CALL  hid_error(int device)
{
	struct Device *dev = NULL;

	// Get the handle 
	if (device < 0 || device >= MAX_DEVICES)
		return NULL;
	if (devices[device].valid == 0)
		return NULL;
	dev = &devices[device];

	return NULL;
}
