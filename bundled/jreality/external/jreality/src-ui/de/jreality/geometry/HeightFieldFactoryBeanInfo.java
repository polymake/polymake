package de.jreality.geometry;

import java.beans.BeanDescriptor;
import java.beans.SimpleBeanInfo;

/** Announces the customizer {@link HeightFieldFactoryCustomizer} for JavaBeans introspection.
 * 
 * @author G. Paul Peters, 03.06.2010
 */
public class HeightFieldFactoryBeanInfo extends SimpleBeanInfo {

	private final BeanDescriptor bd = new BeanDescriptor(HeightFieldFactory.class, HeightFieldFactoryCustomizer.class);
	
	public HeightFieldFactoryBeanInfo() {
		bd.setDisplayName("Quad Mesh Factory Explorer");
	}

	@Override
	public BeanDescriptor getBeanDescriptor() {
		return bd;
	}
}
