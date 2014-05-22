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


package de.jreality.portal;

import java.awt.Component;
import java.awt.Dimension;
import java.beans.Statement;
import java.util.List;

import javax.swing.JFrame;

import de.jreality.math.FactoredMatrix;
import de.jreality.math.Matrix;
import de.jreality.math.MatrixBuilder;
import de.jreality.math.Pn;
import de.jreality.math.Rn;
import de.jreality.scene.Camera;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphNode;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.Transformation;
import de.jreality.scene.Viewer;
import de.jreality.scene.proxy.scene.RemoteSceneGraphComponent;
import de.jreality.util.CameraUtility;
import de.jreality.util.ConfigurationAttributes;
import de.jreality.util.LoggingSystem;
import de.jreality.util.Secure;
import de.jreality.util.SystemProperties;
import de.smrj.ClientFactory;

/**
 * 
 * A Viewer for rendering on remote clients of a distributed environment.
 * It gets set the local copy of the master scene graph, adapts its camera path,
 * and sets the correct view port before each render call.
 * 
 * It is assumed that the camera path has the following structure:
 * root -\> ... ... -\> VE -\> headComponent -\> camera
 * 
 * Conceptually, the VE component represents the coordinate system of the virtual
 * environment (i.e. CAVE, Portal). This node is navigated in the scene, so we
 * imagine that the space inside the screens of the VE is navigated in the 3D scene.
 * Thus, the path to this node should be the avatarPath.
 * The heasComponent has the euclidean transformation of the head inside the
 * VE and this transformation is written from directly from the tracking system, on
 * the master scene graph.
 * 
 * This class replaces the head component of its local scene graph copy by the two
 * components: cameraTranslationNode and cameraOrientationNode.
 * 
 * The cameraOrientationNode has the rotation from the VE coordinate
 * system to the screen, and this transformation is fixed.
 * 
 * The cameraTranslationNode has the translation of the head inside the VE, which is set
 * in each render call (setHeadMatrix, called from render). There, also the correct
 * viewport and orientationMatrix of the camera is computed and set.
 * 
 * 
 * @author Steffen Weissmann
 *
 */
public class HeadTrackedViewer implements Viewer, RemoteViewer, ClientFactory.ResetCallback {

	Viewer viewer;
	private SceneGraphComponent cameraTranslationNode;
	private SceneGraphComponent cameraOrientationNode;
	double[] tmpHead = new double[16];
	private boolean hasSceneRoot;
	private boolean hasCamPath;
	private SceneGraphComponent headComponent;
	SceneGraphPath portalPath;
	SceneGraphPath cameraPath;

	Camera cam;

	public static HeadTrackedViewer create(Class<? extends Viewer> viewerClass) {
		ConfigurationAttributes config = ConfigurationAttributes.getDefaultConfiguration();
		return create(config, viewerClass);
	}

	private static JFrame frame;
	private static HeadTrackedViewer hv;

	public static HeadTrackedViewer create(ConfigurationAttributes config, Class<? extends Viewer> viewerClass) {
		if (frame == null) {
			hv = new HeadTrackedViewer(config, viewerClass);
			Component viewingComponent = (Component) hv.getViewingComponent();
			frame = PortalUtility.displayPortalViewingComponent(viewingComponent);
		}
		return hv;
	}

	public HeadTrackedViewer(ConfigurationAttributes config, Class<? extends Viewer> viewerClass) {
		try {
			viewer = viewerClass.newInstance();
		} catch (Exception e) {
			e.printStackTrace();
			throw new Error("Viewer creation failed!");
		}
		init(config);
	}

	private void init(ConfigurationAttributes config) {
		cameraTranslationNode = new SceneGraphComponent();
		cameraTranslationNode.setTransformation(new Transformation());

		cameraTranslationNode.setName("cam Translation");

		cameraOrientationNode = new SceneGraphComponent();
		cameraOrientationNode.setName("cam Orientation");
		// set camera orientation to value from config file...
		double[] rot = config.getDoubleArray("camera.orientation");
		MatrixBuilder mb = MatrixBuilder.euclidean();
		double screenRotation = 0;
		if (rot != null) {
			screenRotation = rot[0] * ((Math.PI * 2.0) / 360.);
			mb.rotate(screenRotation, new double[] { rot[1], rot[2], rot[3] });
		}
		mb.assignTo(cameraOrientationNode);
		cameraTranslationNode.addChild(cameraOrientationNode);
	}

	public SceneGraphComponent getAuxiliaryRoot() {
		return viewer.getAuxiliaryRoot();
	}

	public SceneGraphPath getCameraPath() {
		return cameraPath;
	}

	public SceneGraphComponent getSceneRoot() {
		return viewer.getSceneRoot();
	}

	public Object getViewingComponent() {
		return viewer.getViewingComponent();
	}

	public boolean hasViewingComponent() {
		return viewer.hasViewingComponent();
	}

	public void render() {
		if (!hasSceneRoot || !hasCamPath) return;
		setHeadMatrix(headComponent.getTransformation().getMatrix(tmpHead));
		viewer.render();
	}

	public void setAuxiliaryRoot(SceneGraphComponent ar) {
		viewer.setAuxiliaryRoot(ar);
	}

	public void setCameraPath(SceneGraphPath camPath) {
		cameraPath = new SceneGraphPath(camPath);
		hasCamPath = !(camPath == null || camPath.getLength() == 0);
		// empty path => reset fields
		if (camPath == null || camPath.getLength() == 0) {
			viewer.setCameraPath(null);
			headComponent = null;
			portalPath = null;
			cam = null;
			return;
		}
		// new camera path => extract headComponent and set artificial camera path
		cam = (Camera) camPath.getLastElement();
		// TODO: do these settings on client side...
		//cam.setStereo(config.getBool("camera.stereo"));
		//cam.setEyeSeparation(config.getDouble("camera.eyeSeparation"));
		cam.setOnAxis(false);

		camPath.pop();

		headComponent = camPath.getLastComponent();

		camPath.pop(); // now this should be the portal path
		portalPath = new SceneGraphPath(camPath);
		System.err.println("portal path matrix = "+Rn.matrixToString(portalPath.getMatrix(null)));

		// add camera position and orientation, add camera there
		// DONT CHANGE SCENEGRAPH
//		camPath.getLastComponent().addChild(cameraTranslationNode);
		cameraOrientationNode.setCamera(cam);
		if (cam.isOnAxis()) {
			LoggingSystem.getLogger(CameraUtility.class).info("portal camera is on-axis: changing to off-axis");
			cam.setOnAxis(false);
		}
		
		//added on 18.05.2012 andre
		boolean quadBufferedStereo = "true".equals(Secure.getProperty(SystemProperties.JOGL_QUAD_BUFFERED_STEREO));
		boolean isLeftEye = "true".equals(Secure.getProperty(SystemProperties.JOGL_LEFT_STEREO));
		boolean isRightEye = "true".equals(Secure.getProperty(SystemProperties.JOGL_RIGHT_STEREO));
		//andre 17.05.2012
		boolean isMasterStereo = "true".equals(Secure.getProperty(SystemProperties.JOGL_MASTER_STEREO));
		
		if(quadBufferedStereo){//andre 19.04.2012
			cam.setStereo(true);}//andre 19.04.2012
		else if (isLeftEye || isRightEye){
			cam.setStereo(false);//andre 19.04.2012
			cam.setLeftEye(isLeftEye);//andre 19.04.2012
			cam.setRightEye(isRightEye);//andre 19.04.2012
		}
		else if (isMasterStereo){//andre 17.05.2012
			cam.setStereo(false);//andre 17.05.2012
		}
		else {//andre 19.04.2012
			cam.setStereo(true);//andre 19.04.2012
		}//andre 18.05.2012
		
		// build the right camera path
		camPath.push(cameraTranslationNode);
		camPath.push(cameraOrientationNode);
		camPath.push(cam);

		// set camera path to viewer
		viewer.setCameraPath(camPath);

		// hack
		setHeadMatrix(Rn.identityMatrix(4));
	}

	public void setSceneRoot(SceneGraphComponent r) {
		hasSceneRoot = !(r == null);
		viewer.setSceneRoot(r);
	}

//	public void setMetric(int sig) {
//	viewer.setMetric(sig);
//	}

	double[] tmp1 = new double[16];
	double[] tmp2 = new double[16];
	FactoredMatrix headMatrix = new FactoredMatrix();
	FactoredMatrix headTranslation = new FactoredMatrix();
	FactoredMatrix portalMatrix = new FactoredMatrix();
	Matrix totalOrientation = new Matrix();
	Matrix world2cam = new Matrix();

	private void setHeadMatrix(double[] head) {

		// this sets the translation in the camera path; and sets the
		// orientation matrix for the camera (for detection of eye positions)
		// and the cameras viewport.

		headMatrix.assignFrom(head);
		headTranslation.setTranslation(headMatrix.getTranslation());

		headTranslation.assignTo(cameraTranslationNode);

		totalOrientation.assignFrom(cameraOrientationNode.getTransformation());
		totalOrientation.invert();
		totalOrientation.multiplyOnRight(headMatrix);
		cam.setOrientationMatrix(totalOrientation.getArray());

		portalMatrix.assignFrom(portalPath.getMatrix(tmp1));
		world2cam.assignFrom(viewer.getCameraPath().getInverseMatrix(tmp2));

		double[] portalOriginInCamCoordinates = world2cam.multiplyVector(portalMatrix.getTranslation());
		Pn.dehomogenize(portalOriginInCamCoordinates, portalOriginInCamCoordinates);

		PortalCoordinateSystem.setPORTALViewport(portalOriginInCamCoordinates, cam);

	}

	Statement waitStatement;
	public void waitForRenderFinish() {
		if (waitStatement == null) waitStatement = new Statement(viewer, "waitForRenderFinish", null);
		try {
			waitStatement.execute();
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	public void setRemoteSceneRoot(RemoteSceneGraphComponent r) {
		setSceneRoot((SceneGraphComponent) r);
	}

	public void setRemoteAuxiliaryRoot(RemoteSceneGraphComponent r) {
		setAuxiliaryRoot((SceneGraphComponent) r);
	}

	public void setRemoteCameraPath(List<SceneGraphNode> list) {
		setCameraPath(SceneGraphPath.fromList(list));
	}

	public void resetCalled() {
		frame.setVisible(false);
		frame.dispose();
		frame = null;
	}

	public Dimension getViewingComponentSize() {
		return viewer.getViewingComponentSize();
	}

	public boolean canRenderAsync() {
		return viewer.canRenderAsync();
	}

	public void renderAsync() {
		viewer.renderAsync();
	}

}
