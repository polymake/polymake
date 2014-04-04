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
 * @author weissman
 *
 **/
public class FlyTool extends AbstractTool {
  
  private final transient InputSlot forwardBackwardSlot = InputSlot.getDevice("ForwardBackwardAxis");
  private final transient InputSlot leftRightSlot = InputSlot.getDevice("LeftRightAxis");
  private final transient InputSlot timerSlot = InputSlot.getDevice("SystemTime");
  
  private transient double velocity;
  private transient boolean isFlying;
  
  private double[] olddir = {0,0,-1,0};
  private double gain=1, dGain = 1.1;
  private boolean raiseToThirdPower = true;
  
  public FlyTool() {
	  addCurrentSlot(forwardBackwardSlot);
	  addCurrentSlot(leftRightSlot);
  }
  
  EffectiveAppearance eap;
  
  public void perform(ToolContext tc) {
	if (tc.getSource() == forwardBackwardSlot) {
		velocity = tc.getAxisState(forwardBackwardSlot).doubleValue();
		// TODO make this transformation an option 
		if (raiseToThirdPower) velocity = velocity*velocity*velocity;
		if (tc.getAxisState(forwardBackwardSlot).isReleased()) {
			isFlying = false;
			removeCurrentSlot(timerSlot);
			return;
		}
		if (!isFlying) {
			isFlying = true;
			addCurrentSlot(timerSlot);
		}
		return;
	}
	if (tc.getSource() == leftRightSlot)	{
		double changer = tc.getAxisState(leftRightSlot).doubleValue();
		if (changer == 0.0) return;
		gain *= (changer < 0 ? 1.0/dGain : dGain);
		System.err.println("Gain is "+gain);
		return;
	}
    if (eap == null || !EffectiveAppearance.matches(eap, tc.getRootToToolComponent())) {
        eap = EffectiveAppearance.create(tc.getRootToToolComponent());
      }
    int metric = eap.getAttribute("metric", Pn.EUCLIDEAN);
    LoggingSystem.getLogger(this).fine("metric is "+metric);
    SceneGraphComponent ship = tc.getRootToToolComponent().getLastComponent();

    Matrix pointerMatrix = new Matrix(tc.getTransformationMatrix(InputSlot.getDevice("PointerTransformation")));
    Matrix localPointer = ToolUtility.worldToTool(tc, pointerMatrix);
    double[] dir = localPointer.getColumn(2); // z-axis ( modulo +/- )
    //System.out.println("");
    if (metric != Pn.EUCLIDEAN) {
    	Pn.normalize(dir, dir, metric);
        // have to be careful in non-euclidean case that we don't flip the direction inadvertantly
        if (Rn.innerProduct(dir, olddir) < 0) for (int i = 0; i<4; ++i) dir[i] = -dir[i];
    }
    double[] shipPosition = localPointer.getColumn(3);
   //System.out.println("FlyTool: dir is "+Rn.toString(dir));
   
    Matrix shipMatrix = new Matrix();
    if (ship.getTransformation() != null) shipMatrix.assignFrom(ship.getTransformation());
      
    // the new position also depends on the metric;
    // val is the distance we have moved in the direction dir
    // use dragTowards to calculate the resulting point
    double val = tc.getAxisState(timerSlot).intValue()*0.001;    
    //Rn.times(dir, val*gain*velocity, dir);
    val = val*gain*velocity;
    double[] newShipPosition = Pn.dragTowards(null, shipPosition, dir, val, metric);
    //System.out.println("FlyTool: old position is "+Rn.toString(Pn.normalize(shipPosition, shipPosition,metric)));
    //System.out.println("FlyTool: new position is "+Rn.toString(Pn.normalize(newShipPosition,newShipPosition, metric)));
    MatrixBuilder.init(shipMatrix, metric).translateFromTo(shipPosition,newShipPosition).assignTo(ship);
    ship.getTransformation().setMatrix(P3.orthonormalizeMatrix(null, ship.getTransformation().getMatrix(), 10E-10, metric));    tc.getViewer().render();
    System.arraycopy(dir, 0, olddir, 0, 4);
  }

  public double getGain() {
  	return gain;
  }
  
  public void setGain(double gain) {
  	this.gain = gain;
  }

}
