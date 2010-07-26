/*******************************************************
 Windows HID simplification

 Alan Ott
 Signal 11 Software

 8/22/2009

 Copyright 2009, All Rights Reserved.
 
 This software may be used by anyone for any reason so
 long as this copyright notice remains intact.
********************************************************/

#include <windows.h>

//#define HIDAPI_USE_DDK

extern "C" {
	#include <setupapi.h>
	#ifdef HIDAPI_USE_DDK
		#include <hidsdi.h>
	#endif
}
#include <stdio.h>
#include <stdlib.h>


#include "hidapi.h"

extern "C" {

#ifndef HIDAPI_USE_DDK
	// Since we're not building with the DDK, and the HID header
	// files aren't part of the SDK, we have to define all this
	// stuff here. In lookup_functions(), the function pointers
	// defined below are set.
	typedef struct _HIDD_ATTRIBUTES{
		ULONG Size;
		USHORT VendorID;
		USHORT ProductID;
		USHORT VersionNumber;
	} HIDD_ATTRIBUTES, *PHIDD_ATTRIBUTES;
	typedef BOOLEAN (__stdcall *HidD_GetAttributes_)(HANDLE device, PHIDD_ATTRIBUTES attrib);
	typedef BOOLEAN (__stdcall *HidD_GetSerialNumberString_)(HANDLE device, PVOID buffer, ULONG buffer_len);
	typedef BOOLEAN (__stdcall *HidD_GetManufacturerString_)(HANDLE handle, PVOID buffer, ULONG buffer_len);
	typedef BOOLEAN (__stdcall *HidD_GetProductString_)(HANDLE handle, PVOID buffer, ULONG buffer_len);
	typedef BOOLEAN (__stdcall *HidD_SetFeature_)(HANDLE handle, PVOID data, ULONG length);
	typedef BOOLEAN (__stdcall *HidD_GetFeature_)(HANDLE handle, PVOID data, ULONG length);
	typedef BOOLEAN (__stdcall *HidD_GetIndexedString_)(HANDLE handle, ULONG string_index, PVOID buffer, ULONG buffer_len);
	
	static HidD_GetAttributes_ HidD_GetAttributes;
	static HidD_GetSerialNumberString_ HidD_GetSerialNumberString;
	static HidD_GetManufacturerString_ HidD_GetManufacturerString;
	static HidD_GetProductString_ HidD_GetProductString;
	static HidD_SetFeature_ HidD_SetFeature;
	static HidD_GetFeature_ HidD_GetFeature;
	static HidD_GetIndexedString_ HidD_GetIndexedString;

	static BOOLEAN initialized = FALSE;
#endif // HIDAPI_USE_DDK

struct hid_device_ {
		HANDLE device_handle;
		BOOL blocking;
		void *last_error_str;
		DWORD last_error_num;
};

static hid_device *new_hid_device()
{
	hid_device *dev = (hid_device*) calloc(1, sizeof(hid_device));
	dev->device_handle = INVALID_HANDLE_VALUE;
	dev->blocking = true;
	dev->last_error_str = NULL;
	dev->last_error_num = 0;

	return dev;
}


static void register_error(hid_device *device, const char *op)
{
	LPTSTR msg=NULL;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&msg, 0/*sz*/,
		NULL);
	
	// Store the message off in the Device entry so that 
	// the hid_error() function can pick it up.
	LocalFree(device->last_error_str);
	device->last_error_str = msg;
}

#ifndef HIDAPI_USE_DDK
static void lookup_functions()
{
	HMODULE lib = LoadLibrary("hid.dll");
	if (lib) {
#define RESOLVE(x) x = (x##_)GetProcAddress(lib, #x);
		RESOLVE(HidD_GetAttributes);
		RESOLVE(HidD_GetSerialNumberString);
		RESOLVE(HidD_GetManufacturerString);
		RESOLVE(HidD_GetProductString);
		RESOLVE(HidD_SetFeature);
		RESOLVE(HidD_GetFeature);
		RESOLVE(HidD_GetIndexedString);
		//FreeLibrary(lib);
#undef RESOLVE
	}
	initialized = true;
}
#endif

struct hid_device_info HID_API_EXPORT * HID_API_CALL hid_enumerate(unsigned short vendor_id, unsigned short product_id)
{
	int i;
	int handle = -1;
	BOOL res;
	struct hid_device_info *root = NULL; // return object
	struct hid_device_info *cur_dev = NULL;

#ifndef HIDAPI_USE_DDK
	if (!initialized)
		lookup_functions();
#endif

	// Windows objects for interacting with the driver.
	GUID InterfaceClassGuid = {0x4d1e55b2, 0xf16f, 0x11cf, 0x88, 0xcb, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30};
	SP_DEVINFO_DATA devinfo_data;
	SP_DEVICE_INTERFACE_DATA device_interface_data;
	SP_DEVICE_INTERFACE_DETAIL_DATA *device_interface_detail_data = NULL;
	HDEVINFO device_info_set = INVALID_HANDLE_VALUE;

	// Initialize the Windows objects.
	devinfo_data.cbSize = sizeof(SP_DEVINFO_DATA);
	device_interface_data.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);


	// Get information for all the devices belonging to the HID class.
	device_info_set = SetupDiGetClassDevs(&InterfaceClassGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	
	// Iterate over each device in the HID class, looking for the right one.
	int device_index = 0;
	for (;;) {
		DWORD required_size = 0;
		res = SetupDiEnumDeviceInterfaces(device_info_set,
			NULL,
			&InterfaceClassGuid,
			device_index,
			&device_interface_data);
		
		if (!res) {
			// A return of FALSE from this function means that
			// there are no more devices.
			break;
		}

		// Call with 0-sized detail size, and let the function
		// tell us how long the detail struct needs to be. The
		// size is put in &required_size.
		res = SetupDiGetDeviceInterfaceDetail(device_info_set,
			&device_interface_data,
			NULL,
			0,
			&required_size,
			NULL);

		// Allocate a long enough structure for device_interface_detail_data.
		device_interface_detail_data = (SP_DEVICE_INTERFACE_DETAIL_DATA*) malloc(required_size);
		device_interface_detail_data->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

		// Get the detailed data for this device. The detail data gives us
		// the device path for this device, which is then passed into
		// CreateFile() to get a handle to the device.
		res = SetupDiGetDeviceInterfaceDetail(device_info_set,
			&device_interface_data,
			device_interface_detail_data,
			required_size,
			NULL,
			NULL);

		if (!res) {
			//register_error(dev, "Unable to call SetupDiGetDeviceInterfaceDetail");
			// Continue to the next device.
			goto cont;
		}

		//wprintf(L"HandleName: %s\n", device_interface_detail_data->DevicePath);

		// Open a handle to the device
		HANDLE write_handle = CreateFile(device_interface_detail_data->DevicePath,
			GENERIC_WRITE |GENERIC_READ,
			0x0, /*share mode*/
			NULL,
			OPEN_EXISTING,
			FILE_FLAG_OVERLAPPED,//FILE_ATTRIBUTE_NORMAL,
			0);

		// Check validity of write_handle.
		if (write_handle == INVALID_HANDLE_VALUE) {
			// Unable to open the device.
			//register_error(dev, "CreateFile");
			CloseHandle(write_handle);
			goto cont;
		}		


		// Get the Vendor ID and Product ID for this device.
		HIDD_ATTRIBUTES attrib;
		attrib.Size = sizeof(HIDD_ATTRIBUTES);
		HidD_GetAttributes(write_handle, &attrib);
		//wprintf(L"Product/Vendor: %x %x\n", attrib.ProductID, attrib.VendorID);

		// Check the VID/PID to see if we should add this
		// device to the enumeration list.
		if ((vendor_id == 0x0 && product_id == 0x0) || 
			(attrib.VendorID == vendor_id && attrib.ProductID == product_id)) {

			#define WSTR_LEN 512
			const char *str;
			struct hid_device_info *tmp;
			wchar_t wstr[WSTR_LEN]; // TODO: Determine Size
			size_t len;

			/* VID/PID match. Create the record. */
			tmp = (hid_device_info*) calloc(1, sizeof(struct hid_device_info));
			if (cur_dev) {
				cur_dev->next = tmp;
			}
			else {
				root = tmp;
			}
			cur_dev = tmp;
			
			/* Fill out the record */
			cur_dev->next = NULL;
			str = device_interface_detail_data->DevicePath;
			if (str) {
				len = strlen(str);
				cur_dev->path = (char*) calloc(len+1, sizeof(char));
				strncpy(cur_dev->path, str, len+1);
				cur_dev->path[len] = '\0';
			}
			else
				cur_dev->path = NULL;

			/* Serial Number */
			res = HidD_GetSerialNumberString(write_handle, wstr, sizeof(wstr));
			wstr[WSTR_LEN-1] = 0x0000;
			if (res) {
				cur_dev->serial_number = _wcsdup(wstr);
			}

			/* Manufacturer String */
			res = HidD_GetManufacturerString(write_handle, wstr, sizeof(wstr));
			wstr[WSTR_LEN-1] = 0x0000;
			if (res) {
				cur_dev->manufacturer_string = _wcsdup(wstr);
			}

			/* Product String */
			res = HidD_GetProductString(write_handle, wstr, sizeof(wstr));
			wstr[WSTR_LEN-1] = 0x0000;
			if (res) {
				cur_dev->product_string = _wcsdup(wstr);
			}

			/* VID/PID */
			cur_dev->vendor_id = attrib.VendorID;
			cur_dev->product_id = attrib.ProductID;
		}

cont:
		CloseHandle(write_handle);

		// We no longer need the detail data. It can be freed
		free(device_interface_detail_data);

		device_index++;

	}

	// Close the device information handle.
	SetupDiDestroyDeviceInfoList(device_info_set);

	return root;

}

void  HID_API_EXPORT HID_API_CALL hid_free_enumeration(struct hid_device_info *devs)
{
	// TODO: Merge this with the Linux version. This function is platform-independent.
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


HID_API_EXPORT hid_device * HID_API_CALL hid_open(unsigned short vendor_id, unsigned short product_id, wchar_t *serial_number)
{
	// TODO: Merge this functions with the Linux version. This function should be platform independent.
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

HID_API_EXPORT hid_device * HID_API_CALL hid_open_path(const char *path)
{
  	int i;
	hid_device *dev;

#ifndef HIDAPI_USE_DDK
	if (!initialized)
		lookup_functions();
#endif

	dev = new_hid_device();

	// Open a handle to the device
	dev->device_handle = CreateFile(path,
			GENERIC_WRITE |GENERIC_READ,
			0x0, /*share mode*/
			NULL,
			OPEN_EXISTING,
			FILE_FLAG_OVERLAPPED,//FILE_ATTRIBUTE_NORMAL,
			0);

	// Check validity of write_handle.
	if (dev->device_handle == INVALID_HANDLE_VALUE) {
		// Unable to open the device.
		register_error(dev, "CreateFile");
		CloseHandle(dev->device_handle);
		free(dev);
		return NULL;
	}

	return dev;
}

int HID_API_EXPORT HID_API_CALL hid_write(hid_device *dev, const unsigned char *data, size_t length)
{
	DWORD bytes_written;
	BOOL res;

	OVERLAPPED ol;
	memset(&ol, 0, sizeof(ol));
	ol.Internal = 0x0;
	ol.InternalHigh = 0x0;
	ol.Pointer = 0x0;
	ol.hEvent = 0x0;

	res = WriteFile(dev->device_handle, data, length, NULL, &ol);
	
	if (!res) {
		if (GetLastError() != ERROR_IO_PENDING) {
			// WriteFile() failed. Return error.
			register_error(dev, "WriteFile");
			return -1;
		}
	}

	// Wait here until the write is done. This makes
	// hid_write() synchronous.
	res = GetOverlappedResult(dev->device_handle, &ol, &bytes_written, TRUE/*wait*/);
	if (!res) {
		// The Write operation failed.
		register_error(dev, "WriteFile");
		return -1;
	}

	return bytes_written;
}


int HID_API_EXPORT HID_API_CALL hid_read(hid_device *dev, unsigned char *data, size_t length)
{
	DWORD bytes_read;
	BOOL res;

	HANDLE ev;
	ev = CreateEvent(NULL, FALSE, FALSE /*inital state f=nonsignaled*/, NULL);

	OVERLAPPED ol;
	memset(&ol, 0, sizeof(ol));
	ol.Internal = 0x0;
	ol.InternalHigh = 0x0;
	ol.Pointer = 0x0;
	ol.hEvent = ev;

	res = ReadFile(dev->device_handle, data, length, &bytes_read, &ol);
	
	
	if (!res) {
		if (GetLastError() != ERROR_IO_PENDING) {
			// ReadFile() has failed.
			// Clean up and return error.
			CloseHandle(ev);
			goto end_of_function;
		}
	}

	if (!dev->blocking) {
		// See if there is any data yet.
		res = WaitForSingleObject(ev, 0);
		CloseHandle(ev);
		if (res != WAIT_OBJECT_0) {
			// There was no data. Cancel this read and return.
			CancelIo(dev->device_handle);
			// Zero bytes available.
			return 0;
		}
	}

	// Either WaitForSingleObject() told us that ReadFile has completed, or
	// we are in non-blocking mode. Get the number of bytes read. The actual
	// data has been copied to the data[] array which was passed to ReadFile().
	res = GetOverlappedResult(dev->device_handle, &ol, &bytes_read, TRUE/*wait*/);

	if (bytes_read > 0 && data[0] == 0x0) {
		/* If report numbers aren't being used, but Windows sticks a report
		   number (0x0) on the beginning of the report anyway. To make this
		   work like the other platforms, and to make it work more like the
		   HID spec, we'll skip over this byte. */
		bytes_read--;
		memmove(data, data+1, bytes_read);
	}
	
end_of_function:
	if (!res) {
		register_error(dev, "ReadFile");
		return -1;
	}
	
	return bytes_read;
}

int HID_API_EXPORT HID_API_CALL hid_set_nonblocking(hid_device *dev, int nonblock)
{
	dev->blocking = !nonblock;
	return 0; /* Success */
}

int HID_API_EXPORT HID_API_CALL hid_send_feature_report(hid_device *dev, const unsigned char *data, size_t length)
{
	BOOL res = HidD_SetFeature(dev->device_handle, (PVOID)data, length);
	if (!res) {
		register_error(dev, "HidD_SetFeature");
		return -1;
	}

	return length;
}


int HID_API_EXPORT HID_API_CALL hid_get_feature_report(hid_device *dev, unsigned char *data, size_t length)
{
	BOOL res = HidD_GetFeature(dev->device_handle, data, length);
	if (!res) {
		register_error(dev, "HidD_GetFeature");
		return -1;
	}

	return 0; /* Windows doesn't give us an actual length, unfortunately */
}

void HID_API_EXPORT HID_API_CALL hid_close(hid_device *dev)
{
	CloseHandle(dev->device_handle);
	LocalFree(dev->last_error_str);
	free(dev);
}

int HID_API_EXPORT_CALL HID_API_CALL hid_get_manufacturer_string(hid_device *dev, wchar_t *string, size_t maxlen)
{
	BOOL res;

	res = HidD_GetManufacturerString(dev->device_handle, string, 2 * maxlen);
	if (!res) {
		register_error(dev, "HidD_GetManufacturerString");
		return -1;
	}

	return 0;
}

int HID_API_EXPORT_CALL HID_API_CALL hid_get_product_string(hid_device *dev, wchar_t *string, size_t maxlen)
{
	BOOL res;

	res = HidD_GetProductString(dev->device_handle, string, 2 * maxlen);
	if (!res) {
		register_error(dev, "HidD_GetProductString");
		return -1;
	}

	return 0;
}

int HID_API_EXPORT_CALL HID_API_CALL hid_get_serial_number_string(hid_device *dev, wchar_t *string, size_t maxlen)
{
	BOOL res;

	res = HidD_GetSerialNumberString(dev->device_handle, string, 2 * maxlen);
	if (!res) {
		register_error(dev, "HidD_GetSerialNumberString");
		return -1;
	}

	return 0;
}

int HID_API_EXPORT_CALL HID_API_CALL hid_get_indexed_string(hid_device *dev, int string_index, wchar_t *string, size_t maxlen)
{
	BOOL res;

	res = HidD_GetIndexedString(dev->device_handle, string_index, string, 2 * maxlen);
	if (!res) {
		register_error(dev, "HidD_GetIndexedString");
		return -1;
	}

	return 0;
}


HID_API_EXPORT const char * HID_API_CALL  hid_error(hid_device *dev)
{
	return (const char *)dev->last_error_str;
}


//#define PICPGM
//#define S11
#define P32
#ifdef S11 
  unsigned short VendorID = 0xa0a0;
	unsigned short ProductID = 0x0001;
#endif

#ifdef P32
  unsigned short VendorID = 0x04d8;
	unsigned short ProductID = 0x3f;
#endif


#ifdef PICPGM
  unsigned short VendorID = 0x04d8;
  unsigned short ProductID = 0x0033;
#endif


#if 0
int __cdecl main(int argc, char* argv[])
{
	int res;
	unsigned char buf[65];

	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);

	// Set up the command buffer.
	memset(buf,0x00,sizeof(buf));
	buf[0] = 0;
	buf[1] = 0x81;
	

	// Open the device.
	int handle = open(VendorID, ProductID, L"12345");
	if (handle < 0)
		printf("unable to open device\n");


	// Toggle LED (cmd 0x80)
	buf[1] = 0x80;
	res = write(handle, buf, 65);
	if (res < 0)
		printf("Unable to write()\n");

	// Request state (cmd 0x81)
	buf[1] = 0x81;
	write(handle, buf, 65);
	if (res < 0)
		printf("Unable to write() (2)\n");

	// Read requested state
	read(handle, buf, 65);
	if (res < 0)
		printf("Unable to read()\n");

	// Print out the returned buffer.
	for (int i = 0; i < 4; i++)
		printf("buf[%d]: %d\n", i, buf[i]);

	return 0;
}
#endif

} // extern "C"

