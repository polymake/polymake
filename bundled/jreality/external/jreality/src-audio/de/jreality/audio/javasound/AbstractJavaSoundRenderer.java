package de.jreality.audio.javasound;

import java.io.File;
import java.io.IOException;

import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.LineUnavailableException;
import javax.sound.sampled.SourceDataLine;

import de.jreality.audio.AbstractAudioRenderer;
import de.jreality.audio.AudioBackend;
import de.jreality.audio.SoundEncoder;
import de.jreality.audio.WavFileWriter;

public abstract class AbstractJavaSoundRenderer extends AbstractAudioRenderer implements Runnable {

	private static final boolean WRITE_TO_FILE = false;
	private static final String AUDIO_FILE_NAME = "jraudio.wav";

	private SourceDataLine outputLine;

	private SoundEncoder encoder;

	protected int frameSize=512;
	
	protected int sampleRate;
	protected int channels;
	
	private boolean running;
	
	Thread soundThread=null;

	protected int bufferLength;

	private WavFileWriter wavFile;
	
	/**
	 * computes buffer length
	 * 
	 * @param sdl
	 * @throws LineUnavailableException 
	 */
	protected void openSourceDataLine() throws LineUnavailableException {
		AudioFormat af = JavaSoundUtility.outputFormat(channels);
		outputLine = JavaSoundUtility.createSourceDataLine(af);
		System.out.println(outputLine.getFormat());
		sampleRate = (int) af.getSampleRate();
		int bytesPerSample = (af.getSampleSizeInBits()+7)/8;
		bufferLength = frameSize * bytesPerSample * channels;
		//System.out.println("stereo out buffer size = "+outputLine.getBufferSize());
		
		// make the buffer of the outputLine twice as big as the backend buffer:
		outputLine.open(af, 2*bufferLength);
		
		//System.out.println("stereoOut bufferSize: requested="+bufferLength+" obtained="+outputLine.getBufferSize());
		
		/*
		if (bufferLength != outputLine.getBufferSize()) {
			bufferLength = outputLine.getBufferSize();
			System.out.println("Suggested buffer size not accepted. Adjusting frame size ["+frameSize+"] to "+(bufferLength/bytesPerSample));
			frameSize = bufferLength/bytesPerSample;
		}
		*/
		
		outputLine.start();
		
		if (WRITE_TO_FILE) try {
			System.out.println("try opening wav file...");
			wavFile = new WavFileWriter(af.getChannels(), sampleRate, af.getSampleSizeInBits(), new File(AUDIO_FILE_NAME));
			System.out.println("opened "+AUDIO_FILE_NAME);
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		
	}
	
	public void launch() throws LineUnavailableException {
		if (root == null || microphonePath == null) throw new IllegalStateException("need root and microphone path to launch");
		if (isRunning()) throw new IllegalStateException("JavaSound renderer already started...");
		encoder=createSoundEncoder();
		backend=new AudioBackend(root, microphonePath, sampleRate, interpolationFactory, soundPathFactory);
		startRenderThread();
	}

	protected abstract SoundEncoder createSoundEncoder();

	private void startRenderThread() {
		setRunning(true);
		soundThread=new Thread(this, "JavaSound render thread");
		soundThread.setPriority(Thread.MAX_PRIORITY);
		soundThread.setDaemon(true);
		soundThread.start();
	}

	public synchronized void shutdown() {
		setRunning(false);
		while (isRunning()) {
			try {
				wait(1);
			} catch (InterruptedException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
		soundThread=null;
		if (backend != null) backend.dispose();
		if (outputLine != null) outputLine.close();
	}

	public void run() {
		while (isRunning()) {
			backend.processFrame(encoder, frameSize);
		}
		System.out.println("JavaSound stopped");
	}

	public synchronized boolean isRunning() {
		return running;
	}

	private synchronized void setRunning(boolean b) {
		running=b;
	}
	
	public void setFrameSize(int fs) {
		frameSize=fs;
	}
	
	public int getFrameSize() {
		return frameSize;
	}

	protected void writePCM(byte[] buf, int offset, int len) {
		outputLine.write(buf, offset, len);
		if (wavFile != null) {
			try {
				wavFile.write(buf, offset, len);
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
	}
}
