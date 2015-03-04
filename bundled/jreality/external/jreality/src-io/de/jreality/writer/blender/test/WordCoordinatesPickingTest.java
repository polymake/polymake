package de.jreality.writer.blender.test;

import static de.jreality.scene.tool.InputSlot.LEFT_BUTTON;
import static de.jreality.shader.CommonAttributes.POINT_RADIUS;
import static de.jreality.shader.CommonAttributes.POINT_SHADER;
import static de.jreality.shader.CommonAttributes.RADII_WORLD_COORDINATES;
import static de.jreality.shader.CommonAttributes.VERTEX_DRAW;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import de.jreality.geometry.Primitives;
import de.jreality.math.MatrixBuilder;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.Appearance;
import de.jreality.scene.PointSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.DoubleArray;
import de.jreality.tools.ActionTool;

public class WordCoordinatesPickingTest {

	public static void main(String[] args) {
		SceneGraphComponent root = new SceneGraphComponent("root");
		MatrixBuilder.euclidean().scale(2.0).assignTo(root);
		
		PointSet ps = Primitives.point(new double[]{0,0,0});
//		ps.setVertexAttributes(Attribute.RELATIVE_RADII, new DoubleArray(new double[]{1}));
		
		SceneGraphComponent geomRoot = new SceneGraphComponent("Geom root");
		geomRoot.setGeometry(ps);
		Appearance app = new Appearance("app");
		app.setAttribute(VERTEX_DRAW, true);
		app.setAttribute(POINT_SHADER + '.' + RADII_WORLD_COORDINATES, true);
		app.setAttribute(POINT_SHADER + '.' + POINT_RADIUS, 1.0);
		geomRoot.setAppearance(app);
		root.addChild(geomRoot);
		
//		SceneGraphComponent geomRoot2 = new SceneGraphComponent("Geom root2");
//		geomRoot2.setGeometry(ps);
//		Appearance app2 = new Appearance("app");
//		app2.setAttribute(VERTEX_DRAW, true);
//		app2.setAttribute(POINT_SHADER + '.' + RADII_WORLD_COORDINATES, false);
//		app2.setAttribute(POINT_SHADER + '.' + POINT_RADIUS, 1.0);
//		geomRoot2.setAppearance(app2);
//		MatrixBuilder.euclidean().translate(1, 0, 0).assignTo(geomRoot2);
//		root.addChild(geomRoot2);
		
		ActionTool tool = new ActionTool(LEFT_BUTTON);
		root.addTool(tool);
		tool.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				System.out.println(e);
			}
		});
		
		JRViewer.display(root);
	}
	
}
