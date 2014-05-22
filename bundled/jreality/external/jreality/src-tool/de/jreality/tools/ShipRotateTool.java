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
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.tool.AbstractTool;
import de.jreality.scene.tool.InputSlot;
import de.jreality.scene.tool.ToolContext;

/**
 * @author weissman
 *
 **/
public class ShipRotateTool extends AbstractTool {
  
  InputSlot leftRightSlot = InputSlot.getDevice("LeftRightAxis");
  InputSlot timerSlot = InputSlot.getDevice("SystemTime");
  
  double angle;
  boolean isRotating;
  
  double gain=1;
  
  public ShipRotateTool() {
	  addCurrentSlot(leftRightSlot);
  }
  
  double[] tmp = new double[16];
  public void perform(ToolContext tc) {
	if (tc.getSource() == leftRightSlot) {
		angle = tc.getAxisState(leftRightSlot).doubleValue();
		angle = angle*angle*angle;
		if (tc.getAxisState(leftRightSlot).isReleased()) {
			isRotating = false;
			removeCurrentSlot(timerSlot);
			return;
		}
		if (!isRotating) {
			isRotating = true;
			addCurrentSlot(timerSlot);
		}
		return;
	}
    SceneGraphComponent ship = tc.getRootToToolComponent().getLastComponent();

    Matrix shipMatrix = new Matrix();
    if (ship.getTransformation() != null) shipMatrix.assignFrom(ship.getTransformation());

    double val = tc.getAxisState(timerSlot).intValue()*0.001;
    MatrixBuilder.euclidean(shipMatrix).rotateY(val*angle*gain).assignTo(ship);
  }

  public double getGain() {
  	return gain;
  }
  
  public void setGain(double gain) {
  	this.gain = gain;
  }

}
