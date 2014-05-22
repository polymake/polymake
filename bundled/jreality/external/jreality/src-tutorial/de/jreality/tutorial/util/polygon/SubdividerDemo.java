package de.jreality.tutorial.util.polygon;

import de.jreality.plugin.JRViewer;
import de.jreality.scene.SceneGraphComponent;

public class SubdividerDemo {

	/**
	 * Create a sequence of points, on a circle in the xy-plane.
	 * @param n number of points
	 * @param r the radius
	 * @return the points
	 */
	public static double[][] circle(int n, double r) {
		double[][] verts = new double[n][3];
		double dphi = 2.0*Math.PI/n;
		for (int i=0; i<n; i++) {
			verts[i][0]=r*Math.cos(i*dphi);
			verts[i][1]=r*Math.sin(i*dphi);
		}
		return verts;
	}
	
	public static void main(String[] args) {
		// create control points, subdivider and the view:
		DragPointSet dps = new DragPointSet(circle(5,1));
		//dps.setClosed(false);
		SubdividedPolygon sub = new SubdividedPolygon(dps);
		PointSequenceView subView = new PointSequenceView(sub);
		
		// attach the corresponding components to a common base component:
		SceneGraphComponent root = new SceneGraphComponent();
		root.addChild(dps.getBase());
		root.addChild(subView.getBase());
		
		// display:
		JRViewer.display(root);
	}
	
}
