package de.jreality.audio;

import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;

public interface AudioRenderer {

	void setSceneRoot(SceneGraphComponent root);
	void setMicrophonePath(SceneGraphPath microphonePath);
	void setInterpolationFactory(Interpolation.Factory interpolationFactory);
	void setSoundPathFactory(SoundPath.Factory soundPathFactory);
	void launch() throws Exception;
	void shutdown() throws Exception;
	
}
