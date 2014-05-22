package de.jreality.audio;


import de.jreality.scene.AudioSource;
import de.jreality.scene.data.SampleReader;

/**
 * A subclass of AudioSource using ring buffers.
 * 
 * @author brinkman
 *
 */
public abstract class RingBufferSource extends AudioSource {
	
	protected RingBuffer ringBuffer = null;
	protected int sampleRate = 0;
	

	public RingBufferSource(String name) {
		super(name);
	}	

	// write _at least_ n samples to ringBuffer if available, no sync necessary
	protected abstract void writeSamples(int n);
	
	protected int readSamples(RingBuffer.Reader reader, float buffer[], int initialIndex, int nSamples) {
		startReader();
		try {
			synchronized(this) {
				int needed = nSamples-reader.valuesLeft();
				if (needed>0 && state==State.RUNNING) {
					writeSamples(needed);
				}
			}
			return reader.read(buffer, initialIndex, nSamples);
		} finally {
			writingFinished();
			finishReader();
		}
	}
 
	public SampleReader createReader() {
		return new SampleReader() {
			private final RingBuffer.Reader reader = ringBuffer.createReader();
			public void clear() {
				reader.clear();
			}
			public int getSampleRate() {
				return sampleRate;
			}
			public int read(float[] buffer, int initialIndex, int nSamples) {
				return readSamples(reader, buffer, initialIndex, nSamples);
			}
		};
	}
}
