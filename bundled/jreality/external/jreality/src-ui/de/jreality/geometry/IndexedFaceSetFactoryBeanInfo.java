package de.jreality.geometry;

import java.beans.BeanDescriptor;
import java.beans.SimpleBeanInfo;

/** Announces the customizer {@link IndexedFaceSetFactoryCustomizer} for JavaBeans introspection.
 * 
 * @author G. Paul Peters, 03.06.2010
 */
public class IndexedFaceSetFactoryBeanInfo extends SimpleBeanInfo {

	private final BeanDescriptor bd = new BeanDescriptor(IndexedFaceSetFactory.class, IndexedFaceSetFactoryCustomizer.class);
	
	public IndexedFaceSetFactoryBeanInfo() {
		bd.setDisplayName("Indexed Face Set Factory Explorer");
	}

	@Override
	public BeanDescriptor getBeanDescriptor() {
		return bd;
	}
}
