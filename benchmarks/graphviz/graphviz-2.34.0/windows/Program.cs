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

using Microsoft.VisualBasic.ApplicationServices;

namespace Graphviz
{
	public class Program : WindowsFormsApplicationBase
	{
		protected override bool OnStartup(StartupEventArgs eventArgs)
		{
			/* if no files opened from the Explorer, pose the open file dialog to get them, then open the lot */
			ICollection<string> filesToOpen = eventArgs.CommandLine.Count == 0 ?
				(ICollection<string>)FormController.Instance.FilesToOpen() :
				(ICollection<string>)eventArgs.CommandLine;
			if (filesToOpen != null) {
				MainForm = FormController.Instance.OpenFiles(filesToOpen);
				return base.OnStartup(eventArgs);
			}
			else
				/* user cancelled open dialog, so just quit */
				return false;
		}
		
		protected override void OnStartupNextInstance(StartupNextInstanceEventArgs eventArgs)
		{
			/* if some files opened from the Explorer, open them */
			if (eventArgs.CommandLine.Count > 0)
				MainForm = FormController.Instance.OpenFiles(eventArgs.CommandLine);
			base.OnStartupNextInstance(eventArgs);
		}
		
		private Program()
		{
			EnableVisualStyles = true;
			IsSingleInstance = true;
			ShutdownStyle = ShutdownMode.AfterAllFormsClose;
		}
		
		[STAThread]
		static void Main(string[] commandLine)
		{
			new Program().Run(commandLine);
		}
	}
}
