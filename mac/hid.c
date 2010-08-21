/*******************************************************
 HIDAPI - Multi-Platform library for
 communication with HID devices.
 
 Alan Ott
 Signal 11 Software

 2010-07-03

 Copyright 2010, All Rights Reserved.
 
 This software may be used by anyone for any reason so
 long as this copyright notice remains intact.
********************************************************/


#include <IOKit/hid/IOHIDManager.h>
#include <IOKit/hid/IOHIDKeys.h>
#include <wchar.h>
#include <locale.h>
#include <pthread.h>

#include "hidapi.h"

/* Linked List of input reports received from the device. */
struct input_report {
	uint8_t *data;
	size_t len;
	struct input_report *next;
};

struct hid_device_ {
	IOHIDDeviceRef device_handle;
	int blocking;
	int uses_numbered_reports;
	CFStringRef run_loop_mode;
	uint8_t *input_report_buf;
	struct input_report *input_reports;
	pthread_mutex_t mutex;
};


static hid_device *new_hid_device()
{
	hid_device *dev = calloc(1, sizeof(hid_device));
	dev->device_handle = NULL;
	dev->blocking = 1;
	dev->uses_numbered_reports = 0;
	dev->input_reports = NULL;
	
	return dev;
}

static 	IOHIDManagerRef hid_mgr = 0x0;


static void register_error(hid_device *device, const char *op)
{

}


static long get_long_property(IOHIDDeviceRef device, CFStringRef key)
{
	CFTypeRef ref;
	long value;
	
	ref = IOHIDDeviceGetProperty(device, key);
	if (ref) {
		if (CFGetTypeID(ref) == CFNumberGetTypeID()) {
			CFNumberGetValue((CFNumberRef) ref, kCFNumberSInt32Type, &value);
			return value;
		}
	}
	return 0;
}

static unsigned short get_vendor_id(IOHIDDeviceRef device)
{
	return get_long_property(device, CFSTR(kIOHIDVendorIDKey));
}

static unsigned short get_product_id(IOHIDDeviceRef device)
{
	return get_long_property(device, CFSTR(kIOHIDProductIDKey));
}

static long get_location_id(IOHIDDeviceRef device)
{
	return get_long_property(device, CFSTR(kIOHIDLocationIDKey));
}

static long get_usage(IOHIDDeviceRef device)
{
	long res;
	res = get_long_property(device, CFSTR(kIOHIDDeviceUsageKey));
	if (!res)
		res = get_long_property(device, CFSTR(kIOHIDPrimaryUsageKey));
	return res;
}

static long get_usage_page(IOHIDDeviceRef device)
{
	long res;
	res = get_long_property(device, CFSTR(kIOHIDDeviceUsagePageKey));
	if (!res)
		res = get_long_property(device, CFSTR(kIOHIDPrimaryUsagePageKey));
	return res;
}

static long get_max_report_length(IOHIDDeviceRef device)
{
	return get_long_property(device, CFSTR(kIOHIDMaxInputReportSizeKey));
}

static int get_string_property(IOHIDDeviceRef device, CFStringRef prop, wchar_t *buf, size_t len)
{
	CFStringRef str = IOHIDDeviceGetProperty(device, prop);

	buf[0] = 0x0000;

	if (str) {
		CFRange range;
		range.location = 0;
		range.length = len;
		CFIndex used_buf_len;
		CFStringGetBytes(str,
			range,
			kCFStringEncodingUTF32LE,
			(char)'?',
			FALSE,
			(UInt8*)buf,
			len,
			&used_buf_len);
		return used_buf_len;
	}
	else
		return 0;
		
}

static int get_string_property_utf8(IOHIDDeviceRef device, CFStringRef prop, char *buf, size_t len)
{
	CFStringRef str = IOHIDDeviceGetProperty(device, prop);

	buf[0] = 0x0000;

	if (str) {
		CFRange range;
		range.location = 0;
		range.length = len;
		CFIndex used_buf_len;
		CFStringGetBytes(str,
			range,
			kCFStringEncodingUTF8,
			(char)'?',
			FALSE,
			(UInt8*)buf,
			len,
			&used_buf_len);
		return used_buf_len;
	}
	else
		return 0;
		
}


static int get_serial_number(IOHIDDeviceRef device, wchar_t *buf, size_t len)
{
	return get_string_property(device, CFSTR(kIOHIDSerialNumberKey), buf, len);
}

static int get_manufacturer_string(IOHIDDeviceRef device, wchar_t *buf, size_t len)
{
	return get_string_property(device, CFSTR(kIOHIDManufacturerKey), buf, len);
}

static int get_product_string(IOHIDDeviceRef device, wchar_t *buf, size_t len)
{
	return get_string_property(device, CFSTR(kIOHIDProductKey), buf, len);
}



static int get_transport(IOHIDDeviceRef device, wchar_t *buf, size_t len)
{
	return get_string_property(device, CFSTR(kIOHIDTransportKey), buf, len);
}

/* Implementation of wcsdup() for Mac. */
static wchar_t *dup_wcs(const wchar_t *s)
{
	size_t len = wcslen(s);
	wchar_t *ret = malloc((len+1)*sizeof(wchar_t));
	wcscpy(ret, s);

	return ret;
}


static int make_path(IOHIDDeviceRef device, char *buf, size_t len)
{
	int res;
	unsigned short vid, pid;
	char transport[32];

	buf[0] = '\0';

	res = get_string_property_utf8(
		device, CFSTR(kIOHIDTransportKey),
		transport, sizeof(transport));
	
	if (!res)
		return -1;

	vid = get_vendor_id(device);
	pid = get_product_id(device);

	res = snprintf(buf, len, "%s_%04hx_%04hx_%p",
	                   transport, vid, pid, device);
	
	
	buf[len-1] = '\0';
	return res+1;
}

static void init_hid_manager()
{
	/* Initialize all the HID Manager Objects */
	hid_mgr = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
	IOHIDManagerSetDeviceMatching(hid_mgr, NULL);
	IOHIDManagerScheduleWithRunLoop(hid_mgr, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
	IOHIDManagerOpen(hid_mgr, kIOHIDOptionsTypeNone);
}


struct hid_device_info  HID_API_EXPORT *hid_enumerate(unsigned short vendor_id, unsigned short product_id)
{
	struct hid_device_info *root = NULL; // return object
	struct hid_device_info *cur_dev = NULL;
	CFIndex num_devices;
	int i;
	
	setlocale(LC_ALL,"");

	/* Set up the HID Manager if it hasn't been done */
	if (!hid_mgr)
		init_hid_manager();
	
	/* Get a list of the Devices */
	CFSetRef device_set = IOHIDManagerCopyDevices(hid_mgr);

	/* Convert the list into a C array so we can iterate easily. */	
	num_devices = CFSetGetCount(device_set);
	IOHIDDeviceRef *device_array = calloc(num_devices, sizeof(IOHIDDeviceRef));
	CFSetGetValues(device_set, (const void **) device_array);

	/* Iterate over each device, making an entry for it. */	
	for (i = 0; i < num_devices; i++) {
		unsigned short dev_vid;
		unsigned short dev_pid;
		#define BUF_LEN 256
		wchar_t buf[BUF_LEN];
		char cbuf[BUF_LEN];

		IOHIDDeviceRef dev = device_array[i];

		dev_vid = get_vendor_id(dev);
		dev_pid = get_product_id(dev);

		/* Check the VID/PID against the arguments */
		if ((vendor_id == 0x0 && product_id == 0x0) ||
		    (vendor_id == dev_vid && product_id == dev_pid)) {
			struct hid_device_info *tmp;
			size_t len;

		    	/* VID/PID match. Create the record. */
			tmp = malloc(sizeof(struct hid_device_info));
			if (cur_dev) {
				cur_dev->next = tmp;
			}
			else {
				root = tmp;
			}
			cur_dev = tmp;
			
			/* Fill out the record */
			cur_dev->next = NULL;
			len = make_path(dev, cbuf, sizeof(cbuf));
			cur_dev->path = strdup(cbuf);

			/* Serial Number */
			get_serial_number(dev, buf, BUF_LEN);
			cur_dev->serial_number = dup_wcs(buf);

			/* Manufacturer and Product strings */
			get_manufacturer_string(dev, buf, BUF_LEN);
			cur_dev->manufacturer_string = dup_wcs(buf);
			get_product_string(dev, buf, BUF_LEN);
			cur_dev->product_string = dup_wcs(buf);
			
			/* VID/PID */
			cur_dev->vendor_id = dev_vid;
			cur_dev->product_id = dev_pid;
		}
	}
	
	free(device_array);
	CFRelease(device_set);
	
	return root;
}

void  HID_API_EXPORT hid_free_enumeration(struct hid_device_info *devs)
{
	/* This function is identical to the Linux version. Platform independent. */
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

hid_device * HID_API_EXPORT hid_open(unsigned short vendor_id, unsigned short product_id, wchar_t *serial_number)
{
	/* This function is identical to the Linux version. Platform independent. */
	struct hid_device_info *devs, *cur_dev;
	const char *path_to_open = NULL;
	hid_device * handle = NULL;
	
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

/* The Run Loop calls this function for each input report received.
   This function puts the data into a linked list to be picked up by
   hid_read(). */
void hid_report_callback(void *context, IOReturn result, void *sender,
                         IOHIDReportType report_type, uint32_t report_id,
                         uint8_t *report, CFIndex report_length)
{
	struct input_report *rpt;
	hid_device *dev = context;
	
	/* Make a new Input Report object */
	rpt = calloc(1, sizeof(struct input_report));
	rpt->data = calloc(1, report_length);
	memcpy(rpt->data, report, report_length);
	rpt->len = report_length;
	rpt->next = NULL;
	
	/* Attach the new report object to the end of the list. */
	if (dev->input_reports == NULL) {
		/* The list is empty. Put it at the root. */
		dev->input_reports = rpt;
	}
	else {
		/* Find the end of the list and attach. */
		struct input_report *cur = dev->input_reports;
		while (cur->next != NULL)
			cur = cur->next;
		cur->next = rpt;
	}
	
	/* Stop the Run Loop. This is mostly used for when blocking is
	   enabled, but it doesn't hurt for non-blocking as well.  */
	CFRunLoopStop(CFRunLoopGetCurrent());
}

hid_device * HID_API_EXPORT hid_open_path(const char *path)
{
  	int i;
	hid_device *dev = NULL;
	CFIndex num_devices;
	
	dev = new_hid_device();

	/* Set up the HID Manager if it hasn't been done */
	if (!hid_mgr)
		init_hid_manager();

	CFSetRef device_set = IOHIDManagerCopyDevices(hid_mgr);
	
	num_devices = CFSetGetCount(device_set);
	IOHIDDeviceRef *device_array = calloc(num_devices, sizeof(IOHIDDeviceRef));
	CFSetGetValues(device_set, (const void **) device_array);	
	for (i = 0; i < num_devices; i++) {
		char cbuf[BUF_LEN];
		size_t len;
		IOHIDDeviceRef os_dev = device_array[i];
		
		len = make_path(os_dev, cbuf, sizeof(cbuf));
		if (!strcmp(cbuf, path)) {
			// Matched Paths. Open this Device.
			IOReturn ret = IOHIDDeviceOpen(os_dev, kIOHIDOptionsTypeNone);
			if (ret == kIOReturnSuccess) {
				char str[32];
				CFIndex max_input_report_len;

				free(device_array);
				CFRelease(device_set);
				dev->device_handle = os_dev;
				
				/* Create the buffers for receiving data */
				max_input_report_len = (CFIndex) get_max_report_length(os_dev);
				dev->input_report_buf = calloc(max_input_report_len, sizeof(uint8_t));
				
				/* Create the Run Loop Mode for this device.
				   printing the reference seems to work. */
				sprintf(str, "%p", os_dev);
				dev->run_loop_mode = 
					CFStringCreateWithCString(NULL, str, kCFStringEncodingASCII);
				
				/* Attach the device to a Run Loop */
				IOHIDDeviceScheduleWithRunLoop(os_dev, CFRunLoopGetCurrent(), dev->run_loop_mode);
				IOHIDDeviceRegisterInputReportCallback(
					os_dev, dev->input_report_buf, max_input_report_len,
					&hid_report_callback, dev);
				
				pthread_mutex_init(&dev->mutex, NULL);
				
				return dev;
			}
			else {
				goto return_error;
			}
		}
	}

return_error:
	free(device_array);
	CFRelease(device_set);
	free(dev);
	return NULL;
}

static int set_report(hid_device *dev, IOHIDReportType type, const unsigned char *data, size_t length)
{
	const unsigned char *data_to_send;
	size_t length_to_send;
	IOReturn res;

	if (data[0] == 0x0) {
		/* Not using numbered Reports.
		   Don't send the report number. */
		data_to_send = data+1;
		length_to_send = length-1;
	}
	else {
		/* Using numbered Reports.
		   Send the Report Number */
		data_to_send = data;
		length_to_send = length;
	}
	
	res = IOHIDDeviceSetReport(dev->device_handle,
	                           type,
	                           data[0], /* Report ID*/
	                           data_to_send, length_to_send);
	
	if (res == kIOReturnSuccess) {
		return length;
	}
	else
		return -1;

}

int HID_API_EXPORT hid_write(hid_device *dev, const unsigned char *data, size_t length)
{
	return set_report(dev, kIOHIDReportTypeOutput, data, length);
}

/* Helper function, so that this isn't duplicated in hid_read(). */
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
	int ret_val = -1;

	/* Lock this function */
	pthread_mutex_lock(&dev->mutex);
	
	/* There's an input report queued up. Return it. */
	if (dev->input_reports) {
		/* Return the first one */
		ret_val = return_data(dev, data, length);
		goto ret;
	}
	
	/* There are no input reports queued up.
	   Need to get some from the OS. */

	/* Move the device's run loop to this thread. */
	IOHIDDeviceScheduleWithRunLoop(dev->device_handle, CFRunLoopGetCurrent(), dev->run_loop_mode);
	
	if (dev->blocking) {
		/* Run the Run Loop until it stops timing out. In other
		   words, until something happens. This is necessary because
		   there is no INFINITE timeout value. */
		SInt32 code;
		while (1) {
			code = CFRunLoopRunInMode(dev->run_loop_mode, 1000, TRUE);
			
			/* Return if some data showed up. */
			if (dev->input_reports)
				break;
			
			/* Break if The Run Loop returns Finished or Stopped. */
			if (code != kCFRunLoopRunTimedOut &&
			    code != kCFRunLoopRunHandledSource)
				break;
		}
		
		/* See if the run loop and callback gave us any reports. */
		if (dev->input_reports) {
			ret_val = return_data(dev, data, length);
			goto ret;
		}
		else {
			ret_val = -1; /* An error occured (maybe CTRL-C?). */
			goto ret;
		}
	}
	else {
		/* Non-blocking. See if the OS has any reports to give. */
		SInt32 code;
		code = CFRunLoopRunInMode(dev->run_loop_mode, 0, TRUE);
		if (dev->input_reports) {
			/* Return the first one */
			ret_val = return_data(dev, data, length);
			goto ret;
		}
		else {
			ret_val = 0; /* No data*/
			goto ret;
		}
	}

ret:
	/* Unlock */
	pthread_mutex_unlock(&dev->mutex);
	return ret_val;
}

int HID_API_EXPORT hid_set_nonblocking(hid_device *dev, int nonblock)
{
	/* All Nonblocking operation is handled by the library. */
	dev->blocking = !nonblock;
	
	return 0;
}

int HID_API_EXPORT hid_send_feature_report(hid_device *dev, const unsigned char *data, size_t length)
{
	return set_report(dev, kIOHIDReportTypeFeature, data, length);
}

int HID_API_EXPORT hid_get_feature_report(hid_device *dev, unsigned char *data, size_t length)
{
	CFIndex len = length;
	IOReturn res;

	res = IOHIDDeviceGetReport(dev->device_handle,
	                           kIOHIDReportTypeFeature,
	                           data[0], /* Report ID */
	                           data, &len);
	if (res == kIOReturnSuccess)
		return len;
	else
		return -1;
}


void HID_API_EXPORT hid_close(hid_device *dev)
{
	if (!dev)
		return;
	
	/* Close the OS handle to the device. */
	IOHIDDeviceClose(dev->device_handle, kIOHIDOptionsTypeNone);

	/* Delete any input reports still left over. */
	struct input_report *rpt = dev->input_reports;
	while (rpt) {
		struct input_report *next = rpt->next;
		free(rpt->data);
		free(rpt);
		rpt = next;
	}

	/* Free the string and the report buffer. */
	CFRelease(dev->run_loop_mode);
	free(dev->input_report_buf);
	pthread_mutex_destroy(&dev->mutex);

	free(dev);
}

int HID_API_EXPORT_CALL hid_get_manufacturer_string(hid_device *dev, wchar_t *string, size_t maxlen)
{
	return get_manufacturer_string(dev->device_handle, string, maxlen);
}

int HID_API_EXPORT_CALL hid_get_product_string(hid_device *dev, wchar_t *string, size_t maxlen)
{
	return get_product_string(dev->device_handle, string, maxlen);
}

int HID_API_EXPORT_CALL hid_get_serial_number_string(hid_device *dev, wchar_t *string, size_t maxlen)
{
	return get_serial_number(dev->device_handle, string, maxlen);
}

int HID_API_EXPORT_CALL hid_get_indexed_string(hid_device *dev, int string_index, wchar_t *string, size_t maxlen)
{
	int res;

	// TODO:

	return 0;
}


HID_API_EXPORT const wchar_t * HID_API_CALL  hid_error(hid_device *dev)
{
	// TODO:

	return NULL;
}


#if 0
int main(void)
{
	IOHIDManagerRef mgr;
	int i;
	
	mgr = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
	IOHIDManagerSetDeviceMatching(mgr, NULL);
	IOHIDManagerOpen(mgr, kIOHIDOptionsTypeNone);
	
	CFSetRef device_set = IOHIDManagerCopyDevices(mgr);
	
	CFIndex num_devices = CFSetGetCount(device_set);
	IOHIDDeviceRef *device_array = calloc(num_devices, sizeof(IOHIDDeviceRef));
	CFSetGetValues(device_set, (const void **) device_array);
	
	setlocale(LC_ALL, "");
	
	for (i = 0; i < num_devices; i++) {
		IOHIDDeviceRef dev = device_array[i];
		printf("Device: %p\n", dev);
		printf("  %04hx %04hx\n", get_vendor_id(dev), get_product_id(dev));

		wchar_t serial[256], buf[256];
		char cbuf[256];
		get_serial_number(dev, serial, 256);

		
		printf("  Serial: %ls\n", serial);
		printf("  Loc: %ld\n", get_location_id(dev));
		get_transport(dev, buf, 256);
		printf("  Trans: %ls\n", buf);
		make_path(dev, cbuf, 256);
		printf("  Path: %s\n", cbuf);
		
	}
	
	return 0;
}
#endif
