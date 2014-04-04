package de.jreality.audio.jack;


import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import de.jreality.geometry.Primitives;
import de.jreality.math.MatrixBuilder;
import de.jreality.plugin.JRViewer;
import de.jreality.plugin.JRViewer.ContentType;
import de.jreality.scene.AudioSource;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.tools.ActionTool;
import de.jreality.tools.DraggingTool;


public class TestJack {

	public static SceneGraphComponent getAudioComponent() throws Exception {
		final JackSource source = new JackSource("foo", "");
		source.start();
		
		SceneGraphComponent audioComponent = new SceneGraphComponent("monolith");
		audioComponent.setAudioSource(source);
		audioComponent.setGeometry(Primitives.cube());
		MatrixBuilder.euclidean().translate(0, 5, 0).scale(2, 4.5, .5).assignTo(audioComponent);
	
		ActionTool actionTool = new ActionTool("PanelActivation");
		actionTool.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				if (source.getState() == AudioSource.State.RUNNING) {
					source.pause();
				} else {
					source.start();
				}
			}
		});
		audioComponent.addTool(actionTool);
		audioComponent.addTool(new DraggingTool());
		return audioComponent;
	}

	
	public static void main(String[] args) throws Exception {
		JRViewer v = new JRViewer();
		v.addBasicUI();
		v.addAudioSupport();
		v.addVRSupport();
		v.setPropertiesFile("TestJack.jrw");
		v.addContentSupport(ContentType.TerrainAligned);
		v.setContent(getAudioComponent());
		v.startup();
	}
	
}
