package de.jreality.audio.javasound;

import javax.sound.sampled.LineUnavailableException;

import de.jreality.audio.AmbisonicsSoundEncoder;
import de.jreality.audio.SoundEncoder;
import de.jreality.audio.util.Limiter;

public class StereoRenderer extends AbstractJavaSoundRenderer {

	private static final boolean LIMIT = true;
	byte[] buffer;
	float[] fbuffer;
	float[] fbuffer_lookAhead;
	
	private static final float W_SCALE = (float) Math.sqrt(0.5);
	private static final float Y_SCALE = 0.5f;
	
	
	@Override
	protected SoundEncoder createSoundEncoder() {
		return new AmbisonicsSoundEncoder() {
			public void finishFrame() {
				renderAmbisonics(bw, bx, by, bz);
			}
		};
	}
	
	@Override
	public void launch() throws LineUnavailableException {
		channels=2;
		openSourceDataLine();
		buffer = new byte[bufferLength];
		fbuffer = new float[2*frameSize]; // 2 channels
		fbuffer_lookAhead = new float[2*frameSize];
	
		super.launch();
	}

	public void renderAmbisonics(float[] wBuf, float[] xBuf, float[] yBuf, float[] zBuf) {
		if (LIMIT) renderAmbisonicsLimited(wBuf, xBuf, yBuf, zBuf);
		else renderAmbisonicsPlain(wBuf, xBuf, yBuf, zBuf);
	}
		

	Limiter limiter = new Limiter();
	
	public void renderAmbisonicsLimited(float[] wBuf, float[] xBuf, float[] yBuf, float[] zBuf) {
		
		for (int i = 0; i < frameSize; i++) {
			float w = wBuf[i] * W_SCALE;
			float y = yBuf[i] * Y_SCALE;
			fbuffer_lookAhead[2*i]=w+y;
			fbuffer_lookAhead[2*i+1]=w-y;
		}

		limiter.limit(fbuffer, fbuffer_lookAhead);
		
		JavaSoundUtility.floatToByte(buffer, fbuffer);
		writePCM(buffer, 0, bufferLength);
		
		swapBuffers();
	}

	protected void swapBuffers() {
		// swap buffers
		float[] tmpF = fbuffer;
		fbuffer = fbuffer_lookAhead;
		fbuffer_lookAhead = tmpF;
	}
	
	public void renderAmbisonicsPlain(float[] wBuf, float[] xBuf, float[] yBuf, float[] zBuf) {
		for (int i = 0; i < frameSize; i++) {
			float w = wBuf[i] * W_SCALE;
			float y = yBuf[i] * Y_SCALE;
			fbuffer[2*i]=w+y;
			fbuffer[2*i+1]=w-y;
		}
		JavaSoundUtility.floatToByte(buffer, fbuffer);
		writePCM(buffer, 0, bufferLength);
	}
	
}
