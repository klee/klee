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

namespace Graphviz
{
	public class GraphArguments : IDictionary<string, string>
	{
		public GraphArguments(Graph graph)
		{
			_dictionary = new Dictionary<string, string>();
			_graph = graph;
		}
		
		void IDictionary<string, string>.Add(string key, string value)
		{
			_dictionary.Add(key, value);
			_graph.NoteChanged(true);
		}

		bool IDictionary<string, string>.ContainsKey(string key)
		{
			return _dictionary.ContainsKey(key);
		}

		ICollection<string> IDictionary<string, string>.Keys
		{
			get { return _dictionary.Keys; }
		}

		bool IDictionary<string, string>.Remove(string key)
		{
			if (_dictionary.Remove(key)) {
				_graph.NoteChanged(true);
				return true;
			}
			else
				return false;
		}

		bool IDictionary<string, string>.TryGetValue(string key, out string value)
		{
			return _dictionary.TryGetValue(key, out value);
		}

		ICollection<string> IDictionary<string, string>.Values
		{
			get { return _dictionary.Values; }
		}

		string IDictionary<string, string>.this[string key]
		{
			get
			{
				return _dictionary[key];
			}
			set
			{
				_dictionary[key] = value;
				_graph.NoteChanged(true);
			}
		}

		void ICollection<KeyValuePair<string, string>>.Add(KeyValuePair<string, string> item)
		{
			_dictionary.Add(item);
		}

		void ICollection<KeyValuePair<string, string>>.Clear()
		{
			_dictionary.Clear();
			_graph.NoteChanged(true);
		}

		bool ICollection<KeyValuePair<string, string>>.Contains(KeyValuePair<string, string> item)
		{
			return _dictionary.Contains(item);
		}

		void ICollection<KeyValuePair<string, string>>.CopyTo(KeyValuePair<string, string>[] array, int arrayIndex)
		{
			_dictionary.CopyTo(array, arrayIndex);
		}

		int ICollection<KeyValuePair<string, string>>.Count
		{
			get { return ((ICollection<KeyValuePair<string, string>>)_dictionary).Count; }
		}

		bool ICollection<KeyValuePair<string, string>>.IsReadOnly
		{
			get { return true; }
		}

		bool ICollection<KeyValuePair<string, string>>.Remove(KeyValuePair<string, string> item)
		{
			if (_dictionary.Remove(item)) {
				_graph.NoteChanged(true);
				return true;
			}
			else
				return false;
		}

		IEnumerator<KeyValuePair<string, string>> IEnumerable<KeyValuePair<string, string>>.GetEnumerator()
		{
			return _dictionary.GetEnumerator();
		}

		IEnumerator IEnumerable.GetEnumerator()
		{
			return _dictionary.GetEnumerator();
		}

		private readonly Graph _graph;
		private readonly IDictionary<string, string> _dictionary;
	}
}
