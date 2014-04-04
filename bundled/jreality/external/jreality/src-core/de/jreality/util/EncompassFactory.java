package de.jreality.util;

import de.jreality.geometry.BoundingBoxUtility;
import de.jreality.math.Matrix;
import de.jreality.math.MatrixBuilder;
import de.jreality.math.Pn;
import de.jreality.math.Rn;
import de.jreality.scene.Camera;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;

public class EncompassFactory {
	SceneGraphPath avatarPath,  scenePath,  cameraPath;
	double margin = 0.0;
	int metric = Pn.EUCLIDEAN;
	boolean clippingPlanes = true, stereoParameters = true;
	
	public void update() {

	    Rectangle3D bounds = BoundingBoxUtility.calculateBoundingBox(scenePath.getLastComponent());
	    if (bounds.isEmpty()) return;
	    Matrix rootToScene = new Matrix();
	    scenePath.getMatrix(rootToScene.getArray(), 0, scenePath.getLength()-2);
	    Rectangle3D worldBounds = bounds.transformByMatrix(new Rectangle3D(), rootToScene.getArray());
	    Rectangle3D avatarBounds = worldBounds.transformByMatrix(new Rectangle3D(), avatarPath.getInverseMatrix(null));
	    double [] e = avatarBounds.getExtent();
	    double radius = Rn.euclideanNorm(e);
	    double [] c = avatarBounds.getCenter();
	    // TODO: read viewing angle from camera
	    c[2] += margin*radius;
	    //Rn.times(c, margin, c);
	    // add head height to c[1]
	    Matrix camMatrix = new Matrix();
	    cameraPath.getInverseMatrix(camMatrix.getArray(), avatarPath.getLength());
	    
	    Camera camera = ((Camera)cameraPath.getLastElement());
	    if (clippingPlanes) {
			camera.setFar(margin*3*radius);
		    camera.setNear(.3*radius);
	    }
	    if (!stereoParameters || SystemProperties.isPortal) return;
	    SceneGraphComponent avatar = avatarPath.getLastComponent();
	    Matrix m = new Matrix(avatar.getTransformation());
	    if (camera.isPerspective()) {
		    MatrixBuilder.init(m, metric).translate(c).translate(camMatrix.getColumn(3)).assignTo(avatar);	    	
			camera.setFocus(Math.abs(m.getColumn(3)[2]) ); 		//focus);
	    } else {
			double ww = (e[1] > e[0]) ? e[1] : e[0];
			double focus =   ww / Math.tan(Math.PI*(camera.getFieldOfView())/360.0);
			camera.setFocus(Math.abs(focus) ); 		//focus);	    	
	    }
		camera.setEyeSeparation(camera.getFocus()/12.0);		// estimate a reasonable separation based on the focal length	
//			System.err.println("setting focus to "+camera.getFocus());
		}

	public SceneGraphPath getAvatarPath() {
		return avatarPath;
	}

	public void setAvatarPath(SceneGraphPath avatarPath) {
		this.avatarPath = avatarPath;
	}

	public SceneGraphPath getScenePath() {
		return scenePath;
	}

	public void setScenePath(SceneGraphPath scenePath) {
		this.scenePath = scenePath;
	}

	public SceneGraphPath getCameraPath() {
		return cameraPath;
	}

	public void setCameraPath(SceneGraphPath cameraPath) {
		this.cameraPath = cameraPath;
	}

	public double getMargin() {
		return margin;
	}

	public void setMargin(double margin) {
		this.margin = margin;
	}

	public int getMetric() {
		return metric;
	}

	public void setMetric(int metric) {
		this.metric = metric;
	}

	public boolean isClippingPlanes() {
		return clippingPlanes;
	}

	public void setClippingPlanes(boolean clippingPlanes) {
		this.clippingPlanes = clippingPlanes;
	}

	public boolean isStereoParameters() {
		return stereoParameters;
	}

	public void setStereoParameters(boolean stereoParameters) {
		this.stereoParameters = stereoParameters;
	}

		
}
