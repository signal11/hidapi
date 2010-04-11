/*******************************************************
 Windows HID simplification

 Alan Ott
 Signal 11 Software

 8/22/2009

 Copyright 2009, All Rights Reserved.
 
 This software may be used by anyone for any reason so
 long as this copyright notice remains intact.
********************************************************/

#include <stdlib.h>

#ifdef WIN32
      #define HID_API_EXPORT __declspec(dllexport)
      #define HID_API_CALL  _stdcall
#define HID_API_EXPORT_CALL HID_API_EXPORT HID_API_CALL
#endif

#ifdef __cplusplus
extern "C" {
#endif
        int  HID_API_EXPORT HID_API_CALL hid_open(unsigned short vendor_id, unsigned short product_id, wchar_t *serial_number);
        int  HID_API_EXPORT HID_API_CALL hid_write(int device, const unsigned char *data, size_t length);
        int  HID_API_EXPORT HID_API_CALL hid_read(int device, unsigned char *data, size_t length);
        void HID_API_EXPORT HID_API_CALL hid_close(int device);

		int HID_API_EXPORT_CALL hid_get_manufacturer_string(int device, wchar_t *string, size_t maxlen);
		int HID_API_EXPORT_CALL hid_get_product_string(int device, wchar_t *string, size_t maxlen);
		int HID_API_EXPORT_CALL hid_get_serial_number_string(int device, wchar_t *string, size_t maxlen);
		int HID_API_EXPORT_CALL hid_get_indexed_string(int device, int string_index, wchar_t *string, size_t maxlen);

		HID_API_EXPORT const wchar_t* HID_API_CALL hid_error(int device);

#ifdef __cplusplus
}
#endif

