package de.jreality.audio;


/**
 * 
 * An AudioSource getting data from a precomputed sample buffer.
 * 
 * @author <a href="mailto:weissman@math.tu-berlin.de">Steffen Weissmann</a>
 *
 */
public class SampleBufferAudioSource extends RingBufferSource {

	protected float[] samples;
	protected int index;
	protected boolean loop;
	protected int nSamples;

	public SampleBufferAudioSource(String name, float[] sampleBuffer, int sampleRate, boolean loop) {
		super(name);
		this.loop = loop;
		this.sampleRate=sampleRate;
		samples=sampleBuffer;
		nSamples = samples.length;

		ringBuffer = new RingBuffer(sampleRate);
		reset();
	}

	protected void reset() {
		index = 0;
	}

	protected void writeSamples(int nRequested) {
		while (nRequested>0) {
			int n = nSamples-index;
			if (nRequested<n) {
				n = nRequested;
			}
			ringBuffer.write(samples, index, n);
			index += n;
			nRequested -= n;
			if (index>=nSamples) {
				if (loop) {
					index -= nSamples;
				} else {
					state = State.STOPPED;
					hasChanged = true;     // let listeners know that we're done
					reset();
					break;
				}
			}
		}
	}
}
