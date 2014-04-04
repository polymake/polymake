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


package de.jreality.tools;

import de.jreality.math.FactoredMatrix;
import de.jreality.math.Matrix;
import de.jreality.math.MatrixBuilder;
import de.jreality.math.P3;
import de.jreality.math.Pn;
import de.jreality.portal.PortalCoordinateSystem;
import de.jreality.scene.Camera;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.tool.AbstractTool;
import de.jreality.scene.tool.InputSlot;
import de.jreality.scene.tool.ToolContext;
import de.jreality.shader.EffectiveAppearance;
import de.jreality.util.CameraUtility;

/**
 * A tool that sets the head matrix from the head tracker.
 * 
 * @author Steffen Weissman
 * 
 * Extended for non-euclidean metrics by Charles Gunn.
 *
 **/
public class RemotePortalHeadMoveTool extends AbstractTool {
  
  static final transient InputSlot headSlot = InputSlot.getDevice("ShipHeadTransformation");

  Matrix headMatrix = new Matrix();
  Matrix worldToCamera = new Matrix();
  FactoredMatrix portal = new FactoredMatrix();
  EffectiveAppearance eap = null;
  private int metric;
    
  public RemotePortalHeadMoveTool() {
    addCurrentSlot(headSlot, "the current head matrix in PORTAL coordinates");
  }
  
  public void perform(ToolContext tc) {
    tc.getTransformationMatrix(headSlot).toDoubleArray(headMatrix.getArray());
	   if (eap == null || !EffectiveAppearance.matches(eap, tc.getRootToToolComponent())) {
	        eap = EffectiveAppearance.create(tc.getRootToToolComponent());
	      }
	metric = eap.getAttribute("metric", Pn.EUCLIDEAN);
	setHeadMatrix(headMatrix, tc.getViewer().getCameraPath(), tc.getAvatarPath());
  }
  private void setHeadMatrix(Matrix head, SceneGraphPath cameraPath, SceneGraphPath portalPath) {

	Camera camera = (Camera) cameraPath.getLastElement();
//	metric = Pn.ELLIPTIC;
	// the transformation of the camera node is headTranslation * cameraOrientation
	MatrixBuilder.init(null, metric).translate(head.getColumn(3)).times(CameraUtility.cameraOrientation).assignTo(cameraPath.getLastComponent());
//	System.err.println("in setheadmatrix");
	// calculate and set camera orientation matrix:
	double[] mat = cameraPath.getLastComponent().getTransformation().getMatrix();
//	System.err.println("head matrix = "+Rn.matrixToString(mat));
	head.setColumn(3, P3.originP3);
	Matrix camOrientationMatrix = MatrixBuilder.euclidean().times(CameraUtility.inverseCameraOrientation).times(head).getMatrix();
	camera.setOrientationMatrix(camOrientationMatrix.getArray());
    
    // assign camera viewport
	cameraPath.getInverseMatrix(worldToCamera.getArray());
	//portal.assignFrom(portalPath.getMatrix(portal.getArray()));
	Matrix portalToWorld = new Matrix(portalPath.getMatrix(null));
	//double[] portalOriginInCamCoordinates = worldToCamera.multiplyVector(portal.getTranslation());
	double[] portalOriginInCamCoordinates = worldToCamera.multiplyVector(portalToWorld.getColumn(3));
	Pn.dehomogenize(portalOriginInCamCoordinates, portalOriginInCamCoordinates);

    PortalCoordinateSystem.setPORTALViewport(portalOriginInCamCoordinates, camera);
  }


}
