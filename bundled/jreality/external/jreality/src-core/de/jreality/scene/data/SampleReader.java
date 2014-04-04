package de.jreality.scene.data;

/**
 * Sample reader interface.  Sample readers are intended to be chained, e.g., going from an AudioSource to
 * a sample rate converter to a low-pass filter.
 * 
 * @author brinkman
 *
 */
public interface SampleReader {
	/**
	 * 
	 * @return sample rate of output
	 */
	public int getSampleRate();
	
	/**
	 * Writes nSamples samples to the given buffer, starting at initialIndex, if possible
	 * 
	 * @param buffer
	 * @param initialIndex
	 * @param nSamples
	 * @return number of samples written (less than or equal to nSamples)
	 */
	public int read(float buffer[], int initialIndex, int nSamples);
	
	public void clear();
}
