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


package de.jreality.scene;

import java.awt.geom.Rectangle2D;
import java.util.logging.Level;

import de.jreality.scene.event.CameraEvent;
import de.jreality.scene.event.CameraEventMulticaster;
import de.jreality.scene.event.CameraListener;
import de.jreality.util.CameraUtility;
import de.jreality.util.LoggingSystem;
/**
 *
 * The camera represents essentially a projection from three dimensions into two, that is
 * a specification of a viewing frustrum. The camera coordinate system is assumed to
 * point down the positive z-axis. (Note: specific backends may flip the orientation of the
 * z-axis. This issue is handled separately from the camera description here).
 * <p>
 * All instance of Camera require specifying the near and far clipping planes.
 * <p>
 * The camera can be either perspective or orthographic.  If it is perspective, then
 * its viewing frustum can be specified by giving the field of view {@link #setFieldOfView(double)}. 
 * This implies that the camera is on-axis,
 * that is, the viewing frustum is centered on the z-axis.
 * <p>
 * There is also support for off-axis cameras ({@link #setOnAxis(boolean)}).  
 * Use the {@link #setViewPort(Rectangle2D)} method to
 * specify the desired viewport, which is assumed to lie in the <i>z=1</i> plane.  
 * In this case the field of view is ignored.
 * <p>
 * The camera also supports stereo viewing @link #setStereo(boolean). For most desktop environments
 * the only other required parameters are:
 * <ul>
 * <li>eye separation ({@link #setEyeSeparation(double)}), 
 * a horizontal displacement in camera coordinates, and</li>
 * <li>focus {@link #setFocus(double)}, the z-depth where the two images are identical.</li>
 * </ul>  
 * For CAVE-like environments
 * where the eyes are not always oriented horizontally, there is an additional parameter:
 *  an orientation matrix ({@link #setOrientationMatrix(double[])}), a 4x4 transformation matrix
 *  which  defines the rotation that has to be applied to the x-axis to get the
 *  line in camera coordinates on which the eyes lie. (This matrix should fix (0,0,0,1)!).
 *  <p> 
 * Default eye positions are (-eyeSeparation/2,0,0) and (eyeSeparation, 0,0) in the 
 * camera coordinate system.  Use {@link CameraUtility#getNDCToCamera(Camera, double, int)} to generate 
 * the appropriate projection matrices.
 * <p>
 * Instances of {@link de.jreality.scene.event.CameraListener} can register with the camera
 * to be notified 
 * when the camera changes.
 * <p>
 * Due to refactoring, the camera no longer has enough state to provide the perspective viewing transformation
 * from/to camera to/from Normalized Device Coordinates (NDC). It basically lacks the aspect ratio of the
 * output device. This allows to use the same camera for different viewers with
 * i. e. different window sizes. See {@link CameraUtility} for methods which provide this functionality.
 * 
 * 
 * @author Charles Gunn
 * @see de.jreality.util.CameraUtility
 * 
 */
public class Camera extends SceneGraphNode {
	double near, 
			far, 
			fieldOfView,
			aspectRatio,
			focus,
			fstop = -1,		// set this non-negative to activate depth of field (in renderman backend)
			focalLength = .1;
	Rectangle2D viewPort;
	private boolean 	isOnAxis = true,
			isPerspective = true,
			isStereo = false,
			isLeftEye = false,
			isRightEye = false;
	
	double eyeSeparation = 0.07;
	double[] orientationMatrix;		
	
  private CameraEventMulticaster cameraListener = new CameraEventMulticaster();

  private static int UNNAMED_ID;

  public Camera(String name) {
    super(name);
	near = .5;
	far = 50.0;
	fieldOfView = Math.toRadians(60.0);
	focus = 3.0;
  }
  
  public Camera() {
	  this("camera "+(UNNAMED_ID++));
  }


	public double getNear() {
		return near;
	}

	public void setNear(double d) {
		near = d;
		fireCameraChanged();
	}

	public double getFar() {
		return far;
	}

	public void setFar(double d) {
    if (far == d) return;
		far = d;
    fireCameraChanged();
	}

	public double getFieldOfView() {
		return (180.0/Math.PI) * fieldOfView;
	}

	public void setFieldOfView(double d) {
		double f = (Math.PI/180.0)*d;
		if (f == fieldOfView)  return;
		fieldOfView = f;
		fireCameraChanged();
	}

	public double getFocus() {
		return focus;
	}

	public void setFocus(double d) {
    if (focus == d) return;
		focus = d;
		fireCameraChanged();
	}

	public Rectangle2D getViewPort() {
		return viewPort;
	}

	public void setViewPort(Rectangle2D rectangle2D) {
		if (isOnAxis)	
			throw new IllegalStateException("Can't set viewport for an on-axis camera");
		viewPort = rectangle2D;
		fireCameraChanged();
	}

	public boolean isOnAxis() {
		return isOnAxis;
	}

	public void setOnAxis(boolean b) {
		if (isOnAxis == b) return;
		isOnAxis = b;
		fireCameraChanged();
	}

	public boolean isPerspective() {
		return isPerspective;
	}

	public void setPerspective(boolean b) {
    if (isPerspective == b) return;
		isPerspective = b;
		fireCameraChanged();
	}

	public double getEyeSeparation() {
		return eyeSeparation;
	}
	/**
	 * @param eyeSeparation The eyeSeparation to set.
	 */
	public void setEyeSeparation(double eyeSeparation) {
    if (this.eyeSeparation == eyeSeparation) return;
		this.eyeSeparation = eyeSeparation;
    fireCameraChanged();
	}
	/**
	 * The orientation matrix describes the transformation in
	 * camera coordinate system which describes the orientation of
	 * the head; the "standard" position is that the eyes are on the
	 * x-axis, up is the y-axis, and z is the direction of projection
	 * The orientation matrix is used for cameras such as those in the
	 * PORTAL.
	 * @return Returns the orientationMatrix.
	 */
	public double[] getOrientationMatrix() {
		return orientationMatrix;		// TODO Auto-generated method stub

	}

	public void setOrientationMatrix(double[] orientationMatrix) {
		this.orientationMatrix = orientationMatrix;
	   // TODO: maybe compare with old value?
		fireCameraChanged();
	}

	public boolean isStereo() {
		return isStereo;
	}
	
	public boolean isLeftEye() {
		return isLeftEye;
	}
	
	public boolean isRightEye() {
		return isRightEye;
	}

	public void setStereo(boolean isStereo) {
    if (this.isStereo == isStereo) return;
		this.isStereo = isStereo;
		if (!isPerspective)	{
 			LoggingSystem.getLogger(this).log(Level.WARNING,"Stereo camera must be perspective, setting it so.");
			isPerspective = true;
		}
		fireCameraChanged();
	}

	public void setLeftEye(boolean isLeftEye) {
    if (this.isLeftEye == isLeftEye) return;
		this.isLeftEye = isLeftEye;
		if (!isPerspective)	{
 			LoggingSystem.getLogger(this).log(Level.WARNING,"Stereo camera must be perspective, setting it so.");
			isPerspective = true;
		}
		fireCameraChanged();
	}

	public void setRightEye(boolean isRightEye) {
    if (this.isRightEye == isRightEye) return;
		this.isRightEye = isRightEye;
		if (!isPerspective)	{
 			LoggingSystem.getLogger(this).log(Level.WARNING,"Stereo camera must be perspective, setting it so.");
			isPerspective = true;
		}
		fireCameraChanged();
	}
	
	public void addCameraListener(CameraListener listener) {
    cameraListener.add(listener);
  }

  public void removeCameraListener(CameraListener listener) {
    cameraListener.remove(listener);
  }
  
  protected void fireCameraChanged() {
    final CameraListener cl = cameraListener;
    if (cl != null) cl.cameraChanged(new CameraEvent(this));
  }

	public void accept(SceneGraphVisitor v) {
	  v.visit(this);
	}
	
	static void superAccept(Camera c, SceneGraphVisitor v) {
	  c.superAccept(v);
	}
	private void superAccept(SceneGraphVisitor v) {
	  super.accept(v);
	}

	/**
	 * Values useful in Renderman backend.
	 * @return
	 */
	public double getFocalLength() {
		return focalLength;
	}

	/**
	 * Values useful in Renderman backend.
	 * @return
	 */
	public double getFStop() {
		return fstop;
	}

	public void setFStop(double fstop) {
		this.fstop = fstop;
	}

	public void setFocalLength(double focalLength) {
		this.focalLength = focalLength;
	}


 }
