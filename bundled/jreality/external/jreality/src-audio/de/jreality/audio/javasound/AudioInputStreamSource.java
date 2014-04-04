package de.jreality.audio.javasound;

import java.io.IOException;
import java.net.URL;

import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioFormat.Encoding;
import javax.sound.sampled.AudioInputStream;
import javax.sound.sampled.AudioSystem;
import javax.sound.sampled.UnsupportedAudioFileException;

import de.jreality.audio.RingBuffer;
import de.jreality.audio.RingBufferSource;
import de.jreality.util.Input;

/**
 * 
 * A RingBufferSource getting data from a JavaSound AudioInputStream. The loop-functionality
 * depends on the URL to reopen the resource when returning to the beginning; better use
 * CachedAudioInputStreamSource for sound loops (assuming that the sample array is not too long).
 * 
 * @author <a href="mailto:weissman@math.tu-berlin.de">Steffen Weissmann</a>
 *
 */
public class AudioInputStreamSource extends RingBufferSource {
	
	AudioInputStream audioStream;
	AudioFormat format;
	
	boolean loop;
	private URL url;
	
	public AudioInputStreamSource(String name, URL url, boolean loop) throws UnsupportedAudioFileException, IOException {
		this(name, AudioSystem.getAudioInputStream(Input.getInput(url).getInputStream()));
		this.url = url;
		this.loop = loop;
	}

	public AudioInputStreamSource(String name, Input input, boolean loop) throws UnsupportedAudioFileException, IOException {
		this(name, AudioSystem.getAudioInputStream(input.getInputStream()));
		this.loop = loop;
	}
	
	public AudioInputStreamSource(String name, AudioInputStream ain) {
		super(name);
		check(ain);
		sampleRate = (int) format.getSampleRate();
		ringBuffer = new RingBuffer(sampleRate);
	}

	private void check(AudioInputStream ain) {
		audioStream = ain;
		format = ain.getFormat();
		if (format.getEncoding() != Encoding.PCM_SIGNED) {
			System.out.println("converting from format "+format);
			AudioFormat baseFormat = format;
			float inSampleRate = baseFormat.getSampleRate();
			if (inSampleRate == AudioSystem.NOT_SPECIFIED) {
				inSampleRate = 44100;
			}
			format =
			    new AudioFormat(AudioFormat.Encoding.PCM_SIGNED,
			                    inSampleRate,
			                    16,
			                    baseFormat.getChannels(),
			                    baseFormat.getChannels() * 2,
			                    inSampleRate,
			                    false);
			System.out.println("to format "+format);
			audioStream = AudioSystem.getAudioInputStream(format, ain);
		}
	}
	
	@Override
	protected void reset() {
		if (url == null) throw new UnsupportedOperationException("reset not supported without URL");
		try {
			check(AudioSystem.getAudioInputStream(Input.getInput(url).getInputStream()));
		} catch (UnsupportedAudioFileException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}

	@Override
	protected void writeSamples(int nRequested) {
		
		int bytesPerSample = format.getSampleSizeInBits()/8;
		int frameSize = format.getFrameSize();
		
		byte[] buf = new byte[frameSize*nRequested];
		try {
			int read = readAMAP(nRequested, frameSize, buf);
			
			float[] fbuf = new float[nRequested];
			for (int i=0; i<read; i++) {
				fbuf[i] = JavaSoundUtility.getFloat(buf, frameSize*i, bytesPerSample, false);
			}
			ringBuffer.write(fbuf, 0, read);
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}

	private int readAMAP(int nRequested, int frameSize, byte[] buf) throws IOException {
		int read = audioStream.read(buf)/frameSize;
		while (read < nRequested) {
			// check if stream is at end:
			int nowRead = audioStream.read(buf, read*frameSize, (nRequested-read)*frameSize)/frameSize;
			if (nowRead == -1) { // stream at end
				if (loop) {
					reset();
				} else {
					state = State.STOPPED;  // to let listeners know that we're done
					reset();
					hasChanged = true;
				}
					
			} else {
				read += nowRead;
			}
		}
		return read;
	}

}
