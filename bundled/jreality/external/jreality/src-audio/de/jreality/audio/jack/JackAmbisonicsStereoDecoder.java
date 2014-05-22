package de.jreality.audio.jack;

import java.nio.FloatBuffer;

import com.noisepages.nettoyeur.jack.JackException;
import com.noisepages.nettoyeur.jack.JackNativeClient;

/**
 * An Ambisonics stereo decoder for Jack, mostly for testing on desktop systems; reads an Ambisonics
 * B-signal and writes a stereo signal.  In a production environment, one should use a serious decoder
 * such as Fons Adriaensen's AmbDec.  For mathematical background, see
 * http://www.muse.demon.co.uk/ref/speakers.html.
 * 
 * @author brinkman
 *
 */
public class JackAmbisonicsStereoDecoder {

	private static final float wScale = (float) Math.sqrt(0.5);
	private static final float yScale = 0.5f;
	
	private JackAmbisonicsStereoDecoder() {
		// not to be instantiated
	}
	
	public static void main(String args[]) throws InterruptedException, JackException {
		//JRViewer.getLastJRViewer();
		JackNativeClient client = new JackNativeClient("StereoDecoder", 4, 2) {
			@Override
			protected void process(FloatBuffer[] inBufs, FloatBuffer[] outBufs) {
				FloatBuffer bw = inBufs[0];
				FloatBuffer by = inBufs[2];
				FloatBuffer left = outBufs[0];
				FloatBuffer right = outBufs[1];

				int n = left.capacity();
				for(int i = 0; i<n; i++) {
					float w = bw.get()*wScale;
					float y = by.get()*yScale;
					
					left.put(w+y);
					right.put(w-y);
				}
			}
		};
		client.connectOutputPorts("");
		
		while (true) {
			Thread.sleep(100);
		}
	}
}
