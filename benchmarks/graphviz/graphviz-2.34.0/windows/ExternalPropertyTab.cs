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
using System.Drawing;
using System.Windows.Forms.Design;

namespace Graphviz
{
	public class ExternalPropertyTab : PropertyTab
	{
		public ExternalPropertyTab(string name, Bitmap bitmap, PropertyDescriptorCollection externalProperties)
		{
			_name = name;
			_bitmap = bitmap;
			_externalProperties = externalProperties;
		}

		public override PropertyDescriptorCollection GetProperties(object component, Attribute[] attributes)
		{
			if (attributes == null || attributes.Length == 0)
				/* no need to filter by attribute, just return all properties */
				return _externalProperties;
			else {
				/* filter in properties that match all given attributes */
				PropertyDescriptorCollection properties = new PropertyDescriptorCollection(new PropertyDescriptor[0]);
				foreach (PropertyDescriptor property in _externalProperties) {
					bool allMatch = true;
					foreach (Attribute attribute in attributes)
						if (!property.Attributes[attribute.GetType()].Match(attribute)) {
							allMatch = false;
							break;
						}
					if (allMatch)
						properties.Add(property);
				}
				return properties;
			}
			
		}

		public override bool CanExtend(object extendee)
		{
			return true;
		}

		public override string TabName
		{
			get { return _name; }
		}

		// Provides an image for the property tab.
		public override Bitmap Bitmap
		{
			get { return _bitmap; }
		}

		private readonly string _name;
		private readonly Bitmap _bitmap;
		private readonly PropertyDescriptorCollection _externalProperties;
	}
}
