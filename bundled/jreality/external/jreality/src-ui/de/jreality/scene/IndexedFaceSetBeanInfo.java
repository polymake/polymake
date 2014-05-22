package de.jreality.scene;

import java.beans.BeanInfo;
import java.beans.IntrospectionException;
import java.beans.Introspector;
import java.beans.PropertyDescriptor;
import java.beans.SimpleBeanInfo;


public class IndexedFaceSetBeanInfo extends SimpleBeanInfo {
	
	public BeanInfo[] getAdditionalBeanInfo() {
		try {
			return new BeanInfo[]{Introspector.getBeanInfo(IndexedLineSet.class)};
		} catch (Exception e) {
			return null;
		}
	}
	
	public PropertyDescriptor[] getPropertyDescriptors() {
		Class beanClass = IndexedFaceSet.class;
		try {  
			PropertyDescriptor faceCount =
				new PropertyDescriptor(
						" faceCount",
						beanClass,
						"getNumFaces",
						null
				);
			PropertyDescriptor rv[] = {
					faceCount
			};
			return rv;
		} catch (IntrospectionException e) {
			throw new Error(e.toString());
		}
	}
}
