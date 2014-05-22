package de.jreality.geometry;

import org.junit.Before;

public class HeightFieldFactoryCustomizerAndBeanInfoTest 
	extends AbstractGeometryFactoryCustomizerAndBeanInfoTest<HeightFieldFactory, HeightFieldFactoryCustomizer> {

	@Before
	public void turnOffUpdate() {
		customizer.setAlwaysCallUpdateMethdod(false);
	}

	@Override
	HeightFieldFactory getFactory() {
		if (factory == null) factory = new HeightFieldFactory();
		return factory;
	}

	@Override
	 HeightFieldFactoryCustomizer getACustomizer() {
		return new HeightFieldFactoryCustomizer(factory);
	}

	@Override
	int nbOfIntSliders() {
		return 2;
	}

	@Override
	int nbOfDblSliders() {
		return 4;
	}

	@Override
	int nbOfToggleButtons() {
		return 11;
	}

}
