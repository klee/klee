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
using System.Text;
using System.Windows.Forms;
using System.Windows.Forms.Design;
using System.Xml;
using System.Xml.XPath;

namespace Graphviz
{
	public partial class AttributeInspectorForm : Form
	{
		public static Form Instance
		{
			get
			{
				return _instance;
			}
		}
		
		public AttributeInspectorForm()
		{
			InitializeComponent();
	
			/* remove existing tabs and add in our attribute property tabs */
			List<Type> tabsToRemove = new List<Type>();
			foreach (PropertyTab tabToRemove in attributePropertyGrid.PropertyTabs)
				tabsToRemove.Add(tabToRemove.GetType());
			foreach (Type tabToRemove in tabsToRemove)
				attributePropertyGrid.PropertyTabs.RemoveTabType(tabToRemove);
			attributePropertyGrid.PropertyTabs.AddTabType(typeof(GraphPropertyTab), PropertyTabScope.Static);
			attributePropertyGrid.PropertyTabs.AddTabType(typeof(NodePropertyTab), PropertyTabScope.Static);
			attributePropertyGrid.PropertyTabs.AddTabType(typeof(EdgePropertyTab), PropertyTabScope.Static);
			 
			_inspected = null;
			_inspectMainForm = delegate(object sender, EventArgs e)
			{
				/* if the main form has a graph, monitor its changes and show its properties */
				GraphForm graphForm = FormController.Instance.MainForm as GraphForm;
				if (graphForm != null)
				{
					if (_inspected != null)
						_inspected.Changed -= _inspectMainForm;
					_inspected = graphForm;
					_inspected.Changed += _inspectMainForm;
					
					Text = "Attributes of " + graphForm.Text;
					attributePropertyGrid.SelectedObject = graphForm.Graph;
				}
			};
			
			/* inspect the graph when the handle has been created */
			/* NOTE: if we set the SelectedObject BEFORE the handle has been created, this damages the property tabs, a Microsoft bug */
			if (attributePropertyGrid.IsHandleCreated)
				_inspectMainForm(this, EventArgs.Empty);
			else
				attributePropertyGrid.HandleCreated += _inspectMainForm;
			
			/* inspect the graph when the main form changes */
			FormController.Instance.MainFormChanged += _inspectMainForm;
		}

		protected override void OnFormClosing(FormClosingEventArgs e)
		{
			if (e.CloseReason == CloseReason.UserClosing)
			{
			/* instead of closing, we hide ourselves */
			Hide();
			e.Cancel = true;
		}
			else
				_instance = null;
		}
		
		private class GraphPropertyTab : ExternalPropertyTab
		{
			public GraphPropertyTab() : base("Graph Attributes", Properties.Resources.GraphAttributes, _graphProperties)
			{
			}
		}

		private class NodePropertyTab : ExternalPropertyTab
		{
			public NodePropertyTab() : base("Node Attributes", Properties.Resources.NodeAttributes, _nodeProperties)
			{
			}
		}

		private class EdgePropertyTab : ExternalPropertyTab
		{
			public EdgePropertyTab() : base("Edge Attributes", Properties.Resources.EdgeAttributes, _edgeProperties)
			{
			}
		}

		private static PropertyDescriptorCollection GetComponentProperties(XPathNavigator schema, string component)
		{
			PropertyDescriptorCollection properties = new PropertyDescriptorCollection(new PropertyDescriptor[0]);
			
			XmlNamespaceManager namespaces = new XmlNamespaceManager(schema.NameTable);
			namespaces.AddNamespace("xsd", "http://www.w3.org/2001/XMLSchema");
			namespaces.AddNamespace("html", "http://www.w3.org/1999/xhtml");
			
			/* look for each attribute with the graph component */
			XPathNodeIterator componentElements = schema.Select(
				string.Format("/xsd:schema/xsd:complexType[@name='{0}']/xsd:attribute", component),
				namespaces);
			while (componentElements.MoveNext()) {
				List<Attribute> descriptorAttributes = new List<Attribute>();
				string name = componentElements.Current.GetAttribute("ref", string.Empty);
				
				/* add default attribute for any defaults */
				string descriptorDefault = componentElements.Current.GetAttribute("default", string.Empty);
				if (!string.IsNullOrEmpty (descriptorDefault))
					descriptorAttributes.Add(new DefaultValueAttribute(descriptorDefault));
					
				/* add description attribute for any documentation (we'll need to preserve paragraph breaks though) */
				StringBuilder description = new StringBuilder();
				XPathNodeIterator paragraphs = schema.Select (
					string.Format("/xsd:schema/xsd:attribute[@name='{0}']/xsd:annotation/xsd:documentation/html:p", name), namespaces);
				while (paragraphs.MoveNext ()) {
					if (description.Length > 0)
						description.Append("\r\n");
					description.Append((string)paragraphs.Current.Evaluate("normalize-space(.)", namespaces));
				}
				if (description.Length > 0)
					descriptorAttributes.Add(new DescriptionAttribute(description.ToString()));
				
				/* add standard values and type converter attribute for xsd:boolean or any enumerations */
				string type = (string)schema.Evaluate(
					string.Format("string(/xsd:schema/xsd:attribute[@name='{0}']/@type)", name), namespaces);
				switch (type) {
				case "xsd:boolean":
					descriptorAttributes.Add(new StandardValuesTypeConverter.Attribute(new string[]{"false", "true"}));
					descriptorAttributes.Add(new TypeConverterAttribute(typeof(StandardValuesTypeConverter)));
					break;
				default:
					if (!type.StartsWith("xsd")) {
						XPathNodeIterator enumeration = schema.Select(
							string.Format("/xsd:schema/xsd:simpleType[@name='{0}']/xsd:restriction/xsd:enumeration/@value", type),
							namespaces);
						if (enumeration.Count > 0) {
							List<string> standardValues = new List<string>();
							while (enumeration.MoveNext())
								standardValues.Add(enumeration.Current.Value);
							
							descriptorAttributes.Add(new StandardValuesTypeConverter.Attribute(standardValues));
							descriptorAttributes.Add(new TypeConverterAttribute(typeof(StandardValuesTypeConverter)));
						}
					}
					break;
				}
				
				/* now create and add the property to the collection */
				properties.Add(new GraphPropertyDescriptor(component, name, descriptorAttributes.ToArray()));
			}
			return properties;
		}
		
		static AttributeInspectorForm()
		{
			/* read the schema from our application directory */
			/* NOTE: we use XPathDocument instead of XmlDocument since it is faster in the read-only case */
			XPathNavigator schema = new XPathDocument(Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "attributes.xml")).CreateNavigator();
			
			/* scan attribute schema for graph components */
			_graphProperties = GetComponentProperties(schema, "graph");
			_nodeProperties = GetComponentProperties(schema, "node");
			_edgeProperties = GetComponentProperties(schema, "edge");

			// NOTE: this has to run AFTER we set up the graph, node & edge properties so the external property tabs can read them in
			_instance = new AttributeInspectorForm();
		}
		
		private static AttributeInspectorForm _instance;
		private static PropertyDescriptorCollection _graphProperties;
		private static PropertyDescriptorCollection _nodeProperties;
		private static PropertyDescriptorCollection _edgeProperties;
		
		private GraphForm _inspected;
		private EventHandler _inspectMainForm;
	}
}
