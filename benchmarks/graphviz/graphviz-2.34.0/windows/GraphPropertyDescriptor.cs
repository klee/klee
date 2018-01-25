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

namespace Graphviz
{
	public class GraphPropertyDescriptor : PropertyDescriptor
	{
		public GraphPropertyDescriptor(string graphComponent, string name, Attribute[] attrs): base(name, attrs)
		{
			_graphComponent = graphComponent;
		}
		
		public override bool CanResetValue(object component)
		{
			/* can reset whenever value isn't at default */
			return !object.Equals(GetValue(component), DefaultValue);
		}

		public override Type ComponentType
		{
			/* property defined on graph components, so we'll say object */
			get { return typeof(object); }
		}

		public override object GetValue(object component)
		{
			/* return either the set value or the default value */
			string value;
			GetDictionary(component).TryGetValue(Name, out value);
			return value == null ? DefaultValue : value;
		}

		public override bool IsReadOnly
		{
			get { return false; }
		}

		public override Type PropertyType
		{
			/* property returns strings only */
			get { return typeof(string); }
		}

		public override void ResetValue(object component)
		{
			SetValue(component, DefaultValue);
		}

		public override void SetValue(object component, object value)
		{
			GetDictionary(component)[Name] = (string)value;
		}

		public override bool ShouldSerializeValue(object component)
		{
			return CanResetValue(component);
		}
		
		private object DefaultValue
		{
			get
			{
				/* return default value attribute if any */
				DefaultValueAttribute defaultValueAttribute = (DefaultValueAttribute)Attributes[typeof(DefaultValueAttribute)];
				return defaultValueAttribute == null ? null : defaultValueAttribute.Value;
			}
		}
		
		private IDictionary<string, string> GetDictionary(object component)
		{
			/* if the component is a graph, return the appropriate dictionary */
			Graph graph = component as Graph;
			if (graph != null)
				switch (_graphComponent) {
					case "graph":
						return graph.GraphAttributes;
					case "node":
						return graph.DefaultNodeAttributes;
					case "edge":
						return graph.DefaultEdgeAttributes;
					default:
						return null;
				}
			else
				return null;		
		}
		
		private readonly string _graphComponent;
	}
}
