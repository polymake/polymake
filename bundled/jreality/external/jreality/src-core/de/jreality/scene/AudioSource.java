package de.jreality.scene;


import de.jreality.scene.data.SampleReader;
import de.jreality.scene.event.AudioEvent;
import de.jreality.scene.event.AudioEventMulticaster;
import de.jreality.scene.event.AudioListener;

/**
 * The core of audio for jReality.  The basic idea is that a scene graph component can have an audio source
 * attached to it.  Audio renderers request mono sample readers from audio sources, one for each occurrence of
 * the source in the scene graph.  An audio source keeps track of time in terms of the number of samples requested
 * so far.  Readers can read samples concurrently, and sample requests are queued and managed so that an audio
 * source only writes as many samples as the fastest renderer requests.
 * 
 * Samples are floats in the range from -1 to 1.
 * 
 * @author brinkman
 *
 */
public abstract class AudioSource extends SceneGraphNode {
	
	protected transient AudioEventMulticaster audioListener = new AudioEventMulticaster();
	protected transient Boolean hasChanged = false;

	public enum State {RUNNING, STOPPED, PAUSED}
	protected State state = State.STOPPED;

	public AudioSource(String name) {
		super(name);
	}
	
	/**
	 * 
	 * The return value must be a new sample reader for each call; readers must be able to
	 * operate in parallel.
	 * 
	 * @return a new sample reader for this source
	 */
	public abstract SampleReader createReader();
	
	// reset audio engine; no sync necessary, only to be called from stop method
	protected void reset() {
		// default: do nothing
	}


// *************************************** transport functions *****************************************

	/**
	 * set the state of the node.
	 *       
	 * @param state  the new state of the audio source
	 */
	public void setState(State state) {
		switch (state) {
		case RUNNING:
			start();
			break;
		case PAUSED:
			pause();
			break;
		case STOPPED:
			stop();
			break;
		default:
			break;
		}
	}
	
	public State getState() {
		return state;
	}
	
	public void start() {
		startWriter();
		try {
			if (state != State.RUNNING) {
				state = State.RUNNING;
				hasChanged = true;
			}
		} finally {
			finishWriter();
		}
	}
	public void stop() {
		startWriter();
		try {
			if (state != State.STOPPED) {
				state = State.STOPPED;
				reset();
				hasChanged = true;
			}
		} finally {
			finishWriter();
		}
	}
	public void pause() {
		startWriter();
		try {
			if (state != State.PAUSED) {
				state = State.PAUSED;
				hasChanged = true;
			}
		} finally {
			finishWriter();
		}
	}

	
// ************************************** the rest is boilerplate *************************************

	public void accept(SceneGraphVisitor v) {
		startReader();
		try {
			v.visit(this);
		} finally {
			finishReader();
		}
	}
	static void superAccept(AudioSource a, SceneGraphVisitor v) {
		a.superAccept(v);
	}
	private void superAccept(SceneGraphVisitor v) {
		super.accept(v);
	}
	public void addAudioListener(AudioListener listener) {
		startReader();
		try {
			audioListener.add(listener);
		} finally {
			finishReader();
		}
	}
	public void removeAudioListener(AudioListener listener) {
		startReader();
		try {
			audioListener.remove(listener);
		} finally {
			finishReader();
		}
	}
	protected void writingFinished() {
		if (hasChanged && audioListener != null) {
			audioListener.audioChanged(new AudioEvent(this));
		}
		hasChanged = false;
	}
}
