package de.jreality.geometry;

import org.junit.Before;

public class ParametricSurfaceFactoryCustomizerAndBeanInfoTest 
	extends AbstractGeometryFactoryCustomizerAndBeanInfoTest<ParametricSurfaceFactory, ParametricSurfaceFactoryCustomizer> {

	@Before
	public void turnOffUpdate() {
		customizer.setAlwaysCallUpdateMethdod(false);
	}

	@Override
	ParametricSurfaceFactory getFactory() {
		if (factory == null) factory = new ParametricSurfaceFactory();
		return factory;
	}

	@Override
	 ParametricSurfaceFactoryCustomizer getACustomizer() {
		return new ParametricSurfaceFactoryCustomizer(factory);
	}

	@Override
	int nbOfIntSliders() {
		return 2;
	}

	@Override
	int nbOfDblSliders() {
		return 8;
	}

	@Override
	int nbOfToggleButtons() {
		return 11;
	}

}
