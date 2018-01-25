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
using System.ComponentModel;
using System.Drawing;
using System.Drawing.Imaging;
using System.Drawing.Printing;
using System.IO;
using System.Windows.Forms;
using System.Collections.Generic;
using System.Text;

namespace Graphviz
{
	public partial class GraphForm : Form, FormController.IMenus
	{
		public event EventHandler Changed;
		
		public Graph Graph
		{
			get { return _graph; }
		}
		
		public GraphForm(string fileName)
		{
			InitializeComponent();

			/* window title is the filename portion */
			Text = Path.GetFileName(fileName);
		
			/* associated icon is eventually sourced by the desktop from this executable's embedded Win32 resources */
			Icon = Icon.ExtractAssociatedIcon(fileName);

			_graph = new Graph(fileName);
		
			/* whenever graph changes, rerender and display the graph */
			_graph.Changed += delegate(object sender, EventArgs e)
		{
				RenderGraph();
			};
			_graph.Arguments["layout"] = "dot";
			
			_watcher = new PathWatcher(fileName);
			_watcher.SynchronizingObject = this;
			_watcher.Changed += delegate(object sender, CancelEventArgs eventArgs)
			{
				try
				{
					/* replace old graph with new and rerender */
					Graph newGraph = new Graph(((PathWatcher)sender).Watched);
						((IDisposable)_graph).Dispose();
						_graph = newGraph;
						_graph.Arguments["layout"] = "dot";
						RenderGraph();
						
						if (Changed != null)
							Changed(this, EventArgs.Empty);
					}
				catch (Win32Exception exception)
				{
					/* if another process is holding on to the graph, try opening again later */
					if (exception.NativeErrorCode == 32)
						eventArgs.Cancel = false;
				}
			};
			_watcher.Start();
		
			/* set up export dialog with initial file and filters from the graph devices */
			StringBuilder exportFilter = new StringBuilder();
			int filterIndex = 0;
			for (int deviceIndex = 0; deviceIndex < _devices.Count; ++deviceIndex)
			{
				if (deviceIndex > 0)
					exportFilter.Append("|");
					
				string device = _devices[deviceIndex];
				exportFilter.Append(String.Format("{0} (*.{1})|*.{1}", device.ToUpper(), device));
				
				if (filterIndex == 0 && device == "emfplus")
					filterIndex = deviceIndex;
			}
			exportFileDialog.FileName = Path.GetFileNameWithoutExtension(fileName);
			exportFileDialog.Filter = exportFilter.ToString();
			exportFileDialog.FilterIndex = filterIndex + 1;
			
			printDocument.DocumentName = Path.GetFileName(fileName);
		}

		protected override void OnFormClosed(FormClosedEventArgs e)
		{
			/* when form closes, clean up graph too */
			((IDisposable)_graph).Dispose();
			base.OnFormClosed(e);
		}

		private void RenderGraph()
		{
			using (Stream stream = _graph.Render("emfplus:gdiplus"))
				graphControl.Image = new Metafile(stream);
		}
		
		ToolStripMenuItem FormController.IMenus.ExitMenuItem
		{
			get { return exitToolStripMenuItem; }
		}

		ToolStripMenuItem FormController.IMenus.OpenMenuItem
		{
			get { return openToolStripMenuItem; }
		}

		ToolStripMenuItem FormController.IMenus.WindowMenuItem
		{
			get { return windowToolStripMenuItem; }
		}
		
		private void ShowAttributes_Click(object sender, EventArgs eventArgs)
		{
			AttributeInspectorForm.Instance.Show();
		}

		private void ZoomToFit_Click(object sender, EventArgs eventArgs)
		{
			graphControl.ZoomToFit();
		}
		
		private void ActualSize_Click(object sender, EventArgs eventArgs)
		{
			graphControl.ActualSize();
		}
		
		private void ZoomIn_Click(object sender, EventArgs eventArgs)
		{
			graphControl.ZoomIn();
		}

		private void ZoomOut_Click(object sender, EventArgs eventArgs)
		{
			graphControl.ZoomOut();
		}

		private void exportToolStripMenuItem_Click(object sender, EventArgs e)
		{
			/* pose the export dialog and then render the graph with the selected file and format */
			if (exportFileDialog.ShowDialog(this) == DialogResult.OK)
			{
				int filterIndex = exportFileDialog.FilterIndex - 1;
				if (filterIndex >= 0 && filterIndex < _devices.Count)
					_graph.Render(_devices[filterIndex], exportFileDialog.FileName);
			}
		}
		
		private void pageSetupToolStripMenuItem_Click(object sender, EventArgs e)
		{
			pageSetupDialog.ShowDialog(this);
		}

		private void printToolStripMenuItem_Click(object sender, EventArgs e)
		{
			if (printDialog.ShowDialog(this) == DialogResult.OK)
				printDocument.Print();
		}

		private void printPreviewToolStripMenuItem_Click(object sender, EventArgs e)
		{
			printPreviewDialog.ShowDialog(this);
		}

		private void printDocument_BeginPrint(object sender, PrintEventArgs e)
		{
			new TileableImagePrinter((PrintDocument)sender, graphControl.Image);
		}
		
		private static readonly IList<string> _devices = Graph.GetPlugins(Graph.API.Device, false);
		private Graph _graph;
		private readonly PathWatcher _watcher;
	}
}
