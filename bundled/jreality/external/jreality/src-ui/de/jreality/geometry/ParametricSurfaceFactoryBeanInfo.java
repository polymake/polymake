package de.jreality.geometry;

import java.beans.BeanDescriptor;
import java.beans.SimpleBeanInfo;

/** Announces the customizer {@link ParametricSurfaceFactoryCustomizer} for JavaBeans introspection.
 * 
 * @author G. Paul Peters, 03.06.2010
 */
public class ParametricSurfaceFactoryBeanInfo extends SimpleBeanInfo {

	private final BeanDescriptor bd = new BeanDescriptor(ParametricSurfaceFactory.class, ParametricSurfaceFactoryCustomizer.class);
	
	public ParametricSurfaceFactoryBeanInfo() {
		bd.setDisplayName("Parametric Surface Factory Explorer");
	}

	@Override
	public BeanDescriptor getBeanDescriptor() {
		return bd;
	}
}
