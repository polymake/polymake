package de.jreality.geometry;

import java.beans.BeanDescriptor;
import java.beans.SimpleBeanInfo;

/** Announces the customizer {@link PointSetFactoryCustomizer} for JavaBeans introspection.
 * 
 * @author G. Paul Peters, 03.06.2010
 */
public class PointSetFactoryBeanInfo extends SimpleBeanInfo {

	private final BeanDescriptor bd = new BeanDescriptor(PointSetFactory.class, PointSetFactoryCustomizer.class);
	
	public PointSetFactoryBeanInfo() {
		bd.setDisplayName("Point Set Factory Explorer");
	}

	@Override
	public BeanDescriptor getBeanDescriptor() {
		return bd;
	}
}
