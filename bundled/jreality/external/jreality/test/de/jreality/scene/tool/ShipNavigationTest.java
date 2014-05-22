package de.jreality.scene.tool;

import java.util.Arrays;

import de.jreality.geometry.BoundingBoxUtility;
import de.jreality.geometry.Primitives;
import de.jreality.math.MatrixBuilder;
import de.jreality.scene.Camera;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.tools.HeadTransformationTool;
import de.jreality.tools.ShipNavigationTool;
import de.jreality.ui.viewerapp.ViewerApp;
import de.jreality.util.PickUtility;
import de.jreality.util.Rectangle3D;

public class ShipNavigationTest {

	public static void main(String[] args) throws Exception {
		ShipNavigationTool snt = new ShipNavigationTool();
		SceneGraphComponent earth = Primitives.sphere(2, null); //Readers.read(Input.getInput("/net/MathVis/data/testData3D/jrs/earth.jrs"));
		PickUtility.assignFaceAABBTrees(earth);
		ViewerApp va = new ViewerApp(earth);
		va.update();
		Rectangle3D bb = BoundingBoxUtility.calculateBoundingBox(earth);
		snt.setCenter(true);
		snt.setCenter(bb.getCenter());
		System.out.println(Arrays.toString(bb.getCenter()));
		
		va.getToolSystem().getAvatarPath().getLastComponent().addTool(snt);
		va.getViewer().getCameraPath().getLastComponent().addTool(new HeadTransformationTool());
		MatrixBuilder.euclidean().translate(0, 2, 0).assignTo(va.getToolSystem().getAvatarPath().getLastComponent());
		MatrixBuilder.euclidean().translate(0, 1.7, 0).assignTo(va.getViewer().getCameraPath().getLastComponent());
		Camera c = (Camera) va.getViewer().getCameraPath().getLastElement();
		c.setNear(0.1);
		c.setFar(1000);
		va.display();
	}
}
