package de.jreality.audio;

import java.util.Arrays;

import de.jreality.scene.data.SampleReader;
import de.jreality.shader.EffectiveAppearance;


/**
 * 
 * Schroeder reverberator, implementation based on (but slightly different from) the reverb opcode of Csound.
 * 
 * @author brinkman
 *
 */
public class SchroederReverb extends SampleProcessor {

	private static final float[] delays = {0.0297f, 0.0371f, 0.0411f, 0.0437f, 0.09683f, 0.03292f};
	private float[] coeffs = new float[6];
	private float[][] delayLines = new float[6][];
	private int[] lineIndices = new int[6];
	private float reverbTime = AudioAttributes.DEFAULT_REVERB_TIME;

	
	public SchroederReverb(SampleReader reader) {
		super(reader);
		int sampleRate = reader.getSampleRate();
		for(int i = 0; i<6; i++) {
			delayLines[i] = new float[(int) (sampleRate*delays[i]*2+0.5)];
		}
		setReverbTime(reverbTime);
	}

	public void setProperties(EffectiveAppearance app) {
		super.setProperties(app);
		float reverbTime = app.getAttribute(AudioAttributes.REVERB_TIME_KEY, AudioAttributes.DEFAULT_REVERB_TIME);
		if (reverbTime!=getReverbTime()) {
			setReverbTime(reverbTime);
		}
	}

	public void setReverbTime(float reverbTime) {
		this.reverbTime = reverbTime;
		float q0 = (float) Math.log(0.001);
		float q = q0/reverbTime;
		for(int i = 0; i < 4; i++) {
			coeffs[i] = (float) Math.exp(q*delays[i]);
		}
		coeffs[4] = (float) Math.exp(q0*delays[4]/0.005);
		coeffs[5] = (float) Math.exp(q0*delays[5]/0.0017);
	}

	public float getReverbTime() {
		return reverbTime;
	}
	
	public void clear() {
		super.clear();
		for(int i = 0; i < 6; i++) {
			Arrays.fill(delayLines[i], 0);
		}
	}

	public int read(float[] buf, int initialIndex, int samples) {
		final int nRead = reader.read(buf, initialIndex, samples);
		final int terminalIndex = initialIndex+nRead;

		for(int i = initialIndex; i<terminalIndex; i++) {
			float acc = 0;
			float v = buf[i];

			// four comb filters
			for(int j = 0; j < 4; j++) {
				float w = currentFilterValue(j);
				acc += w;
				setFilterValue(j, coeffs[j]*w + v);
				advanceFilterIndex(j);
			}
			        
			// all-pass filter
			float y1 = currentFilterValue(4);
			float z = coeffs[4]*y1 + acc;      // feedback
			setFilterValue(4, z);
			advanceFilterIndex(4);
			y1 -= coeffs[4] * z;               // feedforward
			
			// another all-pass filter
			float y2 = currentFilterValue(5);
			z = coeffs[5]*y2 + y1;
			setFilterValue(5, coeffs[5]*y2 + y1);
			advanceFilterIndex(5);
			buf[i] = y2 - coeffs[5]*z;
		}
		return nRead;
	}

	private void advanceFilterIndex(int j) {
		int i = lineIndices[j]+1;
		lineIndices[j] = (i<delayLines[j].length) ? i : i-delayLines[j].length;
	}

	private float currentFilterValue(int j) {
		return delayLines[j][lineIndices[j]];
	}

	private float setFilterValue(int j, float v) {
		return delayLines[j][lineIndices[j]] = v;
	}
}
