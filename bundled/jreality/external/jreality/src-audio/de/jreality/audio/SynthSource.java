package de.jreality.audio;


/**
 * 
 * Basic node for audio synthesis
 * 
 */
public abstract class SynthSource extends RingBufferSource {

	protected long index = 0; // keeps on ticking for about 15 million years at a sample rate of 192kHz; should be good enough for most practical purposes
	
	private float[] buffer = null;
	
	
	public SynthSource(String name, int samplerate) {
		super(name);
		this.sampleRate=samplerate;
		ringBuffer = new RingBuffer(samplerate);
	}
	
	@Override
	protected void writeSamples(int n) {
		if (buffer==null || buffer.length<n) {
			buffer = new float[n];
		}
		for (int i=0; i<n; i++) {
			buffer[i]=nextSample();
			index++;
		}
		ringBuffer.write(buffer, 0, n);
	}

	/**
	 * Override this method with the actual audio synthesis code
	 * @return
	 */
	protected abstract float nextSample();

	@Override
	protected void reset() {
		index=0;
	}
}
