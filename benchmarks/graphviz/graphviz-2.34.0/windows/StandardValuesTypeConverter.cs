using System;
using System.Collections;
using System.ComponentModel;
using System.Text;

namespace Graphviz
{
	public class StandardValuesTypeConverter : TypeConverter
	{
		public class Attribute : System.Attribute
		{
			public StandardValuesCollection Values
			{
				get { return _values; }
			}
			
			public Attribute(ICollection values)
			{
				_values = new StandardValuesCollection(values);
			}
			
			private readonly StandardValuesCollection _values;
		}
		
		public override StandardValuesCollection GetStandardValues(ITypeDescriptorContext context)
		{
			Attribute attribute = (Attribute)context.PropertyDescriptor.Attributes[typeof(Attribute)];
			return attribute == null ? null : attribute.Values;
		}
		
		public override bool GetStandardValuesExclusive(ITypeDescriptorContext context)
		{
			return false;
		}
		
		public override bool GetStandardValuesSupported(ITypeDescriptorContext context)
		{
			return (Attribute)context.PropertyDescriptor.Attributes[typeof(Attribute)] != null;
		}
	}
}
