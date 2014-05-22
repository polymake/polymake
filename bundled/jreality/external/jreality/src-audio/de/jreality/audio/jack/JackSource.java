package de.jreality.audio.jack;

import java.nio.FloatBuffer;

import com.noisepages.nettoyeur.jack.JackException;
import com.noisepages.nettoyeur.jack.JackNativeClient;

import de.jreality.audio.RingBuffer;
import de.jreality.audio.RingBufferSource;


public class JackSource extends RingBufferSource implements JackProcessor {

	private final long key;
	
	public JackSource(String name, String target) throws JackException {
		super(name);
		sampleRate = JackNativeClient.getSampleRate();
		ringBuffer = new RingBuffer(sampleRate);
		key = JackManager.requestInputPorts(1, target);
		JackManager.addInput(this);
	}

	@Override
	protected void finalize() throws Throwable {
		JackManager.releasePorts(key);
		super.finalize();
	}
	
	public void process(FloatBuffer[] inBufs, FloatBuffer[] outBufs) {
		if (getState() == State.RUNNING) {
			FloatBuffer buffer = inBufs[JackManager.getPort(key)];
			buffer.rewind();
			ringBuffer.write(buffer);
		}
	}

	@Override
	protected void writeSamples(int n) {
		// do nothing; samples are written in process callback
	}
}
