/*******************************************************
 Windows HID simplification

 Alan Ott
 Signal 11 Software

 8/22/2009

 Copyright 2009, All Rights Reserved.
 
 This software may be used by anyone for any reason so
 long as this copyright notice remains intact.
********************************************************/

#include <wchar.h>

#ifdef _WIN32
      #define HID_API_EXPORT __declspec(dllexport)
      #define HID_API_CALL  _stdcall
#else
      #define HID_API_EXPORT
      #define HID_API_CALL
#endif

#define HID_API_EXPORT_CALL HID_API_EXPORT HID_API_CALL

#ifdef __cplusplus
extern "C" {
#endif
		struct hid_device {
			/** Platform-specific device path */
			char *path;
			/** Device Vendor ID */
			unsigned short vendor_id;
			/** Device Product ID */
			unsigned short product_id;
			/** Serial Number */
			wchar_t *serial_number;
			/** Manufacturer String */
			wchar_t *manufacturer_string;
			/** Product string */
			wchar_t *product_string;
			
			/** Pointer to the next device */
			struct hid_device *next;
		};
		

		/** Enumerate the HID Devices.
		    This function returns a linked list of all the HID devices
		    attached to the system which match vendor_id and product_id.
		    If vendor_id and product_id are both set to 0, then all HID
		    devices will be returned.
		    
		    Params:
		    	vendor_id: The Vendor ID (VID) of the types of device to open.
		    	product_id: The Product ID (PID) of the types of device to open.
		    
		    Return:
		    	This function returns a pointer to a linked list of type
		    	struct hid_device, containing information about the HID devices
		    	attached to the system, or NULL in the case of failure. Free
		    	this linked list by calling hid_free_enumeration().
		*/
		struct hid_device HID_API_EXPORT * HID_API_CALL hid_enumerate(unsigned short vendor_id, unsigned short product_id);
		
		/** Free an enumeration Linked List
		    This function frees a linked list created by hid_enumerate().
		    
		    Params:
		    	devs: Pointer to a list of struct_device returned from
		    	      hid_enumerate().
			
			Return:
				This function does not return a value.
		
		*/
		void  HID_API_EXPORT HID_API_CALL hid_free_enumeration(struct hid_device *devs);
		
		/** Open a HID device using a Vendor ID (VID), Product ID (PID) and
		    optionally a serial number. If serial_number is NULL, the first
		    device with the specified VID and PID is opened.

			Params:
				vendor_id: The Vendor ID (VID) of the device to open.
				product_id: The Product ID (PID) of the device to open.
				serial_number: The Serial Number of the device to open
				               (Optionally NULL).
			
			Return:
				This function returns a small integer handle on success
				and -1 on failure.
		*/
		int  HID_API_EXPORT HID_API_CALL hid_open(unsigned short vendor_id, unsigned short product_id, wchar_t *serial_number);

		/** Open a HID device by its path name. The path name be determined
		    by calling hid_enumerate(), or a platform-specific path name can
		    be used (eg: /dev/hidraw0 on Linux).
		    
		    Params:
		    	path: The path name of the device to open
			
			Return:
				This function returns a small integer handle on success
				and -1 on failure.
		*/
		int  HID_API_EXPORT HID_API_CALL hid_open_path(const char *path);

		/** Write an Output report to a HID device. The first byte of data[]
		    must contain the Report ID. For devices which only support a single
			report, this must be set to 0x0. The remaining bytes contain the
			report data. Since the Report ID is mandatory, calls to hid_write()
			will always contain one more byte than the report contains. For
			example, if a hid report is 16 bytes long, 17 bytes must be passed
			to hid_write(), the Report ID (or 0x0, for devices with a single
			report), followed by the report data (16 bytes). In this example,
			the length passed in would be 17.

			hid_write() will send the data on the first OUT endpoint, if one
			exists. If it does not, it will send the data through the Control
			Endpoint (Endpoint 0).

			Params:
				device: A device handle returned from hid_open().
				data: The data to send, including the report number as
				      the first byte.
				length: The length in bytes of the data to send.

			Return Value:
				This function returns the actual number of bytes written and
				-1 on error.
		*/
		int  HID_API_EXPORT HID_API_CALL hid_write(int device, const unsigned char *data, size_t length);

		/** Read a report from a HID device. The first byte will contain the
		    Report number, if multiple reports are supported by the device.

			Params:
				device: A device handle returned from hid_open().
				data: A buffer to put the read data into.
				length: The number of bytes to read. For devices with multiple
				        reports, make sure to read an extra byte for the
						report number.
			
			Return Value:
				This function returns the actual number of bytes read and
				-1 on error.
		*/
		int  HID_API_EXPORT HID_API_CALL hid_read(int device, unsigned char *data, size_t length);

		/** Set the device handle to be non-blocking. In non-blocking mode
		    calls to hid_read() will return immediately with a value of 0
			if there is no data to be read. In blocking mode, hid_read()
			will wait (block) until there is data to read before returning.

			Nonblocking can be turned on and off at any time.

			Params:
				device: A device handle returned from hid_open().
				nonblock: 1 to enable nonblocking or 0 to disable
				          nonblocking.
			
			Return Value:
				This function returns 0 on success and -1 on error.
		*/
		int  HID_API_EXPORT HID_API_CALL hid_set_nonblocking(int device, int nonblock);

		/** Close a HID device.
			
			Params:
				device: A device handle returned from hid_open().
		*/
		void HID_API_EXPORT HID_API_CALL hid_close(int device);

		/** Get The Manufacturer String from a HID device.

			Params:
				device: A device handle returned from hid_open().
				string: A wide string buffer to put the data into.
				maxlen: The length of the buffer in multiples of wchar_t.
			
			Return Value:
				This function returns 0 on success and -1 on error.
		*/
		int HID_API_EXPORT_CALL hid_get_manufacturer_string(int device, wchar_t *string, size_t maxlen);

		/** Get The Product String from a HID device.

			Params:
				device: A device handle returned from hid_open().
				string: A wide string buffer to put the data into.
				maxlen: The length of the buffer in multiples of wchar_t.
			
			Return Value:
				This function returns 0 on success and -1 on error.
		*/
		int HID_API_EXPORT_CALL hid_get_product_string(int device, wchar_t *string, size_t maxlen);

		/** Get The Serial Number String from a HID device.

			Params:
				device: A device handle returned from hid_open().
				string: A wide string buffer to put the data into.
				maxlen: The length of the buffer in multiples of wchar_t.
			
			Return Value:
				This function returns 0 on success and -1 on error.
		*/
		int HID_API_EXPORT_CALL hid_get_serial_number_string(int device, wchar_t *string, size_t maxlen);

		/** Get a string from a HID device, based on its string index.

			Params:
				device: A device handle returned from hid_open().
				string_index: The index of the string to get.
				string: A wide string buffer to put the data into.
				maxlen: The length of the buffer in multiples of wchar_t.
			
			Return Value:
				This function returns 0 on success and -1 on error.
		*/
		int HID_API_EXPORT_CALL hid_get_indexed_string(int device, int string_index, wchar_t *string, size_t maxlen);

		/** Get a string describing the last error which occurred.
			
			Params:
				device: A device handle returned from hid_open().

			Return Value:
				This function returns a string containing the last error
				which occurred or NULL if none has occurred.
		*/
		HID_API_EXPORT const char* HID_API_CALL hid_error(int device);

#ifdef __cplusplus
}
#endif

