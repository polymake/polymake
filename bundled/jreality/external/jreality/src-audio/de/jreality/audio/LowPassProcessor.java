package de.jreality.audio;

import de.jreality.scene.data.SampleReader;
import de.jreality.shader.EffectiveAppearance;


/**
 * 
 * Simple processor with a low-pass filter, mostly as a proof of concept.
 * 
 * @author brinkman
 *
 */
public class LowPassProcessor extends SampleProcessor {

	private LowPassFilter lpf;
	
	public LowPassProcessor(SampleReader reader) {
		super(reader);
		lpf = new LowPassFilter(reader.getSampleRate());
	}
	
	public void setCutOff(float cutOff) {
		lpf.setCutOff(cutOff);
	}

	public float getCutoff() {
		return lpf.getCutOff();
	}
	
	public void clear() {
		super.clear();
		lpf.initialize(0);
	}

	public int read(float[] buffer, int initialIndex, int nSamples) {
		int nRead = reader.read(buffer, initialIndex, nSamples);
		
		for(int i = initialIndex; i<nRead; i++) {
			buffer[i] = lpf.nextValue(buffer[i]);
		}
		
		return nRead;
	}

	public void setProperties(EffectiveAppearance app) {
		super.setProperties(app);
		setCutOff(app.getAttribute("lowPassProcessorCutOff", 44000));
	}
	
	public boolean hasMore() {
		return lpf.hasMore() || super.hasMore();
	}
}
