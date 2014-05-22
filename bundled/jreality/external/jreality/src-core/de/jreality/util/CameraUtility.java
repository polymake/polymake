/**
 *
 * This file is part of jReality. jReality is open source software, made
 * available under a BSD license:
 *
 * Copyright (c) 2003-2006, jReality Group: Charles Gunn, Tim Hoffmann, Markus
 * Schmies, Steffen Weissmann.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * - Neither the name of jReality nor the names of its contributors nor the
 *   names of their associated organizations may be used to endorse or promote
 *   products derived from this software without specific prior written
 *   permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */


package de.jreality.util;

import java.awt.Dimension;
import java.awt.geom.Rectangle2D;
import java.security.PrivilegedAction;
import java.util.logging.Level;
import java.util.prefs.Preferences;

import de.jreality.geometry.BoundingBoxUtility;
import de.jreality.math.Matrix;
import de.jreality.math.MatrixBuilder;
import de.jreality.math.P3;
import de.jreality.math.Pn;
import de.jreality.math.Quaternion;
import de.jreality.math.Rn;
import de.jreality.scene.Camera;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.Viewer;
import de.jreality.shader.CommonAttributes;

/**
 * A collection of static methods related to the jReality Camera class.
 * <p>
 * Most of the methods involve calculating various transformations related to the
 * camera and an instance of Viewer.
 * 
 * @author Charles Gunn
 *
 */
public class CameraUtility {

	 // this is code related to PORTAL walls; but in order to avoid dependency cycles,
	  // between PORTAL and JOGL, put it here
	  public static  Matrix cameraOrientation = new Matrix();
	  public static  Matrix inverseCameraOrientation = new Matrix();
	  static {
		  try {
			  String foo = Secure.getProperty(SystemProperties.ENVIRONMENT);
			  if (foo != null && foo.indexOf("portal") != -1) {
				  ConfigurationAttributes config = ConfigurationAttributes.getDefaultConfiguration();
				  double[] rot = config.getDoubleArray("camera.orientation");
				  MatrixBuilder mb = MatrixBuilder.euclidean();
				  double camRot = 0;
				  if (rot != null) {
					  camRot = rot[0] * ((Math.PI * 2.0) / 360.);
					  mb.rotate(camRot, new double[] { rot[1], rot[2], rot[3] });
				  }
				  cameraOrientation=mb.getMatrix();
				  inverseCameraOrientation = cameraOrientation.getInverse();	
			  }
		  } catch(SecurityException se)	{
			  LoggingSystem.getLogger(CameraUtility.class).log(Level.WARNING,"Security exception in getting configuration options",se);
		  }
	  }
	static boolean debug = false;
	// constants for support of stereo viewing
	public static final int MIDDLE_EYE = 0;
	public static final int LEFT_EYE = 1;
	public static final int RIGHT_EYE = 2;

	private CameraUtility() {
		super();
	}

	/**
	 * Determine the camera for this viewer.
	 * @param v
	 * @return
	 */
	public static Camera getCamera(Viewer v)	{
		if (v == null || v.getCameraPath() == null || !(v.getCameraPath().getLastElement() instanceof Camera)) 
			throw new IllegalStateException("Viewer has no camera!");
		return ((Camera) v.getCameraPath().getLastElement());
	}

	/**
	 * Determine the SceneGraphComponent which contains the camera for this viewer.
	 * @param v
	 * @return
	 */public static SceneGraphComponent getCameraNode(Viewer v)	{
		return  v.getCameraPath().getLastComponent();
	}

	static double nearFarFactor = 0.9;
	/**
	 * @param camera
	 * @param viewer
	 */
	
	public static void encompassNew(Viewer v) {
		SceneGraphPath avatarPath = v.getCameraPath().popNew();
		if (avatarPath.getLength() > 1) avatarPath.pop();
		SceneGraphPath toBound = new SceneGraphPath(v.getSceneRoot());
		CameraUtility.encompass(avatarPath, toBound, v.getCameraPath(), 1.1, SceneGraphUtility.getMetric(v.getCameraPath()));
	}


	public static void encompass( de.jreality.scene.Viewer viewer) {
		// remove camera from the sceneRoot and encompass the result
		SceneGraphPath cp = viewer.getCameraPath();
		if (cp == null) throw new IllegalStateException("camerapath == null");
		if (cp.getLength() < 3)	{
		  throw new IllegalStateException("can't encompass: possibly Camera attached to root");
		}
		boolean visible = false;
		SceneGraphComponent cameraBranch = (SceneGraphComponent) cp.iterator(1).next();
		SceneGraphComponent root = viewer.getSceneRoot();
		Object foo = root.getAppearance().getAttribute(CommonAttributes.METRIC);
		int metric = Pn.EUCLIDEAN;
		if (foo instanceof Integer) {
			metric = ((Integer) foo).intValue();
		}
		try {
			// TODO this is always true if camerapath starts at root
			if(root.isDirectAncestor(cameraBranch)) {
				visible = cameraBranch.isVisible();
				cameraBranch.setVisible(false);
			} 
  		encompass(viewer, root, true, metric);
		} finally {
			cameraBranch.setVisible(visible);
		}
	}
	
	/**
	 * Encompass the world displayed by a viewer and possibly set derived parameters 
	 * in the camera.
	 * @param viewer
	 * @param sgc
	 * @param setStereoParameters
	 */
	@Deprecated
	public static void encompass(Viewer viewer, SceneGraphComponent sgc, boolean setStereoParameters)	{
		encompass(viewer, sgc, setStereoParameters, Pn.EUCLIDEAN);
	}
	public static void encompass(Viewer viewer, SceneGraphComponent sgc, boolean setStereoParameters, int metric)	{
		Rectangle3D worldBox = BoundingBoxUtility.calculateBoundingBox(sgc);//. bbv.getBoundingBox();
		if (worldBox == null || worldBox.isEmpty())	{
			LoggingSystem.getLogger(CameraUtility.class).log(Level.WARNING,"encompass: empty bounding box");
			return;	
		}
		
		SceneGraphPath w2a = viewer.getCameraPath().popNew();
		w2a.pop();
		double[] w2ava = w2a.getInverseMatrix(null);
		worldBox.transformByMatrix(worldBox, w2ava);
		
		if (worldBox == null || worldBox.isEmpty())	{
			LoggingSystem.getLogger(CameraUtility.class).log(Level.WARNING,"encompass: empty bounding box");
			return;	
		}
		
//		if (debug) 
			LoggingSystem.getLogger(CameraUtility.class).log(Level.FINER,"BBox: "+worldBox.toString());
		
		Camera cam = getCamera(viewer);
		// the extent in camera coordinates
		double[] extent = worldBox.getExtent();
		
		double ww = (extent[1] > extent[0]) ? extent[1] : extent[0];
		double focus =  .5 * ww / Math.tan(Math.PI*(cam.getFieldOfView())/360.0);
    
		double[] to = worldBox.getCenter();
		to[2] += extent[2]*.5;
		double[] tofrom = {0,0,focus}; 
		double[] from = Rn.add(null, to, tofrom);
		if (debug) LoggingSystem.getLogger(CameraUtility.class).log(Level.FINER,"translate: "+Rn.toString(from));
		double[] newCamToWorld = P3.makeTranslationMatrix(null, from, metric);
		double[] newWorldToCam = Rn.inverse(null, newCamToWorld);
		getCameraNode(viewer).getTransformation().setMatrix(newCamToWorld); //Translation(from);			
		double[] centerWorld = Rn.matrixTimesVector(null, newWorldToCam, worldBox.getCenter() );
		if (setStereoParameters)	{
				cam.setFocus(Math.abs(centerWorld[2]) ); 		//focus);
				cam.setEyeSeparation(cam.getFocus()/12.0);		// estimate a reasonable separation based on the focal length	
		}
		//TODO figure out why setting the near/far clipping planes sometimes doesn't work
		Rectangle3D cameraBox = worldBox.transformByMatrix(null, newWorldToCam);
		if (debug) LoggingSystem.getLogger(CameraUtility.class).log(Level.FINER,"Bbox: "+cameraBox.toString());
		double zmin = cameraBox.getMinZ();
		double zmax = cameraBox.getMaxZ();
		
		if ( cam.getFar() > 0.0 && zmax < 0.0 && -zmax > .1*cam.getFar() )  cam.setFar(-10*zmax);
		if ( zmin < 0.0 && -zmin < 10*cam.getNear() )  cam.setNear(-.1*zmin);
		//cam.update();
		
	}

	/**
	 * Determine the aspect ratio of the output window of a viewer.
	 * @param v
	 * @return
	 */
	public static double getAspectRatio(Viewer v)		{
		if (!v.hasViewingComponent()) return 1.0;
		Dimension d = v.getViewingComponentSize();
		return d.getHeight() > 0 ? (double) d.getWidth()/ d.getHeight() : 1.0;
	}
	
	/**
	 * Calculate the camera to NDC (normalized device coordinates) transformation
	 * for a given viewer. 
	 * @param v
	 * @return
	 */
	public static double[] getCameraToNDC(Viewer v)			{
		Camera cam = getCamera(v);
		double aspectRatio = getAspectRatio(v);
		return getCameraToNDC(cam, aspectRatio);
	}
	
	/**
	 * 
	 * @param cam
	 * @param aspectRatio
	 * @return
	 */
	public static double[] getCameraToNDC(Camera cam, double aspectRatio)		{
		return getCameraToNDC(cam, aspectRatio, CameraUtility.MIDDLE_EYE);
	}
	
	/**
	 * Calculate a 4x4 projection matrix for this camera.  If <i>which</i> is <code>MIDDLE_EYE</code>, calculate a
	 * normal "monocular" camera.  If <i>which</i> is <code>LEFT_EYE</code> or <code>RIGHT_EYE,</code>, calculate the
	 * projection matrix corresponding to the given eye of a stere-ocular camera.  The stereo case can be derived 
	 * from the monocular case as follows.  
	 * <p>
	 * Define V to be the intersection of the viewing frustum with the plane <i>z = focus</i> (See {@link #setFocus(double)}).
	 * Second, define the positions <i>Pl = (d,0,0,0)</i> and <i>Pr = (-d,0,0,0)</i> where <i>d = eyeSeparation/2.0</i>  (See
	 * {@link #setEyeSeparationMeters(double)}). Then the position of the left eye in
	 * camera coordinates is O.Pl (where O is the camera's orientation matrix (See {@link #setOrientationMatrix(double[])}), or the identity
	 * matrix if none has been set) and similarly for the right eye. Then the viewing frustum for the left eye is the unique viewing frustum determined by 
	 * the position at the left (right) eye and the rectangle V; similarly for the right eye.
	 * <p>
	 * In plain English, the monocular, left, and right views all show the same picture if the world lies in the <i>z = focus</i> plane.
	 * This plane is in fact the focal plane in this sense.
	 * <p>
	 * Note that the <i>orientationMatrix</i> is only non-trivial in the case of virtual environments such as the PORTAL or CAVE. 	 * @deprecated
	 * @param which
	 * @return
	 */
	public static double[] getCameraToNDC(Camera cam, double aspectRatio, int which)		{
		return getCameraToNDC(cam, aspectRatio, which, Pn.EUCLIDEAN);
	}
	
	public static double[] getCameraToNDC(Camera cam, double aspectRatio, int which, int metric)		{
			/** 
			* If the projectoin is orthogonal, scales the viewPort by the \IT{focus}.
			* This method won't be called if the value of \IT{isOnAxis} is FALSE;
			* instead the view port should be directly set using \mlink{setViewPort:}.
			*/
		// comment out the log statement: it actually shows up in the profiler!
//		LoggingSystem.getLogger(CameraUtility.class).log(Level.FINER,"Aspect ratio is "+aspectRatio);
//		System.out.println("Aspect ratio is "+aspectRatio);
		Rectangle2D viewPort = getViewport(cam, aspectRatio);
		if (which == CameraUtility.MIDDLE_EYE)		{
			double[] cameraToNDC = null;
			if (cam.isPerspective())
				cameraToNDC = P3.makePerspectiveProjectionMatrix(null, viewPort, cam.getNear(), cam.getFar());
			else	
				cameraToNDC = P3.makeOrthographicProjectionMatrix(null, viewPort,cam.getNear(), cam.getFar());
			return cameraToNDC;			
		}  // else we're in a stereo mode
		double[] eyePosition = getEyePosition(cam, which);
		// TODO make this work also for non-euclidean cameras
		double[] moveToEye = P3.makeTranslationMatrix(null, eyePosition, metric );
		Rectangle2D newVP = getOffAxisViewPort(cam, viewPort, eyePosition);
		// TODO should we adjust near and far ?
		double[] c2ndc = P3.makePerspectiveProjectionMatrix(null, newVP, cam.getNear(), cam.getFar());		
		double[] iMoveToEye = Rn.inverse(null, moveToEye);
		//LoggingSystem.getLogger().log(Level.FINER,"iMoveToEye is \n"+Rn.matrixToString(iMoveToEye));
		double[] ret = Rn.times(null, c2ndc, iMoveToEye);
		return ret;
	}

	/**
	 * A method required for calculating cam2NDC for a CAVE-like environment.
	 * @param cam
	 * @param which
	 * @return
	 */public static double[] getEyePosition(Camera cam, int which) {
		double factor;
		if (which == MIDDLE_EYE) return ( new double[]{0,0,0,1});
		factor = (which == CameraUtility.LEFT_EYE) ? -1 : 1;
		//if (eyeSeparation == 0.0) eyeSeparation = focus/6.0;
		
		double[] eyePosition = new double[] {factor * cam.getEyeSeparation()/2.0, 0d, 0d, 0d};
		if (cam.getOrientationMatrix() != null)	{
			Rn.matrixTimesVector(eyePosition, cam.getOrientationMatrix(), eyePosition);
			Pn.dehomogenize(eyePosition, eyePosition);
			//LoggingSystem.getLogger().log(Level.FINER,((which == RIGHT_EYE)?"right":"left")+" eye position: "+Rn.toString(eyePosition));
		}
		if (eyePosition[3] == 0.0) eyePosition[3] = 1.0;
		return eyePosition;
	}

	/**
	 * A method required for calculating cam2NDC transformation for an off-axis camera.
	 * NOTE: a stereo camera is an off-axis camera.
	 * @param cam
	 * @param viewPort
	 * @param eyePosition
	 * @return
	 */public static Rectangle2D getOffAxisViewPort(Camera cam, Rectangle2D viewPort, double[] eyePosition) {
		double x = eyePosition[0];
		double y = eyePosition[1];
		double z = eyePosition[2];
		Rectangle2D newVP = new Rectangle2D.Double();
		double focus = cam.getFocus();
		double newFocus = focus + z;
		double fscale = 1.0/newFocus;
		// We want the slice of the non-stereo frustum at z = focus to be also a slice 
		// of the new, stereo frustum.  Make that happen:
		// Scale the camera viewport to lie in the z=focus plane, 
		// translate it into the coordinates of the eye position (left or right),
		// then project it onto the z=1 plane in this coordinate system.
		newVP.setFrameFromDiagonal(fscale*(viewPort.getMinX()*focus-x), 
								 fscale*(viewPort.getMinY()*focus-y), 
								 fscale*(viewPort.getMaxX()*focus-x), 
								 fscale*(viewPort.getMaxY()*focus-y));
		return newVP;
	}

	/**
	 * Determine the viewport of the given camera: the intersection of the viewing frustum
	 * with the z=1 plane.
	 * @param cam
	 * @param aspectRatio
	 * @return
	 */
	public static Rectangle2D getViewport(Camera cam, double aspectRatio) {
		Rectangle2D viewPort = null;
		if (cam.isOnAxis() || cam.getViewPort() == null)	{
			viewPort = new Rectangle2D.Double();
			double hwidth = Math.tan((Math.PI/180.0)*cam.getFieldOfView()/2.0);
			if (!cam.isPerspective())	hwidth *= cam.getFocus();
			if (aspectRatio > 1.0)	{
				viewPort.setFrameFromDiagonal(-hwidth*aspectRatio, -hwidth,hwidth*aspectRatio,  hwidth);
			} else {
				viewPort.setFrameFromDiagonal(-hwidth,-hwidth/aspectRatio, hwidth,  hwidth/aspectRatio);
			}	
		} else viewPort = cam.getViewPort();
		return viewPort;
	}
	
	public static double[][] getNearViewport(Viewer v)	{
		Rectangle2D vp = null;
		Camera camera = getCamera(v);
		Rectangle2D vp1 = getViewport(camera, getAspectRatio(v));
		double near = camera.getNear();
		double[][] nearCorners = new double[4][];
		return nearCorners;
	}

	public static double[] getNDCToCamera(Viewer v)	{
		return Rn.inverse(null, getCameraToNDC(v));
	}
	
	public static double[] getNDCToCamera(Camera cam, double aspectRatio)	{
		return Rn.inverse(null, getCameraToNDC(cam, aspectRatio));
	}
	
    
	public static void encompass(SceneGraphPath avatarPath, SceneGraphPath scene, SceneGraphPath cameraPath) {
		encompass(avatarPath, scene, cameraPath, 0, Pn.EUCLIDEAN);
	}
  
	/**
	 * A method for encompassing the scene.
	 * @param avatarPath
	 * @param scene
	 * @param cameraPath
	 * @param margin
	 * @param metric
	 * @deprecated See  {@link EncompassFactory}, which additionally allows control of setting camera parameters.
	 */
	public static void encompass(SceneGraphPath avatarPath, SceneGraphPath scene, SceneGraphPath cameraPath, double margin, int metric) {
		EncompassFactory ec = new EncompassFactory();
		ec.setAvatarPath(avatarPath);
		ec.setScenePath(scene);
		ec.setCameraPath(cameraPath);
		ec.setMargin(margin);
		ec.setMetric(metric);
		ec.update();
	}
	/**
	 * A method for encompassing the scene.
	 * @param avatarPath
	 * @param scene
	 * @param cameraPath
	 * @param margin
	 * @param metric
	 * @param noTerrain
	 * @deprecated See  {@link EncompassFactory}, which additionally allows control of setting camera parameters.
	 */
	// I have removed this version of the encompass method, and replaced it 
	// (in JRViewerUtility, where it was used) with the EncompassFactory 
//	public static void encompass(SceneGraphPath avatarPath, SceneGraphPath scene, SceneGraphPath cameraPath, double margin, int metric, boolean noTerrain) {
//		EncompassFactory ec = new EncompassFactory();
//		ec.setAvatarPath(avatarPath);
//		ec.setScenePath(scene);
//		ec.setCameraPath(cameraPath);
//		ec.setMargin(margin);
//		ec.setMetric(metric);
//		ec.update();
//		if(!noTerrain){
//			Camera camera = ((Camera)cameraPath.getLastElement());
//			camera.setFar(1000);
//			camera.setNear(.1);
//		}
//	}
//		Rectangle3D bounds = BoundingBoxUtility.calculateBoundingBox(scene.getLastComponent());
//	    if (bounds.isEmpty()) return;
//	    Matrix rootToScene = new Matrix();
//	    scene.getMatrix(rootToScene.getArray(), 0, scene.getLength()-2);
//	    Rectangle3D worldBounds = bounds.transformByMatrix(new Rectangle3D(), rootToScene.getArray());
//	    Rectangle3D avatarBounds = worldBounds.transformByMatrix(new Rectangle3D(), avatarPath.getInverseMatrix(null));
//	    double [] e = avatarBounds.getExtent();
//	    double radius = Rn.euclideanNorm(e);
//	    double [] c = avatarBounds.getCenter();
//	    // TODO: read viewing angle from camera
//	    c[2] += margin*radius;
//	    //Rn.times(c, margin, c);
//	    // add head height to c[1]
//	    Matrix camMatrix = new Matrix();
//	    cameraPath.getInverseMatrix(camMatrix.getArray(), avatarPath.getLength());
//	    
//	    Camera camera = ((Camera)cameraPath.getLastElement());
//		camera.setFar(margin*3*radius);
//	    camera.setNear(.3*radius);
//	    SceneGraphComponent avatar = avatarPath.getLastComponent();
//	    Matrix m = new Matrix(avatar.getTransformation());
//	    if (SystemProperties.isPortal) return;
//	    if (camera.isPerspective()) {
//		    MatrixBuilder.init(m, metric).translate(c).translate(camMatrix.getColumn(3)).assignTo(avatar);	    	
//			camera.setFocus(Math.abs(m.getColumn(3)[2]) ); 		//focus);
//	    } else {
//			double ww = (e[1] > e[0]) ? e[1] : e[0];
//			double focus =   ww / Math.tan(Math.PI*(camera.getFieldOfView())/360.0);
//			camera.setFocus(Math.abs(focus) ); 		//focus);	    	
//	    }
//		camera.setEyeSeparation(camera.getFocus()/12.0);		// estimate a reasonable separation based on the focal length	
////		System.err.println("setting focus to "+camera.getFocus());
//	}

	
	private static final String FOV = "camera:field_of_view";
	private static final String FOCUS = "camera:focus";
	private static final String FOCLEN = "camera:focal_length";
	private static final String EYESEP = "camera:eye_separation";
	private static final String PERSP = "camera:perspective";
	private static final String ONAXIS = "camera:on_axis";
	private static final String STEREO = "camera:stereo";
	
	public static void loadPreferences(Camera cam) {
		Preferences prefs = getCameraPreferences();
		if (prefs == null) {
			LoggingSystem.getLogger(CameraUtility.class).log(Level.INFO, "could not get camera preferences");
			return;
		}
		cam.setFieldOfView(prefs.getDouble(FOV, cam.getFieldOfView()));
		cam.setFocus(prefs.getDouble(FOCUS, cam.getFocus()));
		cam.setFocalLength(prefs.getDouble(FOCLEN, cam.getFocalLength()));
		cam.setEyeSeparation(prefs.getDouble(EYESEP, cam.getEyeSeparation()));
		cam.setPerspective(prefs.getBoolean(PERSP, cam.isPerspective()));
		cam.setOnAxis(prefs.getBoolean(ONAXIS, cam.isOnAxis()));
		cam.setStereo(prefs.getBoolean(STEREO, cam.isStereo()));
	}

	public static void savePreferences(Camera cam) {
		Preferences prefs = getCameraPreferences();
		if (prefs == null) {
			LoggingSystem.getLogger(CameraUtility.class).log(Level.INFO, "could not get camera preferences");
			return;
		}
		prefs.putDouble(FOV, cam.getFieldOfView());
		prefs.putDouble(FOCUS, cam.getFocus());
		prefs.putDouble(FOCLEN, cam.getFocalLength());
		prefs.putDouble(EYESEP, cam.getEyeSeparation());
		prefs.putBoolean(PERSP, cam.isPerspective());
		prefs.putBoolean(ONAXIS, cam.isOnAxis());
		prefs.putBoolean(STEREO, cam.isStereo());
	}

	private static Preferences getCameraPreferences() {
		return Secure.doPrivileged(new PrivilegedAction<Preferences>() {
			public Preferences run() {
				return Preferences.userNodeForPackage(Camera.class);
			}
		});
	}

	// columns of m4 are the elements of the standard xyz axes in homogeneous coordinates 
	private static double[] standardFrame = Rn.transpose(null, new double[]{
		1,0,0,1,
		0,1,0,1,
		0,0,1,1,
		0,0,0,1});
	/**
	 * @param o2ndc	object to normalized device coordinate transformation
	 * @return
	 */
	public static double getNDCExtent(double[] o2ndc) {
		double[][] images = new double[4][4];
		// we can't just use the columns of o2ndc since there's no easy way to
		// dehomogenize them (they are generally points at infinity)
		Matrix imageFrame = new Matrix(Rn.times(null, o2ndc, standardFrame));
		for (int i = 0; i<4; ++i)	{
			images[i] = imageFrame.getColumn(i);
			images[i] = Pn.dehomogenize(null, images[i]);
		}
		double d = 0.0;
		// find the maximum xy extent in ndc coordinates of the three unit vectors
		// in object space
		for (int i = 0; i<3; ++i)	 {
			// now we subtract off the "origin" to get a vector
			double[] tmp = Rn.subtract(null, images[3], images[i]);
			double t = Math.sqrt(Rn.innerProduct(tmp,tmp,2));
			if (t > d) d = t;
		}
		return d;
	}
	
	public static double getScalingFactor(double[] o2w, int metric)	{
		double factor = 0.0;
		Quaternion q1 = new Quaternion(), q2 = new Quaternion();
		double[] tv = new double[4], sv = new double[4];
		boolean[] flipped = new boolean[1];
		P3.factorMatrix(o2w,tv, q1, q2, sv, flipped, metric);
		factor = (sv[0]+sv[1]+sv[2])/3.0;
		return factor;
	}
}
