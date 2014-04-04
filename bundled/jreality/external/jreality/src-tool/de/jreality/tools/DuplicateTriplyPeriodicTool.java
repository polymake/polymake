package de.jreality.tools;

import java.util.List;

import de.jreality.math.Matrix;
import de.jreality.math.MatrixBuilder;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphNode;
import de.jreality.scene.pick.PickResult;
import de.jreality.scene.tool.AbstractTool;
import de.jreality.scene.tool.InputSlot;
import de.jreality.scene.tool.ToolContext;
import de.jreality.util.SceneGraphUtility;

/**
 * This tool listens to the "Duplication" virtual device and allows to replicate
 * a SceneGraphComponent <code>sg</code> (to which the tool is attached) along an
 * orthogonal lattice (aligned with the axes, but with arbitrary lattice spacing).
 * It is assumed that <code>sg</code> has a single parent <code>p</code>. All the
 * the newly generated copies of <code>sg</code> (they are by reference only) will
 * be descendants of <code>p</code> as well.
 * 
 * @author Ulrich
 *
 */
public class DuplicateTriplyPeriodicTool extends AbstractTool {

	private double[] latticeSpacing;
	private double[] center;
	private int count = 0;

	public DuplicateTriplyPeriodicTool(
			double latticeX, double latticeY, double latticeZ,
			double centerX, double centerY, double centerZ
	) {
		super(InputSlot.getDevice("Duplication"));
		latticeSpacing = new double[]{latticeX, latticeY, latticeZ};
		center = new double[]{centerX, centerY, centerZ};
	}

	public void activate(ToolContext tc) {
		PickResult pick = tc.getCurrentPick();
		double[] coords = pick.getObjectCoordinates();
		
		// the tool is attached to the fundamental domain
		SceneGraphComponent domain = tc.getRootToToolComponent().getLastComponent();
		
		// find the SceneGraphComponent that carries the translation to the picked copy
		List<SceneGraphNode> rootToLocal = tc.getRootToLocal().toList();
		int n = rootToLocal.size();
		int j = n-3;
		while (!SceneGraphUtility.getPathsBetween(domain, rootToLocal.get(j)).isEmpty()) {
			j--;
		}
		SceneGraphComponent copy = (SceneGraphComponent)rootToLocal.get(j);
		Matrix m = new Matrix(tc.getRootToLocal().getMatrix(new Matrix().getArray(),j+1,n-1));
		
		// evaluate pick coordinates relative to that component
		coords = m.multiplyVector(coords);
		
		// find out in which direction to translate
		for (int i=0; i<3; i++) {
			coords[i] = (coords[i]-center[i])/latticeSpacing[i];
		}
		int dir=0;
		if (Math.abs(coords[1])>Math.abs(coords[0])) dir=1;
		if (Math.abs(coords[2])>Math.abs(coords[dir])) dir=2;
		double[] trans=new double[3];
		trans[dir]=Math.signum(coords[dir]) * latticeSpacing[dir];
		
		// make a component with the translation we decided on
		SceneGraphComponent newCopy = new SceneGraphComponent();
		newCopy.setName("copy"+count);
		count++;
		MatrixBuilder.euclidean().translate(trans).assignTo(newCopy);
		
		// add the fundamental domain as a child to this component
		newCopy.addChild(domain);
		
		// make the new component a child of the old one
		copy.addChild(newCopy);
	}
}
