package de.jreality.geometry;

import org.junit.Before;

public class IndexedFaceSetFactoryCustomizerAndBeanInfoTest 
	extends AbstractGeometryFactoryCustomizerAndBeanInfoTest<IndexedFaceSetFactory, IndexedFaceSetFactoryCustomizer> {

	@Before
	public void turnOffUpdate() {
		customizer.setAlwaysCallUpdateMethdod(false);
	}

	@Override
	IndexedFaceSetFactory getFactory() {
		if (factory == null) factory = new IndexedFaceSetFactory();
		return factory;
	}

	@Override
	 IndexedFaceSetFactoryCustomizer getACustomizer() {
		return new IndexedFaceSetFactoryCustomizer(factory);
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
