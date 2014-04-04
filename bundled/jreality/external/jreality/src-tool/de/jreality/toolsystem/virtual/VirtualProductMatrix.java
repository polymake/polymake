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

import de.jreality.math.Rn;
import de.jreality.scene.data.DoubleArray;
import de.jreality.scene.tool.InputSlot;
import de.jreality.toolsystem.MissingSlotException;
import de.jreality.toolsystem.ToolEvent;
import de.jreality.toolsystem.VirtualDevice;
import de.jreality.toolsystem.VirtualDeviceContext;


/**
 *
 * TODO: comment this
 *
 * @author weissman
 *
 */
public class VirtualProductMatrix implements VirtualDevice {

  InputSlot leftSlot, rightSlot;
  InputSlot productSlot;
  double[] product = new double[16];
  
  double[] matrixL = new double[16];
  double[] matrixR = new double[16];
  
  public ToolEvent process(VirtualDeviceContext context) throws MissingSlotException {
    try {
      DoubleArray matrixLeft = context.getTransformationMatrix(leftSlot);
      matrixL = matrixLeft.toDoubleArray(matrixL);
    } catch (NullPointerException npe) {
      throw new MissingSlotException(leftSlot);
    }
    try {
    DoubleArray matrixRight = context.getTransformationMatrix(rightSlot);
      matrixR = matrixRight.toDoubleArray(matrixR);
    } catch (NullPointerException npe) {
      throw new MissingSlotException(rightSlot);
    }
      product = Rn.times(product, matrixL, matrixR);
      return new ToolEvent(context.getEvent().getSource(), context.getEvent().getTimeStamp(), productSlot, new DoubleArray(product));
  }

  public void initialize(List inputSlots, InputSlot result, Map configuration) {
    productSlot = result;
    leftSlot = (InputSlot) inputSlots.get(0);
    rightSlot = (InputSlot) inputSlots.get(1);
  }

  public void dispose() {
  }

  public String getName() {
    return "ProductMatrix";
  }

  public String toString() {
    return "VirtualDevice: "+getName();
  }
}
