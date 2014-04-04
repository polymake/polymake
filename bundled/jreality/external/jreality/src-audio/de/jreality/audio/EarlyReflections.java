package de.jreality.audio;

import java.util.Arrays;

import de.jreality.scene.data.SampleReader;

/**
 * Simple simulation of early reflections, based on Moorer's "About This Reverberation Business."
 * 
 * @author brinkman
 *
 */
public class EarlyReflections extends SampleProcessor {

	private static final float[] tapTimes = {.0199f, .0354f, .0389f, .0414f, .0699f, .0796f}; // numbers from Table 3
	private static final float[] gains = {1.02f, .818f, .635f, .719f, .267f, .242f};
	private static final int nTaps = tapTimes.length;
	private int[] offsets = new int[nTaps];
	
	private float[] delayLine;
	private long silentCount = 0;
	private int maxDelay;
	private int index = 0;

	public EarlyReflections(SampleReader reader) {
		super(reader);
		maxDelay = 0;
		for(int i=0; i<nTaps; i++) {
			int n = offsets[i] = (int) (reader.getSampleRate()*tapTimes[i]+0.5);
			if (n>maxDelay) {
				maxDelay = n;
			}
		}
		delayLine = new float[maxDelay];
	}

	public void clear() {
		super.clear();
		Arrays.fill(delayLine, 0);
		silentCount = 0;
	}

	public int read(float[] buffer, int initialIndex, int nSamples) {
		int nRead = reader.read(buffer, initialIndex, nSamples);
		for(int i=initialIndex; i<initialIndex+nSamples; i++) {
			float u = buffer[i];
			if (Math.abs(u)<AudioAttributes.HEARING_THRESHOLD) {
				silentCount++;
			} else {
				silentCount = 0;
			}
			for(int j=0; j<nTaps; j++) {
				u += delayLine[(index+offsets[j]) % maxDelay]*gains[j];
			}
			delayLine[index++] = buffer[i];
			if (index>=maxDelay) {
				index -= maxDelay;
			}
			buffer[i] = u;
		}
		return nRead;
	}
	
	public boolean hasMore() {
		return (silentCount<maxDelay) || super.hasMore();
	}
}
