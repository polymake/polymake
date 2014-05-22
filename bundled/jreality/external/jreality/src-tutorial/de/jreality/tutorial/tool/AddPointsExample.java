package de.jreality.tutorial.tool;

import java.util.ArrayList;
import java.util.List;

import de.jreality.geometry.IndexedLineSetFactory;
import de.jreality.math.Matrix;
import de.jreality.math.Rn;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.tool.AbstractTool;
import de.jreality.scene.tool.InputSlot;
import de.jreality.scene.tool.ToolContext;
import de.jreality.toolsystem.ToolUtility;

public class AddPointsExample extends AbstractTool {

	List<double[]> points = new ArrayList<double[]>();
	IndexedLineSetFactory lsf = new IndexedLineSetFactory();
	
	double offset = 5;
	
	public AddPointsExample() {
		// two initial points:
		addCurrentSlot(InputSlot.SHIFT_LEFT_BUTTON, "add a new point");
		
		updateGeometry();
	}

	@Override
	public void perform(ToolContext tc) {
		if (!tc.getAxisState(InputSlot.SHIFT_LEFT_BUTTON).isPressed()) return;
		
		// determine the pointer transformation:
		// translation is the mouse pointer on the near clipping plane
		// z-axis is the direction of the mouse ray out of the screen
		// for a 6DOF input device, it is the position/orientation of the device
		Matrix m = new Matrix(tc.getTransformationMatrix(InputSlot.POINTER_TRANSFORMATION));
				
		// we compute the coordinates of the new point in world coordinates
		double[] foot = m.getColumn(3);
		double[] dir = m.getColumn(2);
		double[] offset = Rn.times(null, -5, dir);
		double[] newPoint = Rn.add(null, foot, offset);
		
		// now we transform the world coordinates to the coordinate system of the tool component
		points.add(ToolUtility.worldToLocal(tc, newPoint));
		
		updateGeometry();
		
	}

	private void updateGeometry() {
		int n = points.size();
		
		// set new vertices
		lsf.setVertexCount(n);
		if (n>0) lsf.setVertexCoordinates(points.toArray(new double[0][]));

		if (n>1) {
			// compute and set new edge indices:
			int[][]  idx = new int[n-1][2];
			for (int i=1; i<n; i++) {
				idx[i-1][0] = i-1;
				idx[i-1][1] = i;
			}
	
			lsf.setEdgeCount(n-1);
			lsf.setEdgeIndices(idx);
		}		
		lsf.update();
	}
	
	public static void main(String[] args) {

		AddPointsExample example = new AddPointsExample();
		
		SceneGraphComponent cmp = new SceneGraphComponent();
		cmp.setGeometry(example.lsf.getGeometry());
		cmp.addTool(example);
		
		JRViewer.display(cmp);
	}
	
}
