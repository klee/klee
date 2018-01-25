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
using System.ComponentModel;
using System.IO;
using System.Runtime.InteropServices;

namespace Graphviz
{
	public class Graph : IDisposable
	{
		public enum API
		{
			Render = 0,
			Layout = 1,
			TextLayout = 2,
			Device = 3,
			LoadImage = 4
		}
		
		public class Exception : ApplicationException
		{
			public Exception(string message): base(message)
			{
			}
		}

		public static IList<string> GetPlugins(API api, bool showFullPath)
		{
			SortedList<string, string> plugins = new SortedList<string, string>();
			IntPtr pluginList = gvplugin_list(_context, api, showFullPath ? ":" : "");
			foreach (string nextPlugin in Marshal.PtrToStringAnsi(pluginList).Split(new char[]{' '}, StringSplitOptions.RemoveEmptyEntries))
			{
				int lastColon = nextPlugin.LastIndexOf(':');
				string plugin = nextPlugin.Substring(0, lastColon == -1 ? nextPlugin.Length : lastColon);
				plugins[plugin] = plugin;
			}
			
			free(pluginList);
			return plugins.Keys;
		}
		
		public event EventHandler Changed;
		
		public IDictionary<string, string> Arguments
		{
			get { return _arguments; }
		}

		public IDictionary<string, string> GraphAttributes
		{
			get { return _graphAttributes; }
		}

		public IDictionary<string, string> DefaultNodeAttributes
		{
			get { return _defaultNodeAttributes; }
		}

		public IDictionary<string, string> DefaultEdgeAttributes
		{
			get { return _defaultEdgeAttributes; }
		}
				
		public Graph(string filename)
		{
			IntPtr file = fopen(filename, "r");
			if (file == IntPtr.Zero)
				throw new Win32Exception();
			_graph = agread(file);
			if (_graph == IntPtr.Zero)
				throw new Win32Exception();
			fclose(file);
			
			_freeLastLayout = false;
			_arguments = new GraphArguments(this);
			_graphAttributes = new GraphDefaultAttributes(this, _graph);
			_defaultNodeAttributes = new GraphDefaultAttributes(this, agprotonode(_graph));
			_defaultEdgeAttributes = new GraphDefaultAttributes(this, agprotoedge(_graph));
		}
		
		public void Save(string filename)
		{
			IntPtr file = fopen(filename, "w");
			if (file == IntPtr.Zero)
				throw new Win32Exception();
			if (agwrite(_graph, file) != 0)
				throw new Win32Exception();
			fclose(file);
		}
		
		public Stream Render(string format)
		{
			unsafe {
				byte* result;
				uint length;
				if (gvRenderData(_context, _graph, format, out result, out length) != 0)
					throw new Exception("bad render");
				return new RenderStream(result, length);
			}
		}
		
		public void Render(string format, string filename)
		{
			if (gvRenderFilename(_context, _graph, format, filename) != 0)
				throw new Exception("bad render");
		}
		
		public void NoteChanged(bool relayout)
		{
			if (relayout) {
				string layout;
				Arguments.TryGetValue("layout", out layout);
				if (layout != null) {
					if (_freeLastLayout)
						gvFreeLayout(_context, _graph);
						
					if (gvLayout(_context, _graph, layout) != 0)
						throw new Exception("bad layout");
						
					_freeLastLayout = true;
				}
			}
			
			if (Changed != null)
				Changed(this, EventArgs.Empty);
		}
		
		void IDisposable.Dispose()
		{
 			agclose(_graph);
		}
		
		private unsafe class RenderStream : UnmanagedMemoryStream
		{
			public RenderStream(byte* pointer, long length): base(pointer, length)
			{
				_pointer = pointer;
			}

			protected override void Dispose(bool disposing)
			{
				base.Dispose(disposing);
				if (disposing)
					free(_pointer);
			}
			
			private readonly byte* _pointer;
		}
		
		[DllImport("libgraph-4.dll", SetLastError = true)]
		private static extern void agclose(IntPtr file);

		[DllImport("libgraph-4.dll")]
		private static extern IntPtr agprotonode(IntPtr graph);

		[DllImport("libgraph-4.dll")]
		private static extern IntPtr agprotoedge(IntPtr graph);

		[DllImport("libgraph-4.dll", SetLastError = true)]
		private static extern IntPtr agread(IntPtr file);

		[DllImport("libgraph-4.dll", SetLastError = true)]
		private static extern int agwrite(IntPtr graph, IntPtr file);
		
		[DllImport("libgvc-4.dll")]
		private static extern IntPtr gvContext();

		[DllImport("libgvc-4.dll")]
		private static extern int gvFreeLayout(IntPtr context, IntPtr graph);

		[DllImport("libgvc-4.dll")]
		private static extern int gvLayout(IntPtr context, IntPtr graph, string engine);

		[DllImport("libgvc-4.dll")]
		private static extern IntPtr gvplugin_list(IntPtr context, API api, string str);

		[DllImport("libgvc-4.dll")]
		private static extern int gvRenderFilename(IntPtr context, IntPtr graph, string format, string filename);
		
		[DllImport("libgvc-4.dll")]
		private static extern unsafe int gvRenderData(IntPtr context, IntPtr graph, string format, out byte* result, out uint length);

		[DllImport("msvcrt.dll", SetLastError = true)]
		private static extern int fclose(IntPtr file);

		[DllImport("msvcrt.dll", SetLastError = true)]
		private static extern IntPtr fopen(string filename, string mode);

		[DllImport("msvcrt.dll", SetLastError = true)]
		private static extern unsafe void free(byte* pointer);

		[DllImport("msvcrt.dll", SetLastError = true)]
		private static extern void free(IntPtr pointer);

		private static readonly IntPtr _context = gvContext();
		
		private readonly IntPtr _graph;
		private bool _freeLastLayout;
		private readonly GraphArguments _arguments;
		private readonly GraphDefaultAttributes _graphAttributes;
		private readonly GraphDefaultAttributes _defaultNodeAttributes;
		private readonly GraphDefaultAttributes _defaultEdgeAttributes;
	}
}
