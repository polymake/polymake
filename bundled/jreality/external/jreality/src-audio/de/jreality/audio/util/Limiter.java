package de.jreality.audio.util;

/**
 * Simple limiter with one frame look ahead. Limits ramps along fbuffer
 * to match required gain for the look ahead buffer.
 * 
 * @author <a href="mailto:weissman@math.tu-berlin.de">Steffen Weissmann</a>
 *
 */
public class Limiter {

	private static final double RELEASE_FACTOR = 0.99;
	private static final int HOLD_COUNT = 16;
	float maxSignal=1f;
	int holdcnt;

	public void limit(float[] fbuffer, float[] fbuffer_lookAhead) {
		
		int framesize = fbuffer.length;
		
		float nextFrameMaxSignal = maxSignal;
		
		for (int i = 0; i < framesize; i++) {
			float abs = Math.abs(fbuffer_lookAhead[i]);
			if (abs > nextFrameMaxSignal) {
				nextFrameMaxSignal=abs;
				holdcnt=HOLD_COUNT;
			}
		}
		
		boolean rampUp = (nextFrameMaxSignal > maxSignal);

		float delta = Math.abs(nextFrameMaxSignal-maxSignal);
		
		float dd=0;
		if (!rampUp) {
			if (holdcnt == 0 && maxSignal > 1f) {
				//start ramp down...
				delta = - (float) (maxSignal*(1-RELEASE_FACTOR));
				dd = delta/framesize;
			} else {
				holdcnt--;
			}
		} else {
			dd = delta/framesize;
		}
		
		for (int i=0; i<framesize; i++) {
			if (maxSignal >= 1) maxSignal+=dd;
			else maxSignal = 1;
			fbuffer[i]/=maxSignal;
		}

	}
	
}
