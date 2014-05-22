package de.jreality.audio.puredata;

import java.io.File;
import java.net.URL;

import org.puredata.core.PdBase;

import de.jreality.geometry.Primitives;
import de.jreality.math.MatrixBuilder;
import de.jreality.plugin.JRViewer;
import de.jreality.plugin.JRViewer.ContentType;
import de.jreality.scene.AudioSource;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.tools.DraggingTool;


public class PdExample {

	public static SceneGraphComponent getAudioComponent() throws Exception {
		PureDataHub.init(44100, 1);
		URL url = PdExample.class.getResource("test.pd");
		PdBase.openPatch(new File(url.toURI()));
		final AudioSource source = PureDataHub.getPureDataSource(0);
		SceneGraphComponent audioComponent = new SceneGraphComponent("monolith");
		audioComponent.setAudioSource(source);
		audioComponent.setGeometry(Primitives.cube());
		MatrixBuilder.euclidean().translate(0, 5, 0).scale(2, 4.5, .5).assignTo(audioComponent);
		audioComponent.addTool(new DraggingTool());
		source.start();
		return audioComponent;
	}

	public static void main(String[] args) throws Exception {
		JRViewer v = new JRViewer();
		v.addBasicUI();
		v.addAudioSupport();
		v.addVRSupport();
		v.addContentSupport(ContentType.TerrainAligned);
		v.setContent(getAudioComponent());
		v.startup();
	}
}
