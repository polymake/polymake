package de.jreality.geometry;

import org.junit.Before;

public class IndexedLineSetFactoryCustomizerAndBeanInfoTest 
	extends AbstractGeometryFactoryCustomizerAndBeanInfoTest<IndexedLineSetFactory, IndexedLineSetFactoryCustomizer> {

	@Before
	public void turnOffUpdate() {
		customizer.setAlwaysCallUpdateMethdod(false);
	}

	@Override
	IndexedLineSetFactory getFactory() {
		if (factory == null) factory = new IndexedLineSetFactory();
		return factory;
	}

	@Override
	 IndexedLineSetFactoryCustomizer getACustomizer() {
		return new IndexedLineSetFactoryCustomizer(factory);
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
		return 2;
	}

}
