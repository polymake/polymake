package de.jreality.audio.javasound;

import javax.sound.sampled.LineUnavailableException;

import de.jreality.audio.SoundEncoder;
import de.jreality.audio.VbapSoundEncoder;
import de.jreality.audio.util.Limiter;

public class VbapRenderer extends AbstractJavaSoundRenderer {

	byte[] buffer;
	
	float[] fbuffer;
	float[] fbuffer_lookAhead;

	double[][] speakers = new double[][]{
			{0.5, 0},
			{0.5, 0.5},
			{-1.5,1},
			{-1.5,-1},
			{0.5,-0.5},
	};
	int[] channelIDs = new int[]{4,0,2,3,1};

	@Override
	protected SoundEncoder createSoundEncoder() {
		return new VbapSoundEncoder(speakers.length, speakers, channelIDs) {
			public void finishFrame() {
				render(buf);
			}
		};
	}
	
	@Override
	public void launch() throws LineUnavailableException {
		channels = speakers.length;
		openSourceDataLine();
		buffer = new byte[bufferLength];
		fbuffer = new float[channels*frameSize];
		fbuffer_lookAhead = new float[channels*frameSize];

		super.launch();
	}

	Limiter limiter = new Limiter();
	
	public void render(float[] surroundSamples) {
		System.arraycopy(surroundSamples, 0, fbuffer_lookAhead, 0, surroundSamples.length);
		limiter.limit(fbuffer, fbuffer_lookAhead);
		JavaSoundUtility.floatToByte(buffer, fbuffer);
		writePCM(buffer, 0, bufferLength);
		// swap buffers
		float[] tmpF = fbuffer;
		fbuffer = fbuffer_lookAhead;
		fbuffer_lookAhead = tmpF;
	}

}
