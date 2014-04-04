package de.jreality.audio;

import de.jreality.scene.data.SampleReader;

public interface SampleProcessorFactory {
	/**
	 * Creates an instance of class SampleProcessor
	 * 
	 * @param reader:  constructor argument for sample processor
	 * @return sample processor for given reader
	 */
	SampleProcessor getInstance(SampleReader reader);
}
