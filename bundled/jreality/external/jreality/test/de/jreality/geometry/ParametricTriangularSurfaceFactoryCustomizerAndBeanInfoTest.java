package de.jreality.geometry;

import org.junit.Before;

public class ParametricTriangularSurfaceFactoryCustomizerAndBeanInfoTest 
	extends AbstractGeometryFactoryCustomizerAndBeanInfoTest<ParametricTriangularSurfaceFactory, ParametricTriangularSurfaceFactoryCustomizer> {

	@Before
	public void turnOffUpdate() {
		customizer.setAlwaysCallUpdateMethdod(false);
	}

	@Override
	ParametricTriangularSurfaceFactory getFactory() {
		if (factory == null) factory = new ParametricTriangularSurfaceFactory();
		return factory;
	}

	@Override
	 ParametricTriangularSurfaceFactoryCustomizer getACustomizer() {
		return new ParametricTriangularSurfaceFactoryCustomizer(factory);
	}

	@Override
	int nbOfIntSliders() {
		return 0;
	}

	@Override
	int nbOfDblSliders() {
		return 0;
	}

	@Override
	int nbOfToggleButtons() {
		return 7;
	}

}
