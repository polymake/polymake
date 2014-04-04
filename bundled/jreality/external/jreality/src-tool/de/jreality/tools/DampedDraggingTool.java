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

import de.jreality.math.Matrix;
import de.jreality.math.MatrixBuilder;
import de.jreality.math.Pn;
import de.jreality.math.Rn;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.Transformation;
import de.jreality.scene.tool.AbstractTool;
import de.jreality.scene.tool.InputSlot;
import de.jreality.scene.tool.ToolContext;
import de.jreality.shader.EffectiveAppearance;


/** works just like the draggingTool
 *  has spezial feature:
 *  the movement of a dragged object 
 *  is damped(if the corresponding flag is set ). 
 *  If the mouse stops moving the object will  
 *  "float" to the same Point as if no damping where selected  
 * 
 *  you can modify how strong and fast the damping is
 * 
 * @author Gonska
 *
 */
public class DampedDraggingTool extends AbstractTool {

	// public
	private boolean moveChildren;
	private boolean damped=true;
	private double linearScaleFactor=500;
	private double featherPow=2;
	// private
	private transient boolean dragInViewDirection;
	protected transient SceneGraphComponent comp;
	private transient EffectiveAppearance eap;
	private transient int metric;
	private transient Matrix result = new Matrix();
	private transient Matrix local2world = new Matrix();
	private transient Matrix pointer = new Matrix();
	// extra private
	transient protected SceneGraphPath path;
	private double[] lostAxis= new double[] {0,0,1}; 	// differrence represents the lost evolution
	private double lostAngle=0;
	private Matrix newEvolution=new Matrix();	
	private long timeStamp=0;
	private double[] m3;
	
	// Slots
	static InputSlot activationSlot = InputSlot.getDevice("DragActivation");
	static InputSlot alongPointerSlot = InputSlot.getDevice("DragAlongViewDirection");
	static InputSlot evolutionSlot = InputSlot.getDevice("PointerEvolution");
	static InputSlot timerSlot = InputSlot.getDevice("SystemTime");

	
	public DampedDraggingTool() {
		super(activationSlot);
		addCurrentSlot(evolutionSlot);
		addCurrentSlot(alongPointerSlot);
		addCurrentSlot(timerSlot);
	}
	
	public void activate(ToolContext tc) {
		path=moveChildren ? tc.getRootToLocal() : tc.getRootToToolComponent();
		comp = path.getLastComponent();
///		 stop possible animation:
	    AnimatorTool.getInstance(tc).deschedule(comp);
	    if (comp.getTransformation() == null) comp.setTransformation(new Transformation());
		try {
			if (tc.getAxisState(alongPointerSlot).isPressed()) 
				dragInViewDirection = true;
			else dragInViewDirection = false;
		} 
		catch (Exception me) {dragInViewDirection = false;}
		if (eap == null || !EffectiveAppearance.matches(eap, tc.getRootToToolComponent())) 
			eap = EffectiveAppearance.create(tc.getRootToToolComponent()); 
		metric = eap.getAttribute("metric", Pn.EUCLIDEAN);
	}
	
	public void perform(ToolContext tc) {
		/// drag in View direction switch:
		if (tc.getSource() == alongPointerSlot) {
			if (tc.getAxisState(alongPointerSlot).isPressed()) 
				dragInViewDirection = true;			
			else dragInViewDirection = false;			
			return;
		}
		/// get evolution:
		Matrix evolution = new Matrix(tc.getTransformationMatrix(evolutionSlot));
		// need to convert from euclidean to possibly non-euclidean translation:
		if (metric != Pn.EUCLIDEAN)
			MatrixBuilder.init(null, metric).translate(evolution.getColumn(3)).assignTo(evolution);
		/// drag in View direction ( change evolution ) :
		if (dragInViewDirection&& tc.getSource()!=timerSlot ) {
			tc.getTransformationMatrix(InputSlot.getDevice("CameraToWorld")).toDoubleArray(pointer.getArray());
			double dz = evolution.getEntry(0,3)+evolution.getEntry(1,3);
			double[] tlate = Rn.times(null, dz, pointer.getColumn(2));
			if (metric==Pn.EUCLIDEAN) tlate[3] = 1.0;
			MatrixBuilder.init(null, metric).translate(tlate).assignTo(evolution);
		}
		/// wo wird geaendert:
		(moveChildren ? tc.getRootToLocal():tc.getRootToToolComponent()).getMatrix(local2world.getArray());		
		evolution.conjugateBy(local2world.getInverse());
		/// assign evolution:
		comp.getTransformation().getMatrix(result.getArray());		
		/// new : ----------------------------------------
		if ((damped)&&(!dragInViewDirection||	tc.getSource()==timerSlot)) {
			if(tc.getSource()==activationSlot)
				timeStamp=tc.getTime();
			double dt=(tc.getTime()-timeStamp);
			timeStamp+=dt;
			SceneGraphPath toCamPath=tc.getViewer().getCameraPath().popNew();
			double[] m1=path.getInverseMatrix(null);
			double[] m2=toCamPath.getMatrix(null);
			double[] m1i=path.getMatrix(null);
			double[] m2i=toCamPath.getInverseMatrix(null);
			 m3=Rn.times(null, m1, m2);		
			double[] m3i=Rn.times(null, m2i, m1i);
			if(tc.getSource()==timerSlot){
				recalcAngleAndAxis2(dt, MatrixBuilder.euclidean().getMatrix());
			}
			if(tc.getSource()==evolutionSlot){
				double[] camEvolution=Rn.conjugateByMatrix(null,  evolution.getArray(),m3i);
				recalcAngleAndAxis2(dt, new Matrix(camEvolution));
			}
			evolution=new Matrix(Rn.conjugateByMatrix(null,newEvolution.getArray(),m3 ));
		}
		if ((damped)||(tc.getSource()!=timerSlot))
				result.multiplyOnRight(evolution);
		/// new end -------------------------------------
		comp.getTransformation().setMatrix(result.getArray());
	}
	public void deactivate(ToolContext tc) {
		(moveChildren ? tc.getRootToLocal():tc.getRootToToolComponent()).getMatrix(local2world.getArray());		
		comp.getTransformation().getMatrix(result.getArray());
		SceneGraphPath toCamPath=tc.getViewer().getCameraPath().popNew();
		double[] m1=path.getInverseMatrix(null);
		double[] m2=toCamPath.getMatrix(null);
		 m3=Rn.times(null, m1, m2);		
		AnimatorTask task = new AnimatorTask() {
			public boolean run(double time, double dt) {
				timeStamp+=dt;
				boolean unFinished=recalcAngleAndAxis2(dt, MatrixBuilder.euclidean().getMatrix());
				newEvolution=new Matrix(Rn.conjugateByMatrix(null,newEvolution.getArray(),m3 ));
				result.multiplyOnRight(newEvolution);
				comp.getTransformation().setMatrix(result.getArray());
				return unFinished;
			}
		};
		AnimatorTool.getInstance(tc).schedule(comp, task);
	}

////	--------------------- getter setter -----------------
	public void setMoveChildren(boolean moveChildren) {
		this.moveChildren = moveChildren;
	}
	public boolean isMoveChildren() {
		return moveChildren;
	}
	public void setFeathered(boolean feathered) {
		this.damped = feathered;
	}
	public boolean isFeathered() {
		return damped;
	}
	public void setLinearScaleFactor(double linearScaleFactor) {
		if(linearScaleFactor>0)
			this.linearScaleFactor = linearScaleFactor*1000;
		else this.linearScaleFactor = 1000;
	}
	public double getLinearScaleFactor() {
		return linearScaleFactor/1000;
	}
	public void setFeatherPow(double featherPow) {
		if(featherPow>=1)
			this.featherPow = featherPow;
		else this.featherPow =1;
	}
	public double getFeatherPow() {
		return featherPow;
	}
	@Override
	public String getDescription() {
		return "featheres the movement";
	}
	@Override
	protected void setDescription(InputSlot slot, String description) {}
	
//// --------------------- private() helper stuff -----------------
	private double goodAngle(double angle){
		while(angle>Math.PI)  angle-=2*Math.PI;
		while(angle<-Math.PI) angle+=2*Math.PI;
		return angle;
	}
	private double[] normalizedR3(double[] x){
		return Rn.normalize(null, new double[]{x[0],x[1],x[2]});
	}
	private double[] getRotAxisFromTo(double[] x,double[]y){
		if(x.length!=3||y.length!=3)return null;
		double[] result=new double[]{
				x[1]*y[2]-x[2]*y[1],
				-(x[0]*y[2]-x[2]*y[0]),
				x[0]*y[1]-x[1]*y[0]};
		Rn.normalize(result, result);
		return result;
	}
	private double allowedAngle(double angle, double dt){
		double tMinus=Math.pow(angle,1/featherPow);
		double destTMinus=Math.max(tMinus-dt/linearScaleFactor,0);
		double restAngle=Math.pow(destTMinus,featherPow);
		return angle-restAngle;
	}
	private boolean hasFinished(double angle, double dt){
		double tMinus=Math.pow(angle,1/featherPow);
		return tMinus<=0;
	}
	private boolean recalcAngleAndAxis2(double dt, Matrix evolution){	
		/// view Direction:
		double[] viewDir=new double[] {0,0,-1,0};// should be
		/// lost & total Evolution:
		double[] lostEvolution =
			MatrixBuilder.euclidean().rotate(lostAngle, lostAxis).getArray();
		double[] totalEvolution=Rn.times(null, evolution.getArray() , lostEvolution);
		/// total axis &angle:
		double[] destdir= Rn.matrixTimesVector(null, totalEvolution, viewDir);
		viewDir=normalizedR3(viewDir);
		destdir=normalizedR3(destdir);
		double totalAngle=goodAngle(Rn.euclideanAngle(viewDir, destdir));
		double[] totalAxis=getRotAxisFromTo(viewDir, destdir);
		/// possible evolution:
		double possAngle=allowedAngle(totalAngle,dt);
		double[] possibleEvolution=MatrixBuilder.euclidean().rotate(possAngle, totalAxis).getArray();
		/// lost data: 
		lostAngle=totalAngle-possAngle;
		lostAxis=totalAxis;
		newEvolution=new Matrix(possibleEvolution);
		return !hasFinished(totalAngle,dt);
	}
	
}
