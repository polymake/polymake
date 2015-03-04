package de.jreality.writer.blender.test;

import static de.jreality.shader.CommonAttributes.POINT_RADIUS;
import static de.jreality.shader.CommonAttributes.RADII_WORLD_COORDINATES;
import static de.jreality.shader.CommonAttributes.TUBE_RADIUS;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;

import de.jreality.geometry.IndexedLineSetFactory;
import de.jreality.io.JrScene;
import de.jreality.math.MatrixBuilder;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.Appearance;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.Viewer;
import de.jreality.shader.CommonAttributes;
import de.jreality.writer.WriterJRS;
import de.jreality.writer.blender.BlenderConnection;

public class BlenderTestTubesAndSpheres {

	public static void main(String[] args) throws IOException {
		SceneGraphComponent root = new SceneGraphComponent();
		Appearance rootAppearance = new Appearance("Root Apearance");
		rootAppearance.setAttribute(CommonAttributes.VERTEX_DRAW, true);
		rootAppearance.setAttribute(CommonAttributes.EDGE_DRAW, true);
		rootAppearance.setAttribute(CommonAttributes.LINE_SHADER + '.' + TUBE_RADIUS, 1.0);
		rootAppearance.setAttribute(CommonAttributes.POINT_SHADER + '.' + POINT_RADIUS, 0.1);
		root.setAppearance(rootAppearance);
		
		SceneGraphComponent scaledTubeRoot = new SceneGraphComponent("Scaled Tube");
		IndexedLineSetFactory ilsf = new IndexedLineSetFactory();
		ilsf.setVertexCount(2);
		ilsf.setEdgeCount(1);
		ilsf.setVertexCoordinates(new double[][]{
			{0,0,0}, {0,0,1}
		});
		ilsf.setEdgeIndices(new int[][]{{0,1}});
		ilsf.setVertexRelativeRadii(new double[]{1.0, 1.0});
		ilsf.update();
		scaledTubeRoot.setGeometry(ilsf.getGeometry());
		Appearance tubesAppearance = new Appearance("Scaled Tube Appearance");
		scaledTubeRoot.setAppearance(tubesAppearance);
		MatrixBuilder.euclidean().scale(0.5).assignTo(scaledTubeRoot);
		root.addChild(scaledTubeRoot);
		
		SceneGraphComponent transformRoot = new SceneGraphComponent("Scale Transform Root");
		MatrixBuilder.euclidean().scale(0.5).assignTo(transformRoot);
		root.addChild(transformRoot);
		
		SceneGraphComponent worldCoordinateTube = new SceneGraphComponent("World Coordinate Tube");
		worldCoordinateTube.setGeometry(ilsf.getGeometry());
		Appearance tubesWorldAppearance = new Appearance("World Tube Appearance");
		tubesWorldAppearance.setAttribute(CommonAttributes.LINE_SHADER + '.' + RADII_WORLD_COORDINATES, true);
		tubesWorldAppearance.setAttribute(CommonAttributes.POINT_SHADER + '.' + RADII_WORLD_COORDINATES, true);
		worldCoordinateTube.setAppearance(tubesWorldAppearance);
		MatrixBuilder.euclidean().scale(0.5).translate(-3, 0, 0).assignTo(worldCoordinateTube);
		transformRoot.addChild(worldCoordinateTube);
		
		Viewer v = JRViewer.display(root);
		
		JrScene scene = new JrScene(root);

		WriterJRS jrsWriter = new WriterJRS();
		jrsWriter.writeScene(scene, new FileOutputStream("testTubesAndSpheres.jrs"));
		
		// write blender file 
		BlenderConnection r = new BlenderConnection();
		File blenderFile = new File("testTubesAndSpheres.blend");
		r.writeFile(scene, blenderFile);
	}
	
}
