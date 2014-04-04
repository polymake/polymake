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

import de.jreality.geometry.BoundingBoxUtility;
import de.jreality.math.FactoredMatrix;
import de.jreality.math.Matrix;
import de.jreality.math.MatrixBuilder;
import de.jreality.math.P3;
import de.jreality.math.Pn;
import de.jreality.math.Rn;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.Transformation;
import de.jreality.scene.pick.PickResult;
import de.jreality.scene.tool.AbstractTool;
import de.jreality.scene.tool.InputSlot;
import de.jreality.scene.tool.ToolContext;
import de.jreality.shader.EffectiveAppearance;
import de.jreality.util.Rectangle3D;

/**
 *
 * TODO: document this
 *
 * @author brinkman
 *
 */
public class RotateTool extends AbstractTool {

  static InputSlot activationSlot = InputSlot.getDevice("RotateActivation");
  static InputSlot evolutionSlot = InputSlot.getDevice("TrackballTransformation");

  boolean fixOrigin=false;
  private boolean rotateOnPick=false;
  
  public RotateTool() {
    super(activationSlot);
    addCurrentSlot(evolutionSlot);
  }

  transient protected SceneGraphComponent comp;

  transient Matrix center=new Matrix();
  
  transient EffectiveAppearance eap;
  public void activate(ToolContext tc) {
    startTime = tc.getTime();
    comp = (moveChildren ? tc.getRootToLocal():tc.getRootToToolComponent()).getLastComponent();
    // stop possible animation
    AnimatorTool.getInstance(tc).deschedule(comp);
    // TODO is this legitimate?  perhaps we should introduce a boolean "forceNewTransformation"
    if (comp.getTransformation() == null)
      comp.setTransformation(new Transformation());
    if (!fixOrigin){
    	PickResult currentPick = tc.getCurrentPick();
		if(rotateOnPick && currentPick!=null)
    		center=getRotationPoint(tc);
    	else
    		center = getCenter(comp);
    }
    if (eap == null || !EffectiveAppearance.matches(eap, tc.getRootToToolComponent())) {
        eap = EffectiveAppearance.create(tc.getRootToToolComponent());
      }
      metric = eap.getAttribute("metric", Pn.EUCLIDEAN);

  }
  
  private Matrix getCenter(SceneGraphComponent comp) {
	  Matrix centerTranslation = new Matrix();
	    Rectangle3D bb = BoundingBoxUtility.calculateChildrenBoundingBox(comp);
	    // need to respect the metric here
	    MatrixBuilder.init(null, metric).translate(bb.getCenter()).assignTo(centerTranslation);
	    return centerTranslation;
  }
  private Matrix getRotationPoint(ToolContext tc){
	  PickResult currentPick = tc.getCurrentPick();
	double[] obj=currentPick.getObjectCoordinates();
	  double[] pickMatr = currentPick.getPickPath().getMatrix(null);
	  SceneGraphPath compPath=(moveChildren ? tc.getRootToLocal():tc.getRootToToolComponent());
	  double[] compMatrInv=compPath.getInverseMatrix(null);
	  double[] matr=Rn.times(null, compMatrInv,pickMatr);
	  double[] rotationPoint =Rn.matrixTimesVector(null, matr, obj);
	  
	  Matrix centerTranslation = new Matrix();
	    MatrixBuilder
	     .init(null, metric)
	     .translate(rotationPoint)
	     .assignTo(centerTranslation);
	    return centerTranslation;
  }
  
  transient private int metric;

  transient Matrix result = new Matrix();
  transient Matrix evolution = new Matrix();
  
  transient private double startTime;
  
  private boolean moveChildren;

  private double animTimeMin=250;
  private double animTimeMax=750;
  private boolean updateCenter;
  protected boolean success = false;
  public void perform(ToolContext tc) {
	  success = false;
    Matrix object2avatar = new Matrix((moveChildren ? tc.getRootToLocal():tc.getRootToToolComponent()).getInverseMatrix(null)); 
    if (Rn.isNan(object2avatar.getArray())) {
    	return;
    }
	try {
	    object2avatar.assignFrom(P3.extractOrientationMatrix(null, object2avatar.getArray(), P3.originP3, metric));
	} catch (Exception e)	{
	    MatrixBuilder.euclidean().assignTo(object2avatar);	// set identity matrix
	}
    evolution.assignFrom(tc.getTransformationMatrix(evolutionSlot));
    evolution.conjugateBy(object2avatar);
    if (!fixOrigin && updateCenter){ 
    	PickResult currentPick = tc.getCurrentPick();
		if(rotateOnPick && currentPick!=null)
    		center=getRotationPoint(tc);
    	else
    	center = getCenter(comp);
    }
   if (metric != Pn.EUCLIDEAN)
	   P3.orthonormalizeMatrix(evolution.getArray(), evolution.getArray(), 10E-8, metric);
	result.assignFrom(comp.getTransformation());
    if (!fixOrigin) result.multiplyOnRight(center);
    result.multiplyOnRight(evolution);
    if (!fixOrigin) result.multiplyOnRight(center.getInverse());
    if (Rn.isNan(result.getArray())) return;
    success = true;
    comp.getTransformation().setMatrix(result.getArray());
  }

  public void deactivate(ToolContext tc) {
	  double t = tc.getTime()-startTime; 
    if (t > animTimeMin && t < animTimeMax) {
      AnimatorTask task = new AnimatorTask() {
        FactoredMatrix e = new FactoredMatrix(evolution, Pn.EUCLIDEAN);
        double rotAngle = e.getRotationAngle();
        double[] axis = e.getRotationAxis();
        {
          if (rotAngle > Math.PI) rotAngle = -2*Math.PI+rotAngle;
        }
        Matrix cen = new Matrix(center);
        SceneGraphComponent c = comp;
        public boolean run(double time, double dt) {
          if (updateCenter) cen = getCenter(c);
          MatrixBuilder m = MatrixBuilder.euclidean(c.getTransformation());
    		  m.times(cen);
    		  //m.multiplyOnRight(e);
    		  m.rotate(0.05*dt*rotAngle, axis);
          m.times(cen.getInverse());
    		  m.assignTo(c);
          return true;
        }
      };
      AnimatorTool.getInstance(tc).schedule(comp, task);
    }
  }
  
  public boolean getMoveChildren() {
    return moveChildren;
  }
  public void setMoveChildren(boolean moveChildren) {
    this.moveChildren = moveChildren;
  }

  public double getAnimTimeMax() {
  	return animTimeMax;
  }
  
  public void setAnimTimeMax(double animTimeMax) {
  	this.animTimeMax = animTimeMax;
  }
  
  public double getAnimTimeMin() {
  	return animTimeMin;
  }
  
  public void setAnimTimeMin(double animTimeMin) {
  	this.animTimeMin = animTimeMin;
  }
  
  public boolean isUpdateCenter() {
  	return updateCenter;
  }
  
  public void setUpdateCenter(boolean updateCenter) {
  	this.updateCenter = updateCenter;
  	if (!updateCenter)
  		center=new Matrix();
  }
  
  public boolean isFixOrigin() {
  	return fixOrigin;
  }
  
  public void setFixOrigin(boolean fixOrigin) {
  	this.fixOrigin = fixOrigin;
  }
  public boolean isRotateOnPick() {
	  return rotateOnPick;
  }
  public void setRotateOnPick(boolean rotateOnPick) {
	  this.rotateOnPick = rotateOnPick;
  }
}
