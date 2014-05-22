package de.jreality.tutorial.tform;

import de.jreality.geometry.Primitives;
import de.jreality.math.Matrix;
import de.jreality.math.MatrixBuilder;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.event.TransformationEvent;
import de.jreality.scene.event.TransformationListener;
import de.jreality.tools.RotateTool;
import de.jreality.util.SceneGraphUtility;

public class TransformationListenerExample {

	public static void main(String[] args) {
		SceneGraphComponent world = SceneGraphUtility.createFullSceneGraphComponent("world");
		final SceneGraphComponent child1 =  SceneGraphUtility.createFullSceneGraphComponent("child1"),
			child2 = SceneGraphUtility.createFullSceneGraphComponent("child2"),
			child21 = SceneGraphUtility.createFullSceneGraphComponent("child21");
		world.addChildren(child1, child2);
		child2.addChild(child21);
		MatrixBuilder.euclidean().translate(-1.5, 0, 0).assignTo(world);
		
		child1.setGeometry(Primitives.coloredCube());
		child21.setGeometry(Primitives.coloredCube());
		final Matrix translate = new Matrix(MatrixBuilder.euclidean().translate(3, 0, 0).getArray());
		translate.assignTo(child2);
		
		// allow separate rotation of the left cube (child1)
		child1.addTool(new RotateTool());
		
		child1.getTransformation().addTransformationListener(new TransformationListener() {

			// this listener guarantees that the right cube's (child2's) transformation
			// is the inverse of the left one ( child1),  modulo  a translation
			public void transformationMatrixChanged(TransformationEvent ev) {
				Matrix childMatrix = new Matrix(child1.getTransformation().getMatrix());
				childMatrix.invert();
				childMatrix.assignTo(child21);
			}
			
		});
		
		JRViewer.display(world);
	}

}
