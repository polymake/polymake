package de.jreality.audio;

import java.util.Arrays;

/**
 * 
 * Extension of {@link AmbisonicsSoundEncoder} to second order Ambisonics; see
 * http://www.muse.demon.co.uk/ref/speakers.html for mathematical background.
 *
 */
public abstract class Ambisonics2ndOrderSoundEncoder extends AmbisonicsSoundEncoder {

	protected float[] br, bs, bt, bu, bv; // bw, bx, by, bz are already defined in AmbisonicsSoundEncoder
	
	public void startFrame(int framesize) {
		super.startFrame(framesize); // initialize bw, bx, by, bz
		
		if (br == null || br.length != framesize) {
			br=new float[framesize];
			bs=new float[framesize];
			bt=new float[framesize];
			bu=new float[framesize];
			bv=new float[framesize];
		}
		else {
			Arrays.fill(br, 0f);
			Arrays.fill(bs, 0f);
			Arrays.fill(bt, 0f);
			Arrays.fill(bu, 0f);
			Arrays.fill(bv, 0f);
		}
	}
	
	protected void encodeAmbiSample(float v, int idx, float x, float y, float z) {
		super.encodeAmbiSample(v, idx, x, y, z); // compute bw, bx, by, bz
		
		br[idx] += v*(1.5f*z*z-0.5);
		bs[idx] += v*(2f*z*x);
		bt[idx] += v*(2f*y*z);
		bu[idx] += v*(x*x-y*y);
		bv[idx] += v*(2f*x*y);
	}
}
