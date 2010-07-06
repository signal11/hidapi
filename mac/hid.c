/*********************************
Mac OS X version of HID API
 
Alan Ott
Signal 11 Software

2010-07-03
*********************************/


#include <IOKit/hid/IOHIDManager.h>
#include <IOKit/hid/IOHIDKeys.h>
#include <wchar.h>
#include <locale.h>

#include "hidapi.h"

/* Linked List of input reports received from the device. */
struct input_report {
	uint8_t *data;
	size_t len;
	struct input_report *next;
};

struct Device {
	int valid;
	IOHIDDeviceRef device_handle;
	int blocking;
	int uses_numbered_reports;
	CFStringRef run_loop_mode;
	uint8_t *input_report_buf;
	struct input_report *input_reports;
};


#define MAX_DEVICES 64
static struct Device devices[MAX_DEVICES];
static int devices_initialized = 0;
static 	IOHIDManagerRef hid_mgr = 0x0;


static void register_error(struct Device *device, const char *op)
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


struct hid_device  HID_API_EXPORT *hid_enumerate(unsigned short vendor_id, unsigned short product_id)
{
	struct hid_device *root = NULL; // return object
	struct hid_device *cur_dev = NULL;
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
			struct hid_device *tmp;
			size_t len;

		    	/* VID/PID match. Create the record. */
			tmp = malloc(sizeof(struct hid_device));
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

void  HID_API_EXPORT hid_free_enumeration(struct hid_device *devs)
{
	/* This function is identical to the Linux version. Platform independent. */
	struct hid_device *d = devs;
	while (d) {
		struct hid_device *next = d->next;
		free(d->path);
		free(d->serial_number);
		free(d->manufacturer_string);
		free(d->product_string);
		free(d);
		d = next;
	}
}

int HID_API_EXPORT hid_open(unsigned short vendor_id, unsigned short product_id, wchar_t *serial_number)
{
	/* This function is identical to the Linux version. Platform independent. */
	struct hid_device *devs, *cur_dev;
	const char *path_to_open = NULL;
	int handle = -1;
	
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
	struct Device *dev = context;
	
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

int HID_API_EXPORT hid_open_path(const char *path)
{
  	int i;
	int handle = -1;
	struct Device *dev = NULL;
	CFIndex num_devices;

	// Initialize the Device array if it hasn't been done.
	if (!devices_initialized) {
		int i;
		for (i = 0; i < MAX_DEVICES; i++) {
			devices[i].valid = 0;
			devices[i].device_handle = NULL;
			devices[i].blocking = 1;
			devices[i].uses_numbered_reports = 0;
			devices[i].input_reports = NULL;
		}
		devices_initialized = 1;
	}

	// Find an available handle to use;
	for (i = 0; i < MAX_DEVICES; i++) {
		if (!devices[i].valid) {
			devices[i].valid = 1;
			devices[i].device_handle = NULL;
			devices[i].blocking = 1;
			devices[i].uses_numbered_reports = 0;
			devices[i].input_reports = NULL;
			handle = i;
			dev = &devices[i];
			break;
		}
	}

	if (handle < 0) {
		return -1;
	}
	
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
				
				return handle;
			}
			else {
				goto return_error;
			}
		}
	}

return_error:
	free(device_array);
	CFRelease(device_set);
	dev->valid = 0;
	return -1;
}

static int set_report(struct Device *dev, IOHIDReportType type, const unsigned char *data, size_t length)
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

int HID_API_EXPORT hid_write(int device, const unsigned char *data, size_t length)
{
	struct Device *dev = NULL;

	// Get the handle 
	if (device < 0 || device >= MAX_DEVICES)
		return -1;
	if (devices[device].valid == 0)
		return -1;
	dev = &devices[device];
	
	return set_report(dev, kIOHIDReportTypeOutput, data, length);
}

/* Helper function, so that this isn't duplicated in hid_read(). */
static int return_data(struct Device *dev, unsigned char *data, size_t length)
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

int HID_API_EXPORT hid_read(int device, unsigned char *data, size_t length)
{
	struct Device *dev = NULL;
	int bytes_read = -1;

	// Get the handle 
	if (device < 0 || device >= MAX_DEVICES)
		return -1;
	if (devices[device].valid == 0)
		return -1;
	dev = &devices[device];
	
	/* There's an input report queued up. Return it. */
	if (dev->input_reports) {
		/* Return the first one */
		return return_data(dev, data, length);
	}
	
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
				
		if (dev->input_reports) {
			return return_data(dev, data, length);	
		}
		else
			return -1; /* An error occured (maybe CTRL-C?). */
	}
	else {
		/* Non-blocking. See if the OS has any reports to give. */
		SInt32 code;
		code = CFRunLoopRunInMode(dev->run_loop_mode, 0, TRUE);
		if (dev->input_reports) {
			/* Return the first one */
			return return_data(dev, data, length);
		}
		else
			return 0; /* No data*/
	}
	
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
	
	/* All Nonblocking operation is handled by the library. */
	dev->blocking = !nonblock;
	
	return 0;
}

int HID_API_EXPORT hid_send_feature_report(int device, const unsigned char *data, size_t length)
{
	struct Device *dev = NULL;

	// Get the handle 
	if (device < 0 || device >= MAX_DEVICES)
		return -1;
	if (devices[device].valid == 0)
		return -1;
	dev = &devices[device];

	return set_report(dev, kIOHIDReportTypeFeature, data, length);
}

int HID_API_EXPORT hid_get_feature_report(int device, unsigned char *data, size_t length)
{
	struct Device *dev = NULL;
	CFIndex len = length;
	IOReturn res;

	// Get the handle 
	if (device < 0 || device >= MAX_DEVICES)
		return -1;
	if (devices[device].valid == 0)
		return -1;
	dev = &devices[device];

	res = IOHIDDeviceGetReport(dev->device_handle,
	                           kIOHIDReportTypeFeature,
	                           data[0], /* Report ID */
	                           data, &len);
	if (res == kIOReturnSuccess)
		return length;
	else
		return -1;
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

	// TODO:

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

	// TODO:

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

	// TODO:

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

	// TODO:

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
