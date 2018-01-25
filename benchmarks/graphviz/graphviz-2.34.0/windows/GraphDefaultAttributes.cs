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
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Graphviz
{
	public class GraphDefaultAttributes : IDictionary<string, string>
	{
		public GraphDefaultAttributes(Graph graph, IntPtr proto)
		{
			_graph = graph;
			_proto = proto;
		}
		
		void IDictionary<string, string>.Add(string key, string value)
		{
			SetValue(key, value);
		}

		bool IDictionary<string, string>.ContainsKey(string key)
		{
			return !string.IsNullOrEmpty(GetValue(key));
		}

		ICollection<string> IDictionary<string, string>.Keys
		{
			get
			{
				/* return all keys with non-null, non-empty values */
				List<string> keys = new List<string>();
				IEnumerator<Agsym_t> attributes = EnumAttributes();
				while (attributes.MoveNext())
					if (attributes.Current.value != IntPtr.Zero && Marshal.ReadByte(attributes.Current.value) != 0)
						keys.Add(MarshalBytes(attributes.Current.name));
				return keys;
			}
		}

		bool IDictionary<string, string>.Remove(string key)
		{
			/* if key exists, overwrite with empty */
			if (agfindattr(_proto, key) != IntPtr.Zero) {
				SetValue(key, string.Empty);
				return true;
			}
			else
				return false;
		}

		bool IDictionary<string, string>.TryGetValue(string key, out string value)
		{
			/* grab value for key, but if it's empty return null instead */
			value = GetValue(key);
			if (value == string.Empty) {
				value = null;
				return false;
			}
			else
				return value != null;
		}

		ICollection<string> IDictionary<string, string>.Values
		{
			get
			{
				/* return all non-null, non-empty values */
				List<string> values = new List<string>();
				IEnumerator<Agsym_t> attributes = EnumAttributes();
				while (attributes.MoveNext()) {
					string attrValue = MarshalBytes(attributes.Current.value);
					if (!string.IsNullOrEmpty(attrValue))
						values.Add(attrValue);
				}
				return values;
			}
		}

		string IDictionary<string, string>.this[string key]
		{
			get
			{
				/* return non-null, non-empty value */
				string value = GetValue(key);
				if (string.IsNullOrEmpty(value))
					throw new KeyNotFoundException();
				else
					return value;
			}
			set
			{
				SetValue(key, value);
			}
		}

		void ICollection<KeyValuePair<string, string>>.Add(KeyValuePair<string, string> item)
		{
			SetValue(item.Key, item.Value);
		}

		void ICollection<KeyValuePair<string, string>>.Clear()
		{
			IEnumerator<Agsym_t> attributes = EnumAttributes();
			while (attributes.MoveNext())
				SetValue(MarshalBytes(attributes.Current.name), string.Empty);
		}

		bool ICollection<KeyValuePair<string, string>>.Contains(KeyValuePair<string, string> item)
		{
			return GetValue(item.Key) == item.Value;
		}

		void ICollection<KeyValuePair<string, string>>.CopyTo(KeyValuePair<string, string>[] array, int arrayIndex)
		{
			throw new Exception("The method or operation is not implemented.");
		}

		int ICollection<KeyValuePair<string, string>>.Count
		{
			get
			{
				/* count all non-null, non-empty values */
				int count = 0;
				IEnumerator<Agsym_t> attributes = EnumAttributes();
				while (attributes.MoveNext())
					if (attributes.Current.value != IntPtr.Zero && Marshal.ReadByte(attributes.Current.value) != 0)
						++count;
				return count;
			}
		}

		bool ICollection<KeyValuePair<string, string>>.IsReadOnly
		{
			get { return false; }
		}

		bool ICollection<KeyValuePair<string, string>>.Remove(KeyValuePair<string, string> item)
		{
			/* if key + value are present, overwrite value with empty */
			if (GetValue(item.Key) == item.Value) {
				SetValue(item.Key, string.Empty);
				return true;
			}
			else
				return false;
		} 

		IEnumerator<KeyValuePair<string, string>> IEnumerable<KeyValuePair<string, string>>.GetEnumerator()
		{
			/* return all keys with values that are non-null and non-empty */
			IEnumerator<Agsym_t> attributes = EnumAttributes();
			while (attributes.MoveNext()) {
				string attrValue = MarshalBytes(attributes.Current.value);
				if (!string.IsNullOrEmpty(attrValue))
					yield return new KeyValuePair<string, string>(
						MarshalBytes(attributes.Current.value),
						attrValue);
			}
		}

		IEnumerator IEnumerable.GetEnumerator()
		{
			/* return all keys with values that are non-null and non-empty */
			IEnumerator<Agsym_t> attributes = EnumAttributes();
			while (attributes.MoveNext()) {
				string attrValue = MarshalBytes(attributes.Current.value);
				if (!string.IsNullOrEmpty(attrValue))
					yield return new KeyValuePair<string, string>(
						MarshalBytes(attributes.Current.value),
						attrValue);
			}
		}
		
		private string GetValue(string key)
		{
			/* try to find the value for the key */
			IntPtr attrPtr = agfindattr(_proto, key);
			if (attrPtr != IntPtr.Zero) {
				Agsym_t attr = (Agsym_t)Marshal.PtrToStructure(attrPtr, typeof(Agsym_t));
				return MarshalBytes(attr.value);
			}
			else
				return null;
		}
		
		private void SetValue(string key, string value)
		{
			/* change the attribute value and notify the parent graph */
			agattr(_proto, key, value);
			_graph.NoteChanged(true);
		}

		private IEnumerator<Agsym_t> EnumAttributes()
		{
			/* go through each attribute symbol */
			for (IntPtr nextAttrPtr = agfstattr(_proto); nextAttrPtr != IntPtr.Zero; nextAttrPtr = agnxtattr(_proto, nextAttrPtr))
				yield return (Agsym_t)Marshal.PtrToStructure(nextAttrPtr, typeof(Agsym_t));
		}
		
		private static string MarshalBytes(IntPtr bytes)
		{
			return (string)UTF8Marshaler.GetInstance(null).MarshalNativeToManaged(bytes);
		}
		
		[DllImport("libgraph-4.dll")]
		private static extern IntPtr agattr(
			IntPtr obj,
			[MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(UTF8Marshaler))] string name,
			[MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(UTF8Marshaler))] string value);
		
		[DllImport("libgraph-4.dll")]
		private static extern IntPtr agfindattr(
			IntPtr obj,
			[MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(UTF8Marshaler))] string name);
		
		[DllImport("libgraph-4.dll")]
		private static extern IntPtr agfstattr(IntPtr obj);
		
		[DllImport("libgraph-4.dll")]
		private static extern IntPtr agnxtattr(IntPtr obj, IntPtr attr);

		[StructLayout(LayoutKind.Sequential)]
		private struct Agsym_t
		{
			public IntPtr name;
			public IntPtr value;
			
			public int index;
			public byte printed;
			public byte fix;
		};

		private readonly Graph _graph;
		private readonly IntPtr _proto;
		
		
	}
}
