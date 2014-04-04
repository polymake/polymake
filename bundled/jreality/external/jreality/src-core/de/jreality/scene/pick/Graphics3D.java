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


package de.jreality.scene.pick;

import java.awt.Component;

import de.jreality.math.Rn;
import de.jreality.scene.Camera;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.Viewer;
import de.jreality.util.CameraUtility;

/**
 * @author gunn
 *
 */
public class Graphics3D {
	//Viewer viewer;
	double aspectRatio = 1.0;
	SceneGraphComponent theRoot;
	SceneGraphPath cameraPath;
	Camera camera;
	double[] objectToWorld;
	boolean fastAndDirty = false;
	SceneGraphPath currentPath;

	/**
	 * @param v
	 */
	public Graphics3D(Viewer v)	{
		this(v, null);
	}
	
	/**
	 * @param v
	 * @param sgp
	 */
	public Graphics3D(Viewer v, SceneGraphPath sgp)	{
	 	this( v.getCameraPath(), sgp, CameraUtility.getAspectRatio(v));
	 }
	 
//	 public Graphics3D(SceneGraphPath cp, SceneGraphPath sgp)	{
//		 this(cp, sgp, 1.0);
//	}
	 public Graphics3D(SceneGraphPath cp, SceneGraphPath sgp, double ar)	{
			super();
			//objectToWorld = Rn.identityMatrix(4);
			if (sgp != null && sgp.getLength() > 0) setRoot((SceneGraphComponent) sgp.getFirstElement());
			setCameraPath(cp);
			setCurrentPath(sgp);
			setAspectRatio(ar);
	 }
	public void setAspectRatio(double ar) {
		aspectRatio = ar;
	}

	/**
	 * @param cameraPath2
	 */
	private void setCameraPath(SceneGraphPath cameraPath2) {
		cameraPath = cameraPath2;
		if (cameraPath == null) return;
		Object obj = cameraPath.getLastElement();
		if ( obj != null && obj instanceof Camera)	camera = (Camera) cameraPath.getLastElement();
		else throw new IllegalArgumentException("Not a camera path");
	}

	/**
	 * @param sceneRoot
	 */
	private void setRoot(SceneGraphComponent sceneRoot) {
		if (sceneRoot == null) throw new IllegalArgumentException("Root can't be null");
		theRoot = sceneRoot;
	}

	public SceneGraphPath getCameraPath() {
		return cameraPath;
	}
	public SceneGraphComponent getRoot() {
		return theRoot;
	}
	/**
	 * @return
	 */
	public double[] getCameraToNDC() {
		if (cameraPath == null) throw new IllegalStateException("No camera path set for this context");
		return CameraUtility.getCameraToNDC(camera, aspectRatio);
	}

	/**
	 * If the path is set, then it overrides the object2World matrix value
	 * @return
	 */
	public double[] getObjectToWorld() {
		if (objectToWorld != null)	return objectToWorld;
		if (currentPath != null) return currentPath.getMatrix(null);
		return Rn.identityMatrix(4);
	}

	/**
	 * @return
	 */
	public double[] getWorldToObject() {
		return Rn.inverse(null,getObjectToWorld());
	}

	/**
	 * @return
	 */
	public double[] getWorldToCamera() {
		if (cameraPath == null) 
			throw new IllegalStateException("No camera path");
		return cameraPath.getInverseMatrix(null);
	}

	/**
	 * @return
	 */
	public double[] getCameraToWorld() {
		if (cameraPath == null) 			
			throw new IllegalStateException("No camera path");
		return cameraPath.getMatrix(null);
	}
	/**
	 * @return
	 */
	public double[] getWorldToNDC() {
		return Rn.times(null, getCameraToNDC(), getWorldToCamera());
	}

	/**
	 * @param ds
	 */
	public void setObjectToWorld(double[] ds) {
		objectToWorld = (double[]) ds.clone();
	}


	 /* @param ds
	 */
	public double[] getObjectToCamera() {
		return Rn.times(null, getWorldToCamera(), getObjectToWorld());
	}

	/**
	 * @return
	 */
	public double[] getCameraToObject() {
		return Rn.inverse(null, getObjectToCamera());
	}

	/**
	 * @param ds
	 */
	public double[] getObjectToNDC() {
		if (camera == null) 
			throw new IllegalStateException("No camera for this context");
		return Rn.times(null,CameraUtility.getCameraToNDC(camera, aspectRatio), getObjectToCamera());
	}

	/**
	 * @param ds
	 */
	public double[] getNDCToObject() {
		if (camera == null) 
			throw new IllegalStateException("No camera for this context");
		return Rn.inverse(null, Rn.times(null, CameraUtility.getCameraToNDC(camera, aspectRatio), getObjectToCamera()));
	}

	/**
	 * @param ds
	 */
	public double[] getObjectToScreen(Component dr) {
		return Rn.times(null, getNDCToScreen(dr), getObjectToNDC());
	}


	/**
	 * @return
	 */
	public static double[] getNDCToScreen(Component dr) {
		double[] NDCToScreen = Rn.identityMatrix(4);
//		System.out.println("Canvas is "+dr.getSize().toString());
//		System.out.println("x,y,w,h is: "+dr.getX()+"."+dr.getY()+"."+dr.getWidth()+" "+dr.getHeight());
		NDCToScreen[0] = .5 * dr.getWidth();
		NDCToScreen[5] = .5 * dr.getHeight();
		NDCToScreen[3] = .5 * dr.getWidth();//+dr.getX();
		NDCToScreen[7] = .5 * dr.getHeight();//+dr.getY();	
		return NDCToScreen;
	}


	/**
	 * @return
	 */
	public SceneGraphPath getCurrentPath() {
		return currentPath;
	}

	/**
	 * @param path
	 */
	public void setCurrentPath(SceneGraphPath path) {
		//if (path != null) currentPath = ((SceneGraphPath) path.clone());
		//if (path != null && path.getLength() > 0)
			currentPath = path;
	}

	/**
	 * @return
	 */
	public double[] getNDCToWorld() {
		return Rn.inverse(null, getWorldToNDC());
	}


}
