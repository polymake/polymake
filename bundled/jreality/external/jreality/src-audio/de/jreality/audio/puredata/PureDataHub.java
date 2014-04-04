package de.jreality.audio.puredata;

import org.puredata.core.PdBase;

import de.jreality.audio.RingBuffer;
import de.jreality.scene.AudioSource;
import de.jreality.scene.data.SampleReader;

public class PureDataHub {

	private final static Object lock = new Object();
	private final static int BLOCKSIZE = PdBase.blockSize();
	private static int sampleRate, nChannels;
	private static final float[] inBuffer = new float[0];
	private static float[] outBuffer;
	private static PdSource[] channels;
	
	private static class PdSource extends AudioSource {
		private RingBuffer ringBuffer;
		
		public PdSource(String name, int sampleRate) {
			super(name);
			ringBuffer = new RingBuffer(sampleRate);
		}
		
		private int readSamples(RingBuffer.Reader reader, float buffer[], int initialIndex, int nSamples) {
			startReader();
			try {
				synchronized(lock) {
					int needed = nSamples - reader.valuesLeft();
					if (needed > 0 && state == State.RUNNING) {
						writeSamples(needed);
					}
				}
				return reader.read(buffer, initialIndex, nSamples);
			} finally {
				writingFinished();
				finishReader();
			}
		}
		
		@Override
		public SampleReader createReader() {
			return new SampleReader() {
				private final RingBuffer.Reader reader = ringBuffer.createReader();
				public void clear() {
					reader.clear();
				}
				public int getSampleRate() {
					return sampleRate;
				}
				public int read(float[] buffer, int initialIndex, int nSamples) {
					return readSamples(reader, buffer, initialIndex, nSamples);
				}
			};
		}
		
		@Override
		public void start() {
			PureDataHub.start();
		}
		
		@Override
		public void pause() {
			PureDataHub.pause();
		}
		
		@Override
		public void stop() {
			PureDataHub.stop();
		}
		
		private void superStart() {
			super.start();
		}
		
		private void superPause() {
			super.pause();
		}
		
		private void superStop() {
			super.stop();
		}
	}
	
	private PureDataHub() {
		// do nothing
	}
	
	public static void init(int sampleRate, int nChannels) {
		PureDataHub.sampleRate = sampleRate;
		PureDataHub.nChannels = nChannels;
		outBuffer = new float[nChannels * BLOCKSIZE];
		channels = new PdSource[nChannels];
		for (int i = 0; i < nChannels; i++) {
			channels[i] = new PdSource("pd channel " + i, sampleRate);
		}
		PdBase.openAudio(0, nChannels, sampleRate);
		PdBase.computeAudio(true);
	}
	
	public static AudioSource getPureDataSource(int i) {
		return channels[i];
	}
	
	public static int getSampleRate() {
		return sampleRate;
	}
	
	public static int getChannels() {
		return nChannels;
	}
	
	public static void start() {
		synchronized (lock) {
			for (PdSource s: channels) {
				s.superStart();
			}			
		}
	}
	
	public static void pause() {
		synchronized (lock) {
			for (PdSource s: channels) {
				s.superPause();
			}			
		}
	}
	
	public static void stop() {
		synchronized (lock) {
			for (PdSource s: channels) {
				s.superStop();
			}			
		}
	}
	
	private static void writeSamples(int n) {
		for (int i = 0; i < n; i += BLOCKSIZE) {
			PdBase.processRaw(inBuffer, outBuffer);
			for (int j = 0; j < nChannels; j++) {
				channels[j].ringBuffer.write(outBuffer, j * BLOCKSIZE, BLOCKSIZE);
			}
		}
	}
}
