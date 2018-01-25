/* $Id$ $Revision$ */
/* vim:set shiftwidth=4 ts=8: */

/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: See CVS logs. Details at http://www.graphviz.org/
 *************************************************************************/

using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

namespace Graphviz
{
	public class UTF8Marshaler : ICustomMarshaler
	{
		public static ICustomMarshaler GetInstance(string cookie)
		{
			/* this function is needed by the marshalling machinery */
			return _instance;
		}
		
		void ICustomMarshaler.CleanUpManagedData(object managedObj)
		{
			/* free anything allocated by MarshalNativeToManaged */
		}

		void ICustomMarshaler.CleanUpNativeData(IntPtr pNativeData)
		{
			/* free anything allocated by MarshalManagedtoNative */
			Marshal.FreeHGlobal(pNativeData);
		}

		int ICustomMarshaler.GetNativeDataSize()
		{
			return 1;
		}

		IntPtr ICustomMarshaler.MarshalManagedToNative(object managedObj)
		{
			if (managedObj == null)
				return IntPtr.Zero;
			else {
				/* convert managed string to byte array, and then copy over to freshly allocated C string */
				string managedString = (string)managedObj;
				byte[] bytes = Encoding.UTF8.GetBytes(managedString);
				int length = bytes.Length;
				IntPtr nativeData = Marshal.AllocHGlobal(bytes.Length + 1);
				unsafe {
					byte* nativeBytes = (byte*)nativeData;
					int i;
					fixed (byte* bytePtr = bytes) {
						for (i = 0; i < length; ++i)
							nativeBytes[i] = bytePtr[i];
					}
					nativeBytes[i] = 0;
				}
				return nativeData;
			}
		}

		object ICustomMarshaler.MarshalNativeToManaged(IntPtr pNativeData)
		{
			if (pNativeData == IntPtr.Zero)
				return null;
			else
				/* if native data is non-empty C string, calculate length and create a managed string around it */
				/* NOTE: we use the unsafe string constructor to avoid double copying */
				unsafe {
					sbyte* nativeBytes = (sbyte*)pNativeData;
					if (*nativeBytes == 0)
						return string.Empty;
					else {
						int length = 0;
						for (sbyte* nextByte = nativeBytes; *nextByte != 0; ++nextByte)
							++length;
						return new string(nativeBytes, 0, length, Encoding.UTF8);
					}
				}
		}

		private static UTF8Marshaler _instance = new UTF8Marshaler();
	}
}
