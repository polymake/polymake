package de.jreality.audio.csound;

import java.io.IOException;

import de.jreality.scene.AudioSource;
import de.jreality.scene.data.SampleReader;
import de.jreality.util.Input;

public class TestCsound {
	public static void main(String[] args) throws IOException {
		AudioSource s = new CsoundSource("foo", Input.getInput("sound/trapped.csd"));
		float[] buf = new float[5000];
		SampleReader r = s.createReader();
		s.start();
		int N = 0;
		while (s.getState()==AudioSource.State.RUNNING) {
			int n = r.read(buf, 0, buf.length);
			N+=n;
			System.err.println(n+", "+N);
		}
	}
}
