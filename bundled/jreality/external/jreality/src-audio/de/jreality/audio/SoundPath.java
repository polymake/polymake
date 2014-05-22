package de.jreality.audio;

import de.jreality.math.Matrix;
import de.jreality.scene.data.SampleReader;
import de.jreality.shader.EffectiveAppearance;

/**
 * 
 * Represents the physical sound path from the sound source
 * to the microphone at (0,0,0). This class plays the role of a SoundShader
 * comparable to the various GeometryShaders and will be configured from an
 * EffectiveAppearance.
 * 
 * Sound paths are responsible for most of the audio processing, including
 * resampling, interpolation, distance cues, etc.
 * 
 * Convention: Specifying a speed of sound of zero or less means infinite speed
 * of sound, i.e., instantaneous propagation.
 * 
 * @author <a href="mailto:weissman@math.tu-berlin.de">Steffen Weissmann</a>
 *
 */
public interface SoundPath {
	
	void initialize(SampleReader reader, Interpolation.Factory factory);
	
	void setProperties(EffectiveAppearance eapp);
	
	/**
	 * 
	 * @param enc
	 * @param frameSize
	 * @param curPos         current source transformation matrix
	 * @param micInvMatrix   inverse of current microphone transformation matrix
	 * @param directionlessBuffer  cumulative buffer for directionless samples, intended for reverberation; may be null, indicating that there is no directionless processing
	 * @return true if the sound path is still holding samples to be rendered, e.g., due to propagation delays
	 */
	boolean processFrame(SoundEncoder enc, int frameSize, Matrix curPos, Matrix micInvMatrix, float[] directionlessBuffer);

	public interface Factory {
		SoundPath newSoundPath();
	}
}
