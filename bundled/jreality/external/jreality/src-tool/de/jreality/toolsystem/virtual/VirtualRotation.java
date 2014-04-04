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


package de.jreality.toolsystem.virtual;

import java.util.List;
import java.util.Map;

import de.jreality.math.Matrix;
import de.jreality.math.P3;
import de.jreality.math.Rn;
import de.jreality.scene.data.DoubleArray;
import de.jreality.scene.tool.InputSlot;
import de.jreality.toolsystem.MissingSlotException;
import de.jreality.toolsystem.ToolEvent;
import de.jreality.toolsystem.VirtualDevice;
import de.jreality.toolsystem.VirtualDeviceContext;

/**
 * @author weissman
 *
 **/
public class VirtualRotation implements VirtualDevice {
  
  InputSlot pointerNDC;
  InputSlot cameraToWorld;
  
  InputSlot out;

  private double gain = 4;
//  private double aspectRatio = 1;

  private Matrix result = new Matrix();
  private DoubleArray da = new DoubleArray(result.getArray());

  private double oldX = Integer.MAX_VALUE, oldY;
  private double[] mouseCoords = new double[3], mouseCoordsOld = new double[3];
  
  public ToolEvent process(VirtualDeviceContext context)
      throws MissingSlotException {
	  if (context.getEvent().getInputSlot()==cameraToWorld) return null;
    DoubleArray pointer = context.getTransformationMatrix(pointerNDC);
    if (pointer == null) throw new MissingSlotException(pointerNDC);
    if (oldX == Integer.MAX_VALUE) {
      oldX = pointer.getValueAt(3);
      oldY = pointer.getValueAt(7);
      return null;
    }
    double x = pointer.getValueAt(3);
    double y = pointer.getValueAt(7);
    double dist = x*x + y*y;
    double z = 2>dist?Math.sqrt(2 - dist) : 0;
    Rn.setToValue(mouseCoords, x, y, z);
    
    mouseCoords = Rn.normalize(mouseCoords, mouseCoords);
    double[] cross = Rn.crossProduct(null, mouseCoordsOld, mouseCoords);
    double angle = gain*Math.asin(Rn.euclideanNorm(cross));
    double[] cross4 = {cross[0], cross[1], cross[2], 0};
    cross = new Matrix(context.getTransformationMatrix(cameraToWorld)).multiplyVector(cross4);
    result.assignFrom(P3.makeRotationMatrix(null, cross, angle));
    Rn.setToValue(mouseCoordsOld, x, y, z);
    return new ToolEvent(this, context.getEvent().getTimeStamp(), out, da);
  }

  public void initialize(List inputSlots, InputSlot result, Map configuration) {
    pointerNDC = (InputSlot) inputSlots.get(0);
    cameraToWorld = (InputSlot) inputSlots.get(1);
    out = result;
    if (configuration != null)
    try {
      gain = ((Double)configuration.get("gain")).doubleValue();
    } catch (Exception e) {
    }
  }

  public void dispose() {
  }

  public String getName() {
    return "Virtual: Rotation";
  }

}
