from libc.stddef cimport wchar_t, size_t

cdef extern from "hidapi.h":
  ctypedef struct hid_device:
    pass

  cdef struct hid_device_info:
    char *path
    unsigned short vendor_id
    unsigned short product_id
    wchar_t *serial_number
    unsigned short release_number
    wchar_t *manufacturer_string
    wchar_t *product_string
    unsigned short usage_page
    unsigned short usage
    int interface_number
    hid_device_info *next

  hid_device_info* hid_enumerate(unsigned short, unsigned short)
  void hid_free_enumeration(hid_device_info*)
  
  hid_device* hid_open(unsigned short, unsigned short, void*)
  void hid_close(hid_device *)
  int hid_write(hid_device* device, unsigned char *data, int length)
  int hid_read(hid_device* device, unsigned char* data, int max_length)
  int hid_set_nonblocking(hid_device* device, int value)
  int hid_send_feature_report(hid_device* device, unsigned char *data, int length)

  int hid_get_manufacturer_string(hid_device*, wchar_t *, size_t)
  int hid_get_product_string(hid_device*, wchar_t *, size_t)
  int hid_get_serial_number_string(hid_device*, wchar_t *, size_t)
  wchar_t *hid_error(hid_device *)


  

