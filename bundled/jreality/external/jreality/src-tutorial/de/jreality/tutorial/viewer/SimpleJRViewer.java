package de.jreality.tutorial.viewer;

import java.io.IOException;

import de.jreality.geometry.Primitives;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.SceneGraphNode;

public class SimpleJRViewer {
	public static void main(String[] args) throws IOException {
		SceneGraphNode read = Primitives.cube(true); //Readers.read(SimpleJRViewer.class.getResource(face.stl"));
		JRViewer.display(read);
	}

}
