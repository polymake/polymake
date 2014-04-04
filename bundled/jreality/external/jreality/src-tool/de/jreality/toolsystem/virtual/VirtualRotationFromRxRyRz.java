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

import de.jreality.math.MatrixBuilder;
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
 * @author Steffen Weissmann
 *
 */
public class VirtualRotationFromRxRyRz implements VirtualDevice {

  InputSlot rxSlot, rySlot, rzSlot;
  InputSlot outSlot;
  
  double rx, ry, rz;
  
  double gain = 1;
  
  MatrixBuilder mb = MatrixBuilder.euclidean();
  DoubleArray outArray = new DoubleArray(mb.getArray());
  
  
  public ToolEvent process(VirtualDeviceContext context) {
	  try {
		  rx = context.getAxisState(rxSlot).doubleValue();
		  ry = context.getAxisState(rySlot).doubleValue();
		  rz = context.getAxisState(rzSlot).doubleValue();
	  } catch (MissingSlotException mse) {
		  System.out.println("VirtualRotationFromRxRyRz.process(): missing slot");
	  }
	  mb.reset();
	  mb.rotateX(rx * gain);
	  mb.rotateZ(ry * gain);
	  mb.rotateY(-rz * gain);
	  return new ToolEvent(context.getEvent().getSource(), context.getEvent().getTimeStamp(), outSlot, outArray);
  }

  public void initialize(List inputSlots, InputSlot result, Map configuration) {
    outSlot = result;
    rxSlot = (InputSlot) inputSlots.get(0);
    rySlot = (InputSlot) inputSlots.get(1);
    rzSlot = (InputSlot) inputSlots.get(2);
    if (configuration != null)
        try {
          gain = ((Double)configuration.get("gain")).doubleValue();
        } catch (Exception e) {
        }
  }

  public void dispose() {
  }

  public String getName() {
    return "RotationFromRxRyRz";
  }

  public String toString() {
    return "VirtualDevice: "+getName();
  }
  
 }
