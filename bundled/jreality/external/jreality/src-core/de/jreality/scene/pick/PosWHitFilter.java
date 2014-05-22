package de.jreality.scene.pick;

import de.jreality.math.Rn;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.Viewer;
import de.jreality.util.CameraUtility;

public class PosWHitFilter implements HitFilter {
	
	public PosWHitFilter(Viewer v) {
		setViewer(v);
	}
	
	Viewer viewer;
	SceneGraphPath camPath;
	double[] world2ndc;
	public void setViewer(Viewer v)	{
		viewer = v;
		update();
	}

	public void update() {
		camPath = viewer.getCameraPath();
		if (camPath == null) return;
    	double[] world2cam = camPath.getInverseMatrix(null);
    	world2ndc = Rn.times(null, CameraUtility.getCameraToNDC(viewer), world2cam);
	}
	
	public boolean accept(double[] from, double[] to, PickResult h) {
		if (world2ndc == null) return false;
      	double[] ndcCoords = Rn.matrixTimesVector(null, world2ndc, h.getWorldCoordinates());
      	boolean posW = ndcCoords[3] >= 0;
//      	System.err.println("Pos w = "+posW);
      	return posW;
	}

}
