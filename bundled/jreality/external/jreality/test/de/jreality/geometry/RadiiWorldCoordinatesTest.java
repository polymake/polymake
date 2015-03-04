package de.jreality.geometry;

import static de.jreality.shader.CommonAttributes.FACE_DRAW;
import static de.jreality.shader.CommonAttributes.LINE_SHADER;
import static de.jreality.shader.CommonAttributes.POINT_SHADER;
import static de.jreality.shader.CommonAttributes.RADII_WORLD_COORDINATES;

import java.awt.EventQueue;

import de.jreality.math.MatrixBuilder;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.Appearance;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.Transformation;
import de.jreality.scene.Viewer;
import de.jreality.util.CameraUtility;

public class RadiiWorldCoordinatesTest {

	public static void main(String[] args) {
		Appearance app = new Appearance("World Coordinates");
		app.setAttribute(LINE_SHADER + "." + RADII_WORLD_COORDINATES, true);
		app.setAttribute(POINT_SHADER + "." + RADII_WORLD_COORDINATES, true);
		app.setAttribute(FACE_DRAW, false);
		final SceneGraphComponent c = new SceneGraphComponent("scaled");
		c.setAppearance(app);
		final Transformation T = new Transformation();
		c.setTransformation(T);
		c.setGeometry(Primitives.cube());
		final Viewer v = JRViewer.display(c);
		
		try {
			Thread.sleep(2000);
		} catch (InterruptedException e) {}
		
		Runnable r = new Runnable() {
			@Override
			public void run() {
				MatrixBuilder.euclidean().scale(0.1).assignTo(T);
				CameraUtility.encompass(v);		
			}
		};
		EventQueue.invokeLater(r);
	}

}
