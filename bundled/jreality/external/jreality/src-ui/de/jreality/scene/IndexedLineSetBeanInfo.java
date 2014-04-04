package de.jreality.scene;

import java.beans.BeanInfo;
import java.beans.IntrospectionException;
import java.beans.Introspector;
import java.beans.PropertyDescriptor;
import java.beans.SimpleBeanInfo;

public class IndexedLineSetBeanInfo extends SimpleBeanInfo {
	
	public BeanInfo[] getAdditionalBeanInfo() {
		try {
			return new BeanInfo[]{Introspector.getBeanInfo(PointSet.class)};
		} catch (Exception e) {
			return null;
		}
	}
	
	public PropertyDescriptor[] getPropertyDescriptors() {
		Class beanClass = IndexedLineSet.class;
		try {  
			PropertyDescriptor lineCount =
				new PropertyDescriptor(
						" lineCount",
						beanClass,
						"getNumEdges",
						null
				);

			PropertyDescriptor rv[] = {
					lineCount
			};
			return rv;
		} catch (IntrospectionException e) {
			throw new Error(e.toString());
		}
	}
}
