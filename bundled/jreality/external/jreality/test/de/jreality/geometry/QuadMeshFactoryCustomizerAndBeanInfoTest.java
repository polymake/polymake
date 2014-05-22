package de.jreality.geometry;

import org.junit.Before;

public class QuadMeshFactoryCustomizerAndBeanInfoTest 
	extends AbstractGeometryFactoryCustomizerAndBeanInfoTest<QuadMeshFactory, QuadMeshFactoryCustomizer> {

	@Before
	public void turnOffUpdate() {
		customizer.setAlwaysCallUpdateMethdod(false);
	}

	@Override
	QuadMeshFactory getFactory() {
		if (factory == null) factory = new QuadMeshFactory();
		return factory;
	}

	@Override
	 QuadMeshFactoryCustomizer getACustomizer() {
		return new QuadMeshFactoryCustomizer(factory);
	}

	@Override
	int nbOfIntSliders() {
		return 0;
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
