package de.jreality.audio;

public interface DistanceCueFactory {
	
	/**
	 * creates an instance of class DistanceCue
	 * 
	 * @param sampleRate:  the sample rate at which the cue operates
	 * @return distance cue initialized for the given sample rate
	 */
	DistanceCue getInstance(float sampleRate);
}
