package de.jreality.audio.jack;

import java.nio.FloatBuffer;

/**
 * Interface for sinks and sources interacting with {@link JackManager}.
 * 
 * @author brinkman
 */
public interface JackProcessor {
	public void process(FloatBuffer inBufs[], FloatBuffer outBufs[]);
}
