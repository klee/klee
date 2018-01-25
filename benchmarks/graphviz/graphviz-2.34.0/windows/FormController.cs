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
using System.IO;
using System.Windows.Forms;

namespace Graphviz
{
	public class FormController
	{
		/* document forms need to declare these menus */
		public interface IMenus
		{
			ToolStripMenuItem ExitMenuItem
			{
				get;
			}

			ToolStripMenuItem OpenMenuItem
			{
				get;
			}

			ToolStripMenuItem WindowMenuItem
			{
				get;
			}
		}
		
		public static FormController Instance
		{
			get { return _instance; }
		}
		
		public Form MainForm
		{
			/* return the topmost document form */
			get { return _mainForm; }
		}

		public event EventHandler MainFormChanged;

		public string[] FilesToOpen()
		{
			/* if user said OK, return the files he selected */
			return _openFileDialog.ShowDialog() == DialogResult.OK ? _openFileDialog.FileNames : null;
		}

		public Form OpenFiles(ICollection<string> fileNames)
		{
			Form foundForm = null;
			foreach (string fileName in fileNames) {
				string canonicalPath = Path.GetFullPath(fileName).ToLower();
				if (_documentForms.ContainsKey(canonicalPath))
					/* document already open */
					foundForm = _documentForms[canonicalPath];
				else {
					/* document needs to be created */
					_documentForms[canonicalPath] = foundForm = CreateForm(fileName);

					IMenus documentFormMenus = foundForm as IMenus;
					if (documentFormMenus != null) {
						/* exit menu quits the app */
						documentFormMenus.ExitMenuItem.Click += delegate(object sender, EventArgs eventArgs)
						{
							Application.Exit();
						};

						/* open menu asks user which files to open and then opens them */
						documentFormMenus.OpenMenuItem.Click += delegate(object sender, EventArgs eventArgs)
						{
							string[] filesToOpen = FilesToOpen();
							if (filesToOpen != null)
								OpenFiles(filesToOpen);
						};

						/* window menu shows a list of all document forms, select one to activate it */
						documentFormMenus.WindowMenuItem.DropDownOpening += delegate(object sender, EventArgs eventArgs)
						{
							ToolStripMenuItem windowMenuItem = sender as ToolStripMenuItem;
							if (windowMenuItem != null)
							{
								windowMenuItem.DropDownItems.Clear();
								int i = 0;
								foreach (Form form in _documentForms.Values)
								{
									Form innerForm = form;
									ToolStripMenuItem formMenuItem = new ToolStripMenuItem(string.Format("{0} {1}", ++i, form.Text));
									formMenuItem.Checked = Form.ActiveForm == innerForm;
									formMenuItem.Click += delegate(object innerSender, EventArgs innerEventArgs)
									{
										innerForm.Activate();
									};
									windowMenuItem.DropDownItems.Add(formMenuItem);
								}
							}
						};
					}

					/* when form activated, change the main form */
					foundForm.Activated += delegate(object sender, EventArgs eventArgs)
					{
						_mainForm = (Form)sender;
						if (MainFormChanged != null)
							MainFormChanged(_mainForm, EventArgs.Empty);
					};

					/* when form closed, remove it from our list; exit when all closed */
					foundForm.FormClosed += delegate(object sender, FormClosedEventArgs e)
					{
						_documentForms.Remove(canonicalPath);
						if (_documentForms.Count == 0)
							Application.Exit();
					};

					foundForm.Show();
				}
			}
			return foundForm;
		}
		
		private FormController()
		{
			_openFileDialog = new OpenFileDialog();
			_openFileDialog.Filter = "Graphviz graphs (*.dot;*.gv)|*.dot;*.gv|All files (*.*)|*.*";
			_openFileDialog.Multiselect = true;
			
			_documentForms = new SortedDictionary<string, Form>();
			_mainForm = null;
		}

		private Form CreateForm(string fileName)
		{
			return new GraphForm(fileName);
		}

		private static FormController _instance = new FormController();
		private readonly OpenFileDialog _openFileDialog;
		private readonly SortedDictionary<string, Form> _documentForms;
		private Form _mainForm;
	}
}
