package de.jreality.geometry;

import java.beans.BeanDescriptor;
import java.beans.SimpleBeanInfo;

/** Announces the customizer {@link ParametricTriangularSurfaceFactoryCustomizer} for JavaBeans introspection.
 * 
 * @author G. Paul Peters, 03.06.2010
 */
public class ParametricTriangularSurfaceFactoryBeanInfo extends SimpleBeanInfo {

	private final BeanDescriptor bd = new BeanDescriptor(ParametricTriangularSurfaceFactory.class, ParametricTriangularSurfaceFactoryCustomizer.class);
	
	public ParametricTriangularSurfaceFactoryBeanInfo() {
		bd.setDisplayName("Parametric Triangular Surface Factory Explorer");
	}

	@Override
	public BeanDescriptor getBeanDescriptor() {
		return bd;
	}
}
