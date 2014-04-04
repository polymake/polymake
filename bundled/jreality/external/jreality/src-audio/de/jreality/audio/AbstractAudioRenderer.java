package de.jreality.audio;

import de.jreality.audio.Interpolation.Factory;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;

public abstract class AbstractAudioRenderer implements AudioRenderer {

	protected SceneGraphPath microphonePath;
	protected SceneGraphComponent root;

	protected Interpolation.Factory interpolationFactory = AudioAttributes.DEFAULT_INTERPOLATION_FACTORY;
	protected SoundPath.Factory soundPathFactory = AudioAttributes.DEFAULT_SOUNDPATH_FACTORY;

	protected AudioBackend backend;
	
	public void setSceneRoot(SceneGraphComponent root) {
		this.root=root;
	}
	
	public void setMicrophonePath(SceneGraphPath microphonePath) {
		this.microphonePath=microphonePath;
	}

	public void setInterpolationFactory(Factory interpolationFactory) {
		this.interpolationFactory=interpolationFactory;
	}
	
	public void setSoundPathFactory(SoundPath.Factory soundPathFactory) {
		this.soundPathFactory=soundPathFactory;
	}
	
}
