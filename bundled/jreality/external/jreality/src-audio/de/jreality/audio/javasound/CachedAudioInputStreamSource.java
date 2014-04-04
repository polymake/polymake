package de.jreality.audio.javasound;

import java.io.IOException;

import javax.sound.sampled.AudioInputStream;
import javax.sound.sampled.AudioSystem;
import javax.sound.sampled.UnsupportedAudioFileException;

import de.jreality.audio.SampleBufferAudioSource;
import de.jreality.util.Input;

/**
 * 
 * An AudioSource getting data from a JavaSound AudioInputStream. The whole
 * Stream is read into a buffer which makes it easy to loop the sample - but
 * this requires much memory for longer samples. 
 * 
 * @author <a href="mailto:weissman@math.tu-berlin.de">Steffen Weissmann</a>
 *
 */
public class CachedAudioInputStreamSource extends SampleBufferAudioSource {
	
	public CachedAudioInputStreamSource(String name, AudioInputStream ain, boolean loop) {
		super(name, JavaSoundUtility.readAudioFile(ain), (int) ain.getFormat().getSampleRate(), loop);
	}

	public CachedAudioInputStreamSource(String name, Input input, boolean loop) throws UnsupportedAudioFileException, IOException {
		this(name, AudioSystem.getAudioInputStream(input.getInputStream()), loop);
	}
}
