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


package de.jreality.tutorial.util;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.Vector;

import de.jreality.math.Matrix;
import de.jreality.math.MatrixBuilder;
import de.jreality.math.P3;
import de.jreality.math.Pn;
import de.jreality.math.Rn;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.tool.AbstractTool;
import de.jreality.scene.tool.InputSlot;
import de.jreality.scene.tool.ToolContext;
import de.jreality.shader.EffectiveAppearance;
import de.jreality.toolsystem.ToolUtility;
import de.jreality.util.LoggingSystem;

/**
 * This tool is designed to sit "above" the camera in the "ship" or "avatar" node and "fly"
 * in the direction determined by the "PointerShipTransformation" (PST) virtual device.  That is, along a 
 * line determined by the last column (position) and next-to-last column (direction) of the
 * matrix associated to the PST. It operates in a metric-neutral way.
 * @author gunn
 *
 **/
public class FlyTool extends AbstractTool {
  
  private final transient InputSlot forwardBackwardSlot = InputSlot.getDevice("ForwardBackwardAxis");
  private final transient InputSlot shiftForwardBackwardSlot = InputSlot.getDevice("ShiftForwardBackwardAxis");
  private final transient InputSlot leftRightSlot = InputSlot.getDevice("LeftRightAxis");
  private final transient InputSlot shiftLeftRightSlot = InputSlot.getDevice("ShiftLeftRightAxis");
  private final transient InputSlot timerSlot = InputSlot.getDevice("SystemTime");
  private transient InputSlot currentKeySlot;
  private transient double velocity;
  private transient boolean flying, released;
  
  private double[] olddir = {0,0,1,0};
  private double gain=1, rotateGain = .25;
  private Matrix lastStep = new Matrix();
  public FlyTool() {
	  addCurrentSlot(forwardBackwardSlot);
	  addCurrentSlot(shiftForwardBackwardSlot);
	  addCurrentSlot(leftRightSlot);
	  addCurrentSlot(shiftLeftRightSlot);
  }
  
  int metric = Pn.EUCLIDEAN;
  boolean readFromAp = true;
  EffectiveAppearance eap;
  boolean  shiftIsRotate = true;
  
  public void perform(ToolContext tc) {
		if (tc.getSource() == forwardBackwardSlot) {
			currentKeySlot = forwardBackwardSlot;
		} else if (tc.getSource() == shiftForwardBackwardSlot) {
			currentKeySlot = shiftForwardBackwardSlot;
		} else if (tc.getSource() == leftRightSlot) {
			currentKeySlot = leftRightSlot;
		} else if (tc.getSource() == shiftLeftRightSlot){
			currentKeySlot = shiftLeftRightSlot;
		} //else currentKeySlot = null;
		if (currentKeySlot != null) {
			released = tc.getAxisState(currentKeySlot).isReleased();
			if (released) {
				flying = false;
				removeCurrentSlot(timerSlot);
				tc.getViewer().getSceneRoot().setPickable( true);
				return;
			} else {
				flying = true;
				velocity = tc.getAxisState(currentKeySlot).doubleValue();
				velocity = velocity * velocity * velocity;
				addCurrentSlot(timerSlot);
				tc.getViewer().getSceneRoot().setPickable(false);
			}
		}
	if (!flying) return;
	if (readFromAp)	{
	    if (eap == null || !EffectiveAppearance.matches(eap, tc.getRootToToolComponent())) {
	        eap = EffectiveAppearance.create(tc.getRootToToolComponent());
	      }
	    metric = eap.getAttribute("metric", Pn.EUCLIDEAN);		
	}
    LoggingSystem.getLogger(this).fine("metric is "+metric);
    shipSGC = tc.getRootToToolComponent().getLastComponent();
	shipMatrix = new Matrix();
	if (shipSGC.getTransformation() != null) shipMatrix.assignFrom(shipSGC.getTransformation());
      
    double val = tc.getAxisState(timerSlot).intValue();    
    forwardVal = val*velocity*.001;
	double[] dir = null;
	pointerMatrix = new Matrix(tc.getTransformationMatrix(InputSlot.getDevice("PointerTransformation")));
	localPointer = ToolUtility.worldToTool(tc, pointerMatrix);
	if (currentKeySlot == forwardBackwardSlot)	{
        	int direction = 2;
        	moveShipInDirection(direction);   		
   	} else if (currentKeySlot == shiftForwardBackwardSlot) {
   		if (shiftIsRotate)	
   			MatrixBuilder.init(shipMatrix, metric).rotateX(rotateGain*forwardVal).assignTo(shipSGC);  
   		else moveShipInDirection(1);   		
    } else if (currentKeySlot == leftRightSlot) {
        	MatrixBuilder.init(shipMatrix, metric).rotateY(rotateGain*forwardVal).assignTo(shipSGC);  
    } else if (currentKeySlot == shiftLeftRightSlot){
    	if (shiftIsRotate)	
        	MatrixBuilder.init(shipMatrix, metric).rotateZ(rotateGain*forwardVal).assignTo(shipSGC);  
   		else moveShipInDirection(0);   		
    }
    broadcastChange();
  }

private void moveShipInDirection(int direction) {
	double[] dir;
	dir = localPointer.getColumn(direction); 
	if (metric != Pn.EUCLIDEAN) {
	    if (Rn.innerProduct(dir, olddir, 4 ) < 0) 
	    	for (int i = 0; i<4; ++i) dir[i] = -dir[i];
	    }
	double[] shipPosition = localPointer.getColumn(3);
	dir[3] = 0.0;  
	shipPosition = new double[]{0,0,0,1}; 
	double[] newShipPosition = Pn.dragTowards(null, shipPosition, dir, gain*forwardVal, metric); //Pn.EUCLIDEAN); //
	MatrixBuilder.init(null, metric).translateFromTo(shipPosition,newShipPosition).assignTo(lastStep);
	MatrixBuilder.init(shipMatrix, metric).times(lastStep).assignTo(shipSGC);
	if (metric != Pn.EUCLIDEAN)
		shipSGC.getTransformation().setMatrix(P3.orthonormalizeMatrix(null, shipSGC.getTransformation().getMatrix(), 10E-10, metric));        	
	System.arraycopy(dir, 0, olddir, 0, 4);
}
	
	public double getGain() {
	  	return gain;
	}
	  
	public void setGain(double gain) {
	  	this.gain = gain;
	}

	public void setMetric(int sig)	{
		if (sig < -1) { readFromAp = true; return; }	// to turn on reading from appearance again
		metric = sig;
		readFromAp = false;
	}
	Vector<ActionListener> listeners = new Vector<ActionListener>();
	private Matrix pointerMatrix;
	private Matrix localPointer;
	private double forwardVal;
	private Matrix shipMatrix;
	private SceneGraphComponent shipSGC;
	
	
	public  void addChangeListener(ActionListener l)	{
		if (listeners.contains(l)) return;
		listeners.add(l);
	}
	
	public  void removeChangeListener(ActionListener l)	{
		listeners.remove(l);
	}
	public  void broadcastChange()	{
		if (listeners == null) return;
		ActionEvent e = new ActionEvent(this,0,null);
		//SyJOGLConfiguration.theLog.log(Level.INFO,"SelectionManager: broadcasting"+listeners.size()+" listeners");
		if (!listeners.isEmpty())	{
			//JOGLConfiguration.theLog.log(Level.INFO,"SelectionManager: broadcasting"+listeners.size()+" listeners");
			for (ActionListener l : listeners)	{
				l.actionPerformed(e);
			}
		}
	}

	public boolean isShiftIsRotate() {
		return shiftIsRotate;
	}

	public void setShiftIsRotate(boolean shiftIsRotate) {
		this.shiftIsRotate = shiftIsRotate;
	}

}
