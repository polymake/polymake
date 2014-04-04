package de.jreality.audio;

import java.util.Arrays;

import de.jreality.scene.data.SampleReader;
import de.jreality.shader.EffectiveAppearance;


/**
 * Simple but efficient pitch shifter based on B14.sampler.rockafella.pd from
 * "Theory and Technique of Electronic Music" by Miller Puckette.
 * 
 * TODO: make it sound better (add low-pass filter when shifting up?)
 * 
 * @author brinkman
 *
 */
public class ShiftProcessor extends SampleProcessor {
	private static final float WINDOW_SIZE = 0.025f;
	
	private float[] inBuf;
	private int bufSize;
	private int writeIndex = 0;
	private int readIndex = 0;
	private int index1, index2;
	private int windowSize;
	private float halfWindow;
	private float alpha = 0;
	private float phase1;
	private Interpolation inter1 = new Interpolation.Cubic(), inter2 = new Interpolation.Cubic();
	private boolean reset1, reset2;
	private float qEnv;

	
	public ShiftProcessor(SampleReader reader) {
		super(reader);
		int sr = reader.getSampleRate();
		bufSize = sr;
		inBuf = new float[bufSize];
		windowSize = (int) (WINDOW_SIZE*sr);
		halfWindow = (float) windowSize/2;
		phase1 = 0;
		readIndex = bufSize-windowSize;
		reset1 = reset2 = true;
		qEnv = (float) Math.PI/windowSize;
	}

	public void setProperties(EffectiveAppearance app) {
		super.setProperties(app);
		float q = app.getAttribute(AudioAttributes.PITCH_SHIFT_KEY, AudioAttributes.DEFAULT_PITCH_SHIFT);
		setPitchShift(q);
	}

	public void setPitchShift(float q) {
		if (q<.25 || q>4) {
			throw new IllegalArgumentException("pitch shift out of range: "+q);
		}
		alpha = q-1;
	}
	
	public void clear() {
		super.clear();
		inter1.reset();
		inter2.reset();
		Arrays.fill(inBuf, 0);
	}

	public int read(float[] buffer, int initialIndex, int samples) {
		int nRead = readInput(samples);
		for(int i=0; i<nRead; i++) {
			float phase2 = (phase1<halfWindow) ? phase1+halfWindow : phase1-halfWindow;
			int p1 = (int) phase1;
			int p2 = (int) phase2;
			int j1 = (readIndex+p1) % bufSize;
			int j2 = (readIndex+p2) % bufSize;
			
			if (reset1) {
				index1 = j1;
				reset1 = false;
			} else while (index1!=j1) {
				inter1.put(inBuf[index1++]);
				if (index1>=bufSize) {
					index1 -= bufSize;
				}
			}
			if (reset2) {
				index2 = j2;
				reset2 = false;
			} else while (index2!=j2) {
				inter2.put(inBuf[index2++]);
				if (index2>=bufSize) {
					index2 -= bufSize;
				}
			}

			buffer[initialIndex+i] = inter1.get(phase1-p1)*envelope(phase1)+inter2.get(phase2-p2)*envelope(phase2);
			
			float p = phase1+alpha;
			if (phase1<halfWindow && p>=halfWindow) {
				reset2 = true;
			} else if (phase1>=halfWindow && p<halfWindow) {
				reset2 = true;
			}
			phase1 = p;
			if (phase1>=windowSize) {
				phase1 -= windowSize;
				reset1 = true;
			} else if (phase1<0) {
				phase1 += windowSize;
				reset1 = true;
			}
			readIndex++;
			if (readIndex>=bufSize) {
				readIndex -= bufSize;
			}
		}
		return nRead;
	}

	private float envelope(float t) {
		return (float) Math.sin(qEnv*t);
	}

	private int readInput(int samples) {
		int nRead;
		if (bufSize-writeIndex<samples) {
			nRead = reader.read(inBuf, writeIndex, bufSize-writeIndex);
			writeIndex += nRead;
			if (writeIndex>=bufSize) {
				writeIndex = reader.read(inBuf, 0, samples-nRead);
				nRead += writeIndex;
			}
		} else {
			nRead = reader.read(inBuf, writeIndex, samples);
			writeIndex += nRead;
			if (writeIndex>=bufSize) {
				writeIndex -= bufSize;
			}
		}
		return nRead;
	}
}
