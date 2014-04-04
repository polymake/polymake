package de.jreality.audio.csound;

import java.io.IOException;

import csnd.CppSound;
import csnd.Csound;
import csnd.CsoundFile;
import csnd.CsoundMYFLTArray;
import de.jreality.audio.RingBuffer;
import de.jreality.audio.RingBufferSource;
import de.jreality.util.Input;

/**
 * Audio source that uses Csound as its synthesis engine.
 * 
 * @author brinkman
 *
 */
public class CsoundSource extends RingBufferSource {

	private CppSound csnd = new CppSound();
	private CsoundFile csf = csnd.getCsoundFile();
	private CsoundMYFLTArray spout;
	private int ksmps;
	private int nchnls;
	private int bufSize;
	private float scale;
	private float[] cumulativeBuffer;
	private boolean loop = false;

	public CsoundSource(String name, Input csd) throws IOException {
		super(name);
		csf.setCSD(csd.getContentAsString());
		initFields();
	}

	public CsoundSource(String name, Input orc, Input score) throws IOException {
		super(name);
		csf.setOrchestra(orc.getContentAsString());
		csf.setScore(score.getContentAsString());
		initFields();
	}

	private void initFields() {
		csf.setCommand("-n -d foo.orc foo.sco");
		csf.exportForPerformance();
		csnd.compile();
		ksmps = csnd.GetKsmps();
		nchnls = csnd.GetNchnls();
		bufSize = ksmps*nchnls;
		sampleRate = (int) csnd.GetSr();
		scale = (float) csnd.Get0dBFS();
		ringBuffer = new RingBuffer(sampleRate);
		spout = new CsoundMYFLTArray();
		spout.SetPtr(csnd.GetSpout());
		cumulativeBuffer = new float[ksmps];
	}

	public void setLoop(boolean loop) {
		this.loop = loop;
	}

	public Csound getCsound() {
		return csnd;
	}

	@Override
	protected void reset() {
		csnd.RewindScore();
	}

	@Override
	protected void writeSamples(int n) {
		for(int i=0; i<n; i+=ksmps) {
			if (csnd.PerformKsmps()!=0) {
				reset();
				if (!loop) {
					state = State.STOPPED;
					hasChanged = true;
					break;
				}
			}
			for(int j=0; j<ksmps; j++) {
				float v = 0;
				for(int k=j; k<bufSize; k+=ksmps) {
					v += spout.GetValue(k);
				}
				cumulativeBuffer[j] = v/scale;
			}
			ringBuffer.write(cumulativeBuffer, 0, ksmps);
		}
	}
}
