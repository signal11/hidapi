/*******************************************************
 HIDAPI - Multi-Platform library for
 communication with HID devices.

 Alan Ott
 Signal 11 Software

 8/22/2009
 Linux Version - 6/2/2010
 Libusb Version - 8/13/2010

 Copyright 2009, All Rights Reserved.
 
 This software may be used by anyone for any reason so
 long as this copyright notice remains intact.
********************************************************/

/* C */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <errno.h>

/* Unix */
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include <pthread.h>

/* GNU / LibUSB */
#include "libusb.h"
#include "iconv.h"

#include "hidapi.h"

/* Linked List of input reports received from the device. */
struct input_report {
	uint8_t *data;
	size_t len;
	struct input_report *next;
};


struct hid_device_ {
	libusb_device_handle *device_handle;
	int input_endpoint;
	int output_endpoint;
	int input_ep_max_packet_size;
	int interface;
	int blocking; /* boolean */
	pthread_t thread;
	pthread_mutex_t mutex; /* Protects input_reports */
	pthread_cond_t condition;
	int shutdown_thread;
	struct libusb_transfer *transfer;
	struct input_report *input_reports;
};

static int initialized = 0;

hid_device *new_hid_device()
{
	hid_device *dev = calloc(1, sizeof(hid_device));
	dev->device_handle = NULL;
	dev->input_endpoint = 0;
	dev->output_endpoint = 0;
	dev->input_ep_max_packet_size = 0;
	dev->interface = 0;
	dev->blocking = 0;
	dev->shutdown_thread = 0;
	dev->transfer = NULL;
	dev->input_reports = NULL;
	
	pthread_mutex_init(&dev->mutex, NULL);
	pthread_cond_init(&dev->condition, NULL);
	
	return dev;
}

static void register_error(hid_device *device, const char *op)
{

}

/* Get the first language the device says it reports. This comes from
   USB string #0. */
static uint16_t get_first_language(libusb_device_handle *dev)
{
	uint16_t buf[32];
	int len;
	
	/* Get the string from libusb. */
	len = libusb_get_string_descriptor(dev,
			0x0, /* String ID */
			0x0, /* Language */
			(unsigned char*)buf,
			sizeof(buf));
	if (len < 4)
		return 0x0;
	
	return buf[1]; // First two bytes are len and descriptor type.
}

/* This function returns a newly allocated wide string containing the USB
   device string numbered by the index. The returned string must be freed
   by using free(). */
static wchar_t *get_usb_string(libusb_device_handle *dev, uint8_t index)
{
	char buf[512];
	int len;
	wchar_t *str = NULL;
	wchar_t wbuf[256];

	/* iconv variables */
	iconv_t ic;
	size_t inbytes;
	size_t outbytes;
	size_t res;
	char *inptr;
	char *outptr;

	uint16_t lang = get_first_language(dev);
	
	/* Get the string from libusb. */
	len = libusb_get_string_descriptor(dev,
			index,
			lang,
			(unsigned char*)buf,
			sizeof(buf));
	if (len < 0)
		return NULL;
	
	buf[sizeof(buf)-1] = '\0';
	
	if (len+1 < sizeof(buf))
		buf[len+1] = '\0';
	
	/* Initialize iconv. */
	ic = iconv_open("UTF-32", "UTF-16");
	if (ic < 0)
		return NULL;
	
	/* Convert to UTF-32 (wchar_t on glibc systems).
	   Skip the first character (2-bytes). */
	inptr = buf+2;
	inbytes = len-2;
	outptr = (char*) wbuf;
	outbytes = sizeof(wbuf);
	res = iconv(ic, &inptr, &inbytes, &outptr, &outbytes);
	if (res < 0)
		goto err;

	/* Write the terminating NULL. */
	wbuf[sizeof(wbuf)-1] = 0x00000000;
	if (outbytes >= sizeof(wbuf[0]))
		*((wchar_t*)outptr) = 0x00000000;
	
	/* Allocate and copy the string. */
	str = wcsdup(wbuf+1);

err:
	iconv_close(ic);
	
	return str;
}

static char *make_path(libusb_device *dev, int interface_number)
{
	char str[64];
	snprintf(str, sizeof(str), "%04x:%04x:%02x",
		libusb_get_bus_number(dev),
		libusb_get_device_address(dev),
		interface_number);
	str[sizeof(str)-1] = '\0';
	
	return strdup(str);
}

struct hid_device_info  HID_API_EXPORT *hid_enumerate(unsigned short vendor_id, unsigned short product_id)
{
	libusb_device **devs;
	libusb_device *dev;
	libusb_device_handle *handle;
	ssize_t num_devs;
	int i = 0;
	
	struct hid_device_info *root = NULL; // return object
	struct hid_device_info *cur_dev = NULL;
	
	setlocale(LC_ALL,"");
	
	if (!initialized) {
		libusb_init(NULL);
		initialized = 1;
	}
	
	num_devs = libusb_get_device_list(NULL, &devs);
	while ((dev = devs[i++]) != NULL) {
		struct libusb_device_descriptor desc;
		struct libusb_config_descriptor *conf_desc = NULL;
		int skip = 1;
		int j, k;
		int interface_num = 0;

		int res = libusb_get_device_descriptor(dev, &desc);
		unsigned short dev_vid = desc.idVendor;
		unsigned short dev_pid = desc.idProduct;
		
		/* HID's are defined at the interface level. */
		if (desc.bDeviceClass != LIBUSB_CLASS_PER_INTERFACE)
			continue;

		res = libusb_get_active_config_descriptor(dev, &conf_desc);
		if (res < 0)
			libusb_get_config_descriptor(dev, 0, &conf_desc);
		for (j = 0; j < conf_desc->bNumInterfaces; j++) {
			const struct libusb_interface *intf = &conf_desc->interface[j];
			for (k = 0; k < intf->num_altsetting; k++) {
				const struct libusb_interface_descriptor *intf_desc;
				intf_desc = &intf->altsetting[k];
				if (intf_desc->bInterfaceClass == LIBUSB_CLASS_HID) {
					interface_num = intf_desc->bInterfaceNumber;
					skip = 0;
				}
			}
			intf++;
		}
		libusb_free_config_descriptor(conf_desc);

		if (skip)
			continue;
			
		/* Check the VID/PID against the arguments */
		if ((vendor_id == 0x0 && product_id == 0x0) ||
		    (vendor_id == dev_vid && product_id == dev_pid)) {
			struct hid_device_info *tmp;

		    	/* VID/PID match. Create the record. */
			tmp = calloc(1, sizeof(struct hid_device_info));
			if (cur_dev) {
				cur_dev->next = tmp;
			}
			else {
				root = tmp;
			}
			cur_dev = tmp;
			
			/* Fill out the record */
			cur_dev->next = NULL;
			cur_dev->path = make_path(dev, interface_num);
			
			res = libusb_open(dev, &handle);


			if (res >= 0) {
			
				/* Serial Number */
				if (desc.iSerialNumber > 0)
					cur_dev->serial_number =
						get_usb_string(handle, desc.iSerialNumber);

				/* Manufacturer and Product strings */
				if (desc.iManufacturer > 0)
					cur_dev->manufacturer_string =
						get_usb_string(handle, desc.iManufacturer);
				if (desc.iProduct > 0)
					cur_dev->product_string =
						get_usb_string(handle, desc.iProduct);
				
				libusb_close(handle);
			}
			/* VID/PID */
			cur_dev->vendor_id = dev_vid;
			cur_dev->product_id = dev_pid;
		}
	}

	libusb_free_device_list(devs, 1);

	return root;
}

void  HID_API_EXPORT hid_free_enumeration(struct hid_device_info *devs)
{
	struct hid_device_info *d = devs;
	while (d) {
		struct hid_device_info *next = d->next;
		free(d->path);
		free(d->serial_number);
		free(d->manufacturer_string);
		free(d->product_string);
		free(d);
		d = next;
	}
}

hid_device * hid_open(unsigned short vendor_id, unsigned short product_id, wchar_t *serial_number)
{
	struct hid_device_info *devs, *cur_dev;
	const char *path_to_open = NULL;
	hid_device *handle = NULL;
	
	devs = hid_enumerate(vendor_id, product_id);
	cur_dev = devs;
	while (cur_dev) {
		if (cur_dev->vendor_id == vendor_id &&
		    cur_dev->product_id == product_id) {
			if (serial_number) {
				if (wcscmp(serial_number, cur_dev->serial_number) == 0) {
					path_to_open = cur_dev->path;
					break;
				}
			}
			else {
				path_to_open = cur_dev->path;
				break;
			}
		}
		cur_dev = cur_dev->next;
	}

	if (path_to_open) {
		/* Open the device */
		handle = hid_open_path(path_to_open);
	}

	hid_free_enumeration(devs);
	
	return handle;
}

static void read_callback(struct libusb_transfer *transfer)
{
	hid_device *dev = transfer->user_data;
	
	if (transfer->status == LIBUSB_TRANSFER_COMPLETED) {

		printf("Transfer Completed\n");

		struct input_report *rpt = malloc(sizeof(*rpt));
		rpt->data = malloc(transfer->actual_length);
		memcpy(rpt->data, transfer->buffer, transfer->actual_length);
		rpt->len = transfer->actual_length;
		rpt->next = NULL;

		pthread_mutex_lock(&dev->mutex);

		/* Attach the new report object to the end of the list. */
		if (dev->input_reports == NULL) {
			/* The list is empty. Put it at the root. */
			dev->input_reports = rpt;
			pthread_cond_signal(&dev->condition);
		}
		else {
			/* Find the end of the list and attach. */
			struct input_report *cur = dev->input_reports;
			while (cur->next != NULL)
				cur = cur->next;
			cur->next = rpt;
		}
		pthread_mutex_unlock(&dev->mutex);
	}
	else if (transfer->status == LIBUSB_TRANSFER_CANCELLED) {
		dev->shutdown_thread = 1;
		return;
	}
	else if (transfer->status == LIBUSB_TRANSFER_NO_DEVICE) {
		dev->shutdown_thread = 1;
		return;
	}
	else if (transfer->status == LIBUSB_TRANSFER_TIMED_OUT) {
		printf("Timeout (normal)\n");
	}
	else {
		printf("Unknown transfer code: %d\n", transfer->status);
	}
	
	/* Re-submit the transfer object. */
	libusb_submit_transfer(transfer);
}


static void *read_thread(void *param)
{
	hid_device *dev = param;
	unsigned char *buf;
	const size_t length = dev->input_ep_max_packet_size;

	/* Set up the transfer object. */
	buf = malloc(length);
	dev->transfer = libusb_alloc_transfer(0);
	libusb_fill_interrupt_transfer(dev->transfer,
		dev->device_handle,
		dev->input_endpoint,
		buf,
		length,
		read_callback,
		dev,
		5000/*timeout*/);
	
	/* Make the first submission. Further submissions are made
	   from inside read_callback() */
	libusb_submit_transfer(dev->transfer);
	
	/* Handle all the events. */
	while (!dev->shutdown_thread) {
		int res;
		struct timeval tv;

		tv.tv_sec = 0;
		tv.tv_usec = 100; //TODO: Fix this value.
		res = libusb_handle_events_timeout(NULL, &tv);
		if (res < 0) {
			/* There was an error. Break out of this loop. */
			break;
		}
	}

#if 0 // This is done in hid_close()
	/* Cleanup before returning */
	free(dev->transfer->buffer);
	libusb_free_transfer(dev->transfer);
#endif
	
	return NULL;
}


hid_device * HID_API_EXPORT hid_open_path(const char *path)
{
	hid_device *dev = NULL;

	dev = new_hid_device();

	libusb_device **devs;
	libusb_device *usb_dev;
	ssize_t num_devs;
	int res;
	int i = 0;
	int good_open = 0;
	
	setlocale(LC_ALL,"");
	
	if (!initialized) {
		libusb_init(NULL);
		initialized = 1;
	}
	
	num_devs = libusb_get_device_list(NULL, &devs);
	while ((usb_dev = devs[i++]) != NULL) {
		struct libusb_device_descriptor desc;
		struct libusb_config_descriptor *conf_desc = NULL;
		int i,j,k;
		libusb_get_device_descriptor(usb_dev, &desc);

		libusb_get_active_config_descriptor(usb_dev, &conf_desc);
		for (j = 0; j < conf_desc->bNumInterfaces; j++) {
			const struct libusb_interface *intf = &conf_desc->interface[j];
			for (k = 0; k < intf->num_altsetting; k++) {
				const struct libusb_interface_descriptor *intf_desc;
				intf_desc = &intf->altsetting[k];
				if (intf_desc->bInterfaceClass == LIBUSB_CLASS_HID) {
					char *dev_path = make_path(usb_dev, intf_desc->bInterfaceNumber);
					if (!strcmp(dev_path, path)) {
						/* Matched Paths. Open this device */
						printf("Opening this one \n");

						// OPEN HERE //
						res = libusb_open(usb_dev, &dev->device_handle);
						if (res < 0) {
							printf("can't open device\n");
 							break;
						}
						good_open = 1;
						res = libusb_detach_kernel_driver(dev->device_handle, intf_desc->bInterfaceNumber);
						if (res < 0) {
							printf("Unable to detach. Maybe this is OK\n");
						}
						
						res = libusb_claim_interface(dev->device_handle, intf_desc->bInterfaceNumber);
						if (res < 0) {
							printf("can't claim interface %d: %d\n", intf_desc->bInterfaceNumber, res);
							libusb_close(dev->device_handle);
							good_open = 0;
							break;
						}

						/* Store off the interface number */
						dev->interface = intf_desc->bInterfaceNumber;
												
						/* Find the INPUT and OUTPUT endpoints. An
						   OUTPUT endpoint is not required. */
						for (i = 0; i < intf_desc->bNumEndpoints; i++) {
							const struct libusb_endpoint_descriptor *ep
								= &intf_desc->endpoint[i];

							/* Determine the type and direction of this
							   endpoint. */
							int is_interrupt =
								(ep->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK)
							      == LIBUSB_TRANSFER_TYPE_INTERRUPT;
							int is_output = 
								(ep->bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK)
							      == LIBUSB_ENDPOINT_OUT;
							int is_input = 
								(ep->bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK)
							      == LIBUSB_ENDPOINT_IN;

							/* Decide whether to use it for intput or output. */
							if (dev->input_endpoint == 0 &&
							    is_interrupt && is_input) {
								/* Use this endpoint for INPUT */
								dev->input_endpoint = ep->bEndpointAddress;
								dev->input_ep_max_packet_size = ep->wMaxPacketSize;
							}
							if (dev->output_endpoint == 0 &&
							    is_interrupt && is_output) {
								/* Use this endpoint for OUTPUT */
								dev->output_endpoint = ep->bEndpointAddress;
							}
						}
						
						pthread_create(&dev->thread, NULL, read_thread, dev);
						
					}
					free(dev_path);
				}
			}
			intf++;
		}
		libusb_free_config_descriptor(conf_desc);

	}

	libusb_free_device_list(devs, 1);
	
	// If we have a good handle, return it.
	if (good_open) {
		return dev;
	}
	else {
		// Unable to open any devices.
		free(dev);
		return NULL;
	}
}


int HID_API_EXPORT hid_write(hid_device *dev, const unsigned char *data, size_t length)
{
	int res;
	int report_number = data[0];
	int skipped_report_id = 0;

	if (report_number == 0x0) {
		data++;
		length--;
		skipped_report_id = 1;
	}


	if (dev->output_endpoint <= 0) {
		/* No interrput out endpoint. Use the Control Endpoint */
		res = libusb_control_transfer(dev->device_handle,
			LIBUSB_REQUEST_TYPE_CLASS|LIBUSB_RECIPIENT_INTERFACE|LIBUSB_ENDPOINT_OUT,
			0x09/*HID Set_Report*/,
			(2/*HID output*/ << 8) | report_number,
			dev->interface,
			(unsigned char *)data, length,
			1000/*timeout millis*/);
		
		if (res < 0)
			return -1;
		
		if (skipped_report_id)
			length++;
		
		return length;
	}
	else {
		/* Use the interrupt out endpoint */
		int actual_length;
		res = libusb_interrupt_transfer(dev->device_handle,
			dev->output_endpoint,
			(unsigned char*)data,
			length,
			&actual_length, 1000);
		
		if (res < 0)
			return -1;
		
		if (skipped_report_id)
			actual_length++;
		
		return actual_length;
	}
}

/* Helper function, to simplify hid_read().
   This should be called with dev->mutex locked. */
static int return_data(hid_device *dev, unsigned char *data, size_t length)
{
	/* Copy the data out of the linked list item (rpt) into the
	   return buffer (data), and delete the liked list item. */
	struct input_report *rpt = dev->input_reports;
	size_t len = (length < rpt->len)? length: rpt->len;
	memcpy(data, rpt->data, len);
	dev->input_reports = rpt->next;
	free(rpt->data);
	free(rpt);
	return len;
}


int HID_API_EXPORT hid_read(hid_device *dev, unsigned char *data, size_t length)
{
	int bytes_read = -1;

#if 0
	int transferred;
	int res = libusb_interrupt_transfer(dev->device_handle, dev->input_endpoint, data, length, &transferred, 5000);
	printf("transferred: %d\n", transferred);
	return transferred;
#endif

	pthread_mutex_lock(&dev->mutex);

	/* There's an input report queued up. Return it. */
	if (dev->input_reports) {
		/* Return the first one */
		bytes_read = return_data(dev, data, length);
		goto ret;
	}
	
	if (dev->blocking) {
		pthread_cond_wait(&dev->condition, &dev->mutex);
		bytes_read = return_data(dev, data, length);
	}
	else {
		bytes_read = 0;
	}

ret:
	pthread_mutex_unlock(&dev->mutex);

	return bytes_read;
}

int HID_API_EXPORT hid_set_nonblocking(hid_device *dev, int nonblock)
{
	dev->blocking = !nonblock;
	
	return -1;
}


int HID_API_EXPORT hid_send_feature_report(hid_device *dev, const unsigned char *data, size_t length)
{
	int res = -1;
	int skipped_report_id = 0;
	int report_number = data[0];

	if (report_number == 0x0) {
		data++;
		length--;
		skipped_report_id = 1;
	}

	res = libusb_control_transfer(dev->device_handle,
		LIBUSB_REQUEST_TYPE_CLASS|LIBUSB_RECIPIENT_INTERFACE|LIBUSB_ENDPOINT_OUT,
		0x09/*HID set_report*/,
		(3/*HID feature*/ << 8) | report_number,
		dev->interface,
		(unsigned char *)data, length,
		1000/*timeout millis*/);
	
	if (res < 0)
		return -1;
	
	/* Account for the report ID */
	if (skipped_report_id)
		length++;
	
	return length;
}

int HID_API_EXPORT hid_get_feature_report(hid_device *dev, unsigned char *data, size_t length)
{
	int res = -1;
	int skipped_report_id = 0;
	int report_number = data[0];

	if (report_number == 0x0) {
		/* Offset the return buffer by 1, so that the report ID
		   will remain in byte 0. */
		data++;
		length--;
		skipped_report_id = 1;
	}
	res = libusb_control_transfer(dev->device_handle,
		LIBUSB_REQUEST_TYPE_CLASS|LIBUSB_RECIPIENT_INTERFACE|LIBUSB_ENDPOINT_IN,
		0x01/*HID get_report*/,
		(3/*HID feature*/ << 8) | report_number,
		dev->interface,
		(unsigned char *)data, length,
		1000/*timeout millis*/);
	
	if (res < 0)
		return -1;

	if (skipped_report_id)
		res++;
	
	return res;
}


void HID_API_EXPORT hid_close(hid_device *dev)
{
	if (!dev)
		return;
	
	/* Cause read_thread() to stop. */
	libusb_cancel_transfer(dev->transfer);

	/* Wait for read_thread() to end. */
	pthread_join(dev->thread, NULL);
	
	/* Close the handle */
	libusb_close(dev->device_handle);
	
	/* Clean up the thread objects */
	pthread_mutex_destroy(&dev->mutex);
	pthread_cond_destroy(&dev->condition);
	
	free(dev);
}


int HID_API_EXPORT_CALL hid_get_manufacturer_string(hid_device *dev, wchar_t *string, size_t maxlen)
{
	return -1;
}

int HID_API_EXPORT_CALL hid_get_product_string(hid_device *dev, wchar_t *string, size_t maxlen)
{
	return -1;
}

int HID_API_EXPORT_CALL hid_get_serial_number_string(hid_device *dev, wchar_t *string, size_t maxlen)
{
	return -1;
}

int HID_API_EXPORT_CALL hid_get_indexed_string(hid_device *dev, int string_index, wchar_t *string, size_t maxlen)
{
	return -1;
}


HID_API_EXPORT wchar_t * HID_API_CALL  hid_error(hid_device *dev)
{
	return NULL;
}
