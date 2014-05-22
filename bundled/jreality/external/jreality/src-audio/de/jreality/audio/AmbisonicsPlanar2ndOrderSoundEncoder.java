package de.jreality.audio;

import java.util.Arrays;

/**
 * 
 * Sound encoder for second order planar Ambisonics; see http://www.muse.demon.co.uk/ref/speakers.html
 * for mathematical background.
 *
 */
public abstract class AmbisonicsPlanar2ndOrderSoundEncoder implements SoundEncoder {

	protected static final float W_SCALE = (float) Math.sqrt(0.5);
	protected float[] bw, bx, by, bu, bv;
	
	public void startFrame(int framesize) {
		if (bw == null || bw.length != framesize) {
			bw=new float[framesize];
			bx=new float[framesize];
			by=new float[framesize];
			bu=new float[framesize];
			bv=new float[framesize];
		} else {
			Arrays.fill(bw, 0f);
			Arrays.fill(bx, 0f);
			Arrays.fill(by, 0f);
			Arrays.fill(bu, 0f);
			Arrays.fill(bv, 0f);
		}
	}
	
	public abstract void finishFrame();

	public void encodeSample(float v, int idx, float r, float x, float y, float z) {
		// The point (x, y, z) in graphics corresponds to (-z, -x, y) in Ambisonics.
		float rp = (float) Math.sqrt(z*z+x*x);

		if (rp>1e-6f) {
			encodeAmbiSample(v, idx, -z/rp, -x/rp);
		} else {
			encodeSample(v, idx);
		}
	}

	public void encodeSample(float v, int idx) {
		bw[idx] += v*W_SCALE;
	}
	
	protected void encodeAmbiSample(float v, int idx, float x, float y) {
		encodeSample(v, idx);
		bx[idx] += v*x;
		by[idx] += v*y;
		bu[idx] += v*(x*x-y*y);
		bv[idx] += v*(2f*x*y);
	}
}