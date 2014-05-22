package de.jreality.audio;

import de.jreality.shader.EffectiveAppearance;

/**
 * Simple interface for encapsulating various distance cues, such as volume attenuation,
 * low-pass filtering, reverberation, etc.  To be used as an appearance property, with
 * a usable set of simple implementations for most common purposes.
 * 
 * @author brinkman
 *
 */
public interface DistanceCue {

	public static abstract class Attenuation implements DistanceCue {
		public void setSampleRate(float sr) {}
		public boolean hasMore() { return false; }
		public void setProperties(EffectiveAppearance app) {}
		public void reset() {}
	}
	
	public static final class CONSTANT extends Attenuation {
		public float nextValue(float v, float r, float x, float y, float z) {
			return v;
		}
	}
	
	public static final class LINEAR extends Attenuation {
		public float nextValue(float v, float r, float x, float y, float z) {
			return v/Math.max(r, 1);
		}
	}
	
	public static final class EXPONENTIAL extends Attenuation {
		public float nextValue(float v, float r, float x, float y, float z) {
			return v/(float) Math.exp(r);
		}
	}
	
	public static final class CONICAL extends Attenuation {
		public float nextValue(float v, float r, float xMic, float yMic, float zMic) {
			return v*zMic*zMic;
		}
	}
	
	public static final class CARDIOID extends Attenuation {
		public float nextValue(float v, float r, float xMic, float yMic, float zMic) {
			return 0.5f*v*(1+zMic);  // v*(1+cos t)/2
		}
	}
	
	public static final class LOWPASS extends LowPassFilter implements DistanceCue {
		private float freq = AudioAttributes.DEFAULT_DISTANCE_LOWPASS_FREQ;
		public float nextValue(float v, float r, float x, float y, float z) {
			setCutOff(freq/(1+r));
			return nextValue(v);
		}
		public void reset() {
			initialize(0);
		}
		public void setProperties(EffectiveAppearance app) {
			freq = app.getAttribute(AudioAttributes.DISTANCE_LOWPASS_KEY, AudioAttributes.DEFAULT_DISTANCE_LOWPASS_FREQ);
		}
	}
	
	
	void setSampleRate(float sampleRate);
	
	/**
	 * @return true if there will be audible output in the future, even if all future inputs are zero
	 */
	boolean hasMore();
	
	/**
	 * Computes the next value, based on the new sample v and the distance r, as well as the location of
	 * the microphone relative to the sound source.
	 * 
	 * The microphone position is intended for sound sources with directional characteristics.  Since
	 * directional characteristics will not be used routinely, the sound path is not expected to interpolate
	 * xMic, yMic, and zMic.  Any such operations are the responsibility of implementations of this interface.
	 * 
	 * @param v sample
	 * @param r distance from observer when sample is heard (in microphone coordinates)
	 * @param x, y, z  direction of microphone (unit vector in source coordinates)
	 * @return updated value based on v and r
	 */
	float nextValue(float v, float r, float xMic, float yMic, float zMic);
	
	void setProperties(EffectiveAppearance app);
	
	void reset();
}
