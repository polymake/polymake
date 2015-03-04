package de.jreality.tutorial.misc;

import java.io.IOException;

import de.jreality.math.MatrixBuilder;
import de.jreality.plugin.JRViewer;
import de.jreality.plugin.JRViewer.ContentType;
import de.jreality.plugin.content.ContentAppearance;
import de.jreality.plugin.content.ContentLoader;
import de.jreality.plugin.content.ContentTools;
import de.jreality.reader.Readers;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.shader.CommonAttributes;
import de.jreality.util.Input;

public class MatheonBearExample {
	public static void main(String[] args) {
		// customize a JRViewer to have Virtual Reality support (skyboxes, terrain, etc)
		JRViewer v = new JRViewer();
		v.addBasicUI();
		v.addVRSupport();
		v.addContentSupport(ContentType.TerrainAligned);
		SceneGraphComponent bear = new SceneGraphComponent("Bear");
		try {
			bear = Readers.read(Input.getInput("jrs/baer.jrs"));
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		// the bear is a huge geometry and we cannot afford drawing
		// edges and vertices while having real time audio...
		bear.getAppearance().setAttribute(CommonAttributes.VERTEX_DRAW, false);
		bear.getAppearance().setAttribute(CommonAttributes.EDGE_DRAW, false);
		MatrixBuilder.euclidean().rotateX(-Math.PI/2).translate(0,0,2).scale(0.002).assignTo(bear);
		
		v.setContent(bear);
		v.registerPlugin(new ContentAppearance());
		v.registerPlugin(new ContentLoader());
		v.registerPlugin(new ContentTools());
		v.startup();
	}

}
