package de.jreality.tutorial.tool;

import java.awt.Color;

import de.jreality.geometry.Primitives;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.Appearance;
import de.jreality.scene.PointSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.StorageModel;
import de.jreality.shader.DefaultGeometryShader;
import de.jreality.shader.DefaultLineShader;
import de.jreality.shader.DefaultPointShader;
import de.jreality.shader.DefaultPolygonShader;
import de.jreality.shader.ShaderUtility;
import de.jreality.tools.DragEventTool;
import de.jreality.tools.PointDragEvent;
import de.jreality.tools.PointDragListener;

public class DragEventTool01 {

	public static void main(String[] args) {
		SceneGraphComponent cmp = new SceneGraphComponent();

		cmp.setGeometry(Primitives.icosahedron());
		Appearance ap = new Appearance();
		cmp.setAppearance(ap);
		setupAppearance(ap);
		
		DragEventTool t = new DragEventTool();
		t.addPointDragListener(new PointDragListener() {

			public void pointDragStart(PointDragEvent e) {
				System.out.println("drag start of vertex no "+e.getIndex());				
			}

			public void pointDragged(PointDragEvent e) {
				PointSet pointSet = e.getPointSet();
				double[][] points=new double[pointSet.getNumPoints()][];
		        pointSet.getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray(points);
		        points[e.getIndex()]=e.getPosition();  
		        pointSet.setVertexAttributes(Attribute.COORDINATES,StorageModel.DOUBLE_ARRAY.array(3).createReadOnly(points));			
			}

			public void pointDragEnd(PointDragEvent e) {
			}
			
		});
		
		cmp.addTool(t);

		JRViewer.display(cmp);
	}

	private static void setupAppearance(Appearance ap) {
		DefaultGeometryShader dgs;
		DefaultPolygonShader dps;
		DefaultLineShader dls;
		DefaultPointShader dpts;
		dgs = ShaderUtility.createDefaultGeometryShader(ap, true);
		dgs.setShowFaces(true);
		dgs.setShowLines(true);
		dgs.setShowPoints(true);
		dps = (DefaultPolygonShader) dgs.createPolygonShader("default");
		dps.setDiffuseColor(Color.blue);
		dls = (DefaultLineShader) dgs.createLineShader("default");
		dls.setDiffuseColor(Color.yellow);
		dls.setTubeRadius(.03);
		dpts = (DefaultPointShader) dgs.createPointShader("default");
		dpts.setDiffuseColor(Color.red);
		dpts.setPointRadius(.05);
	}
}
