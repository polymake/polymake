package de.jreality.audio;

import de.jreality.scene.data.SampleReader;

/**
 * 
 * Sound encoder interface; implementations of this interface are the very last step in the audio
 * processing chain.  In particular, the encodeSample callback encodes one sample at a time, with
 * a location given in euclidean coordinates, with the assumption that the speakers will be located
 * in a euclidean space.  Any audio processing (e.g., attenuation, reverberation, interpolation,
 * handling of noneuclidean coordinates) is the responsibility of implementations of {@link SampleReader}
 * or {@link SoundPath}.
 *
 */
public interface SoundEncoder {
	void startFrame(int framesize);
	
	/**
	 * 
	 * Encode one sample with location.
	 * 
	 * @param v:       sample, already processed for attenuation, reverberation, interpolation, etc.
	 * @param idx:     index of sample
	 * @param r:       distance of sound source from listener
	 * @param x, y, z: direction of sound source relative to listener; (x, y, z) is a unit vector
	 */
	void encodeSample(float v, int idx, float r, float x, float y, float z);
	
	/**
	 * 
	 * Encode one directionless sample.
	 * 
	 * @param v
	 */
	void encodeSample(float v, int idx);
	
	void finishFrame();
}
