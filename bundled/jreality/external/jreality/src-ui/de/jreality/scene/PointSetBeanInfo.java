package de.jreality.scene;

import java.beans.BeanInfo;
import java.beans.IntrospectionException;
import java.beans.Introspector;
import java.beans.PropertyDescriptor;
import java.beans.SimpleBeanInfo;

public class PointSetBeanInfo extends SimpleBeanInfo {
	
	public BeanInfo[] getAdditionalBeanInfo() {
		try {
			return new BeanInfo[]{Introspector.getBeanInfo(Geometry.class)};
		} catch (Exception e) {
			return null;
		}
	}
	
	public PropertyDescriptor[] getPropertyDescriptors() {
		Class beanClass = PointSet.class;
		try {  
			PropertyDescriptor vertexCount =
				new PropertyDescriptor(
						" vertexCount",
						beanClass,
						"getNumPoints",
						null
				);

			PropertyDescriptor rv[] = {
					vertexCount
			};
			return rv;
		} catch (IntrospectionException e) {
			throw new Error(e.toString());
		}
	}
}
