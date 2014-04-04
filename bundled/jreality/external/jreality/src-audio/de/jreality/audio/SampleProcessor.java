package de.jreality.audio;

import de.jreality.scene.data.SampleReader;
import de.jreality.shader.EffectiveAppearance;


/**
 * SampleReader implementation for chaining
 * 
 * Meant to be extended; this class simply passes audio without changing anything
 * 
 * @author brinkman
 *
 */
public class SampleProcessor implements SampleReader {

	protected SampleReader reader = null;

	/**
	 * Creates a new sample processor that reads from a given reader
	 * @param reader:  reader to draw samples from, frequently an instance of SampleProcessor
	 */
	public SampleProcessor(SampleReader reader) {
		this.reader = reader;
	}

	/**
	 * read properties from effective appearance
	 * 
	 * When overriding this method, you need to call super.setProperties(app) in most cases.
	 * 
	 * consider synchronization when overriding this method
	 * @param app
	 */
	public void setProperties(EffectiveAppearance app) {
		if (reader instanceof SampleProcessor) {
			((SampleProcessor) reader).setProperties(app);
		}
	}

	/**
	 * When overriding this method, you want to return (myResult || super.hasMore()) in most cases.
	 * 
	 * @return true if we have more nonzero samples to render, even if all future inputs are zero
	 */
	public boolean hasMore() {
		return (reader instanceof SampleProcessor) && ((SampleProcessor) reader).hasMore();
	}

	/**
	 * When overriding this method, you want to call super.clear() in most cases.
	 */
	public void clear() {
		reader.clear();
	}
	
	public int getSampleRate() {
		return reader.getSampleRate();
	}
	
	public int read(float[] buffer, int initialIndex, int samples) {
		return reader.read(buffer, initialIndex, samples);
	}
	
	public String toString() {
		return ((reader!=null) ? reader+", " : "")+super.toString();
	}
}
