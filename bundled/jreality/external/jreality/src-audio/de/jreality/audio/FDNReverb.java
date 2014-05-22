package de.jreality.audio;

import java.util.Arrays;

import de.jreality.scene.data.SampleReader;
import de.jreality.shader.EffectiveAppearance;


/**
 * First attempt to build a reverberator with a feedback delay network...
 * 
 * @author brinkman
 *
 */
public class FDNReverb extends SampleProcessor {

	private FDNParameters parameters;
	private float[][] delayLines;
	private int[] lineIndices;
	private float[] outBuffer, inBuffer;
	
	public FDNReverb(SampleReader reader) {
		super(reader);
		setParameters(AudioAttributes.DEFAULT_FDN_PARAMETERS);
	}

	public void setProperties(EffectiveAppearance app) {
		super.setProperties(app);
		FDNParameters params = (FDNParameters) app.getAttribute(AudioAttributes.FDN_PARAMETER_KEY, AudioAttributes.DEFAULT_FDN_PARAMETERS, FDNParameters.class);
		if (params!=parameters) {
			setParameters(params);
		}
		float reverbTime = app.getAttribute(AudioAttributes.REVERB_TIME_KEY, AudioAttributes.DEFAULT_REVERB_TIME);
		if (reverbTime!=params.getReverbTime()) {
			params.setReverbTime(reverbTime);
		}
	}

	public synchronized void setParameters(FDNParameters params) {
		int n = params.numberOfLines();
		int sr = reader.getSampleRate();
		
		parameters = params;
		lineIndices = new int[n];
		delayLines = new float[n][];
		inBuffer = new float[n];
		outBuffer = new float[n];
		
		for(int i=0; i<n; i++) {
			delayLines[i] = new float[(int) (params.delayTime(i)*sr+.5)];
		}
	}

	public void clear() {
		super.clear();
		for(float[] line: delayLines) {
			Arrays.fill(line, 0);
		}
	}

	public synchronized int read(float[] buffer, int i0, int samples) {
		int nRead = reader.read(buffer, i0, samples);
		int n = parameters.numberOfLines();
		for(int i=0; i<nRead; i++) {
			float v = 0;
			for(int j=0; j<n; j++) {
				v += (outBuffer[j] = getValue(j));
			}
			parameters.map(inBuffer, outBuffer);
			for(int j=0; j<n; j++) {
				setValue(j, buffer[i0] + inBuffer[j]);
				advanceIndex(j);
			}
			buffer[i0++] = v/n;
		}
		return nRead;
	}

	private float getValue(int j) {
		return delayLines[j][lineIndices[j]];
	}
	
	private float setValue(int j, float v) {
		return delayLines[j][lineIndices[j]] = v;
	}
	
	private void advanceIndex(int j) {
		int i = lineIndices[j]+1;
		lineIndices[j] = (i<delayLines[j].length) ? i : i-delayLines[j].length;
	}
}
