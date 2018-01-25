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
using System.IO;
using System.Threading;

namespace Graphviz
{
	public class PathWatcher: IDisposable
	{
		public event CancelEventHandler Changed;
		
		public ISynchronizeInvoke SynchronizingObject
		{
			get { return _synchronizingObject; }
			set { _synchronizingObject = value; }
		}
		
		public string Watched
		{
			get { return _watched; }
		}
		
		public PathWatcher(string watched)
		{
			_watched = watched;
			_synchronizingObject = null;
			
			_lastChange = DateTime.MinValue;
			_checker = new Timer(delegate(object state)
			{
				CheckPath();
			});

			_watcher = new FileSystemWatcher(Path.GetDirectoryName(watched), Path.GetFileName(watched));
			_watcher.NotifyFilter = NotifyFilters.FileName | NotifyFilters.LastWrite;
			_watcher.Changed += delegate(object sender, FileSystemEventArgs eventArgs)
			{
				/* check now */
				CheckPath();
			};
			_watcher.Created += delegate(object sender, FileSystemEventArgs eventArgs)
			{
				/* check now */
				CheckPath();
			};
			_watcher.Renamed += delegate(object sender, RenamedEventArgs eventArgs)
			{
				/* if file got changed to our name, check now */
				if (eventArgs.Name == _watcher.Filter)
					CheckPath();
			};
		}
		
		public void Start()
		{
			_watcher.EnableRaisingEvents = true;
		}
		
		void IDisposable.Dispose()
		{
			_checker.Dispose();
		}
		
		private void CheckPath()
		{
			/* schedule a check for the path on the synchronizing object, usually a window */
			_synchronizingObject.BeginInvoke((ThreadStart)delegate()
			{
				if (Changed != null)
				{
					/* debounce file watch changes: if last write time hadn't actually changed, don't bother notifying */
					/* NOTE: we only mutate _lastChange via this synchronized code, so there should be no race condition */
					DateTime thisChange = File.GetLastWriteTimeUtc(_watched);
					if (_lastChange != thisChange)
					{
						/* if any registered delegate didn't cancel the event, ask checker to check it again after a while */
						CancelEventArgs eventArgs = new CancelEventArgs(true);
						Changed(this, eventArgs);
						if (eventArgs.Cancel)
							_lastChange = thisChange;
						else
							_checker.Change(RECHECK_DELAY, Timeout.Infinite);
					}
				}
			}, new object[0]);
		
		}
		private const int RECHECK_DELAY = 100;
		
		private readonly string _watched;
		private ISynchronizeInvoke _synchronizingObject;

		private DateTime _lastChange;
		private readonly Timer _checker;
		private readonly FileSystemWatcher _watcher;

	}
}
