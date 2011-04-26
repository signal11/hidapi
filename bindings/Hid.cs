using System;
using System.Runtime.InteropServices;
using System.Text;
using Mono.Unix;

namespace ika_hid
{
	public class Hid
	{

		private IntPtr dev;
		
		public bool IsOpen() { return dev != IntPtr.Zero; }
		
		public void Open (ushort vid, ushort hid, string serial)
		{
			if (dev != IntPtr.Zero) throw new Exception("a device is already opened; close it first.");
			IntPtr ret = hid_open (vid, hid, serial);
			if (ret == IntPtr.Zero)
				throw new Exception ("Device not found");
			dev = ret;
		}

		public int Read (byte[] buffer, int length)
		{
			AssertValidDev();
			int ret = hid_read (dev, buffer, (uint)length);
			if (ret < 0)
				throw new Exception ("Failed to Read.");
			return ret;
		}

		public void Close ()
		{
			AssertValidDev();
			hid_close (dev);
			dev = IntPtr.Zero;
		}

		public String GetProductString ()
		{
			AssertValidDev();
			byte[] buf = new byte[1000];
			int ret = Hid.hid_get_product_string (dev, buf, (uint)(buf.Length / 4) - 1);
			if (ret < 0)
				throw new Exception ("failed to receive product string");
			return EncodeBuffer(buf);
		}

		public String GetManufacturerString ()
		{
			AssertValidDev();
			byte[] buf = new byte[1000];
			int ret = Hid.hid_get_manufacturer_string (dev, buf, (uint)(buf.Length / 4) - 1);
			if (ret < 0)
				throw new Exception ("failed to receive manufacturer string");
			return EncodeBuffer(buf);
		}


		public int GetFeatureReport (byte[] buffer, int length)
		{
			AssertValidDev();
			int ret = hid_get_feature_report (dev, buffer, (uint)length);
			if (ret < 0)
				throw new Exception ("failed to get feature report");
			return ret;
		}

		public int SendFeatureReport(byte[] buffer){
			int ret = hid_send_feature_report (dev, buffer, (uint)buffer.Length);
			//if (ret < 0)
			//	throw new Exception ("failed to send feature report");
			return ret;
		}
		
		public String Error ()
		{
			AssertValidDev();
			IntPtr ret = hid_error (dev);
			return UnixMarshal.PtrToString (ret, Encoding.UTF32);
		}

		
		public string GetIndexedString(int index) {
			AssertValidDev();
			byte[] buf = new byte[1000];
			int ret = Hid.hid_get_indexed_string (dev, index, buf, (uint)(buf.Length / 4) - 1);
			if (ret < 0)
				throw new Exception ("failed to receive indexed string");
			return EncodeBuffer(buf);
		}
		
		public string GetSerialNumberString() {
			AssertValidDev();
			byte[] buf = new byte[1000];
			int ret = Hid.hid_get_serial_number_string (dev, buf, (uint)(buf.Length / 4) - 1);
			if (ret < 0)
				throw new Exception ("failed to receive serial number string");
			return EncodeBuffer(buf);
			
		}

		private string EncodeBuffer(byte[] buffer) {
			return Encoding.UTF32.GetString (buffer).Trim ('\0');
		}
		
		private void AssertValidDev() {
			if (dev == IntPtr.Zero) throw new Exception("No device opened");
		}
		
		[DllImport("libhidapi.so")]
		private static extern int hid_read (IntPtr device, [Out, MarshalAs(UnmanagedType.LPArray)] byte[] data, uint length);

		[DllImport("libhidapi.so")]
		private static extern IntPtr hid_open (ushort vid, ushort pid, [MarshalAs(UnmanagedType.LPWStr)] string serial);

		[DllImport("libhidapi.so")]
		private static extern void hid_close (IntPtr device);

		[DllImport("libhidapi.so")]
		private static extern int hid_get_product_string (IntPtr device, [Out] byte[] _string, uint length);

		[DllImport("libhidapi.so")]
		private static extern int hid_get_manufacturer_string (IntPtr device, [Out] byte[] _string, uint length);

		[DllImport("libhidapi.so")]
		private static extern int hid_get_feature_report (IntPtr device, [Out, MarshalAs(UnmanagedType.LPArray)] byte[] data, uint length);

		[DllImport("libhidapi.so")]
		private static extern int hid_get_serial_number_string (IntPtr device, [Out] byte[] serial, uint maxlen);

		[DllImport("libhidapi.so")]
		private static extern int hid_get_indexed_string (IntPtr device, int string_index, [Out] byte[] _string, uint maxlen);

		[DllImport("libhidapi.so")]
		private static extern IntPtr hid_error (IntPtr device);

		[DllImport("libhidapi.so")]
		private static extern int hid_send_feature_report (IntPtr device, [In, MarshalAs(UnmanagedType.LPArray)] byte[] data, uint length);

		[DllImport("libhidapi.so")]
		private static extern int hid_set_nonblocking (IntPtr device, [In, MarshalAs(UnmanagedType.SysInt)] bool nonblock);

		[DllImport("libhidapi.so")]
		private static extern int hid_write (IntPtr device, [In, MarshalAs(UnmanagedType.LPArray)] byte[] data, uint length);

		[DllImport("libhidapi.so")]
		private static extern IntPtr hid_open_path ([In, MarshalAs(UnmanagedType.LPStr)] string path);
		
		// TODO:
		
		//public static extern void   hid_free_enumeration(struct hid_device_info *devs)
		//public static extern struct hid_device_info  *  hid_enumerate(unsigned short vendor_id, 
		// 		 unsigned short product_id);
	}

	class Utf32Marshaler : ICustomMarshaler
	{
		private static Utf32Marshaler instance = new Utf32Marshaler ();

		public static ICustomMarshaler GetInstance (string s)
		{
			return instance;
		}

		public void CleanUpManagedData (object o)
		{
		}

		public void CleanUpNativeData (IntPtr pNativeData)
		{
			UnixMarshal.FreeHeap (pNativeData);
		}

		public int GetNativeDataSize ()
		{
			return IntPtr.Size;
		}

		public IntPtr MarshalManagedToNative (object obj)
		{
			string s = obj as string;
			if (s == null)
				return IntPtr.Zero;
			return UnixMarshal.StringToHeap (s, Encoding.UTF32);
		}

		public object MarshalNativeToManaged (IntPtr pNativeData)
		{
			Console.WriteLine ("i was ere");
			return UnixMarshal.PtrToString (pNativeData, Encoding.UTF32);
		}
	}
}

