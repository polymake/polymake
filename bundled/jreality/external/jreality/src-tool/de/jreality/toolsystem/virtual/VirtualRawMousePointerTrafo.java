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
import de.jreality.toolsystem.ToolEvent;
import de.jreality.toolsystem.VirtualDevice;
import de.jreality.toolsystem.VirtualDeviceContext;


/**
 *
 * TODO: comment this
 *
 * @author brinkman
 *
 */
public class VirtualRawMousePointerTrafo implements VirtualDevice {

  InputSlot outSlot;
  InputSlot ndcToWorldSlot;
  InputSlot pointerNdcSlot;
  
  double[] ndcToWorld = new double[16];
  double[] pointerNdc = new double[16];
  double[] pointerTrafo = new double[16];
  DoubleArray outArray = new DoubleArray(pointerTrafo);
  
  
  public ToolEvent process(VirtualDeviceContext context) {
  	try {
    ndcToWorld = context.getTransformationMatrix(ndcToWorldSlot).toDoubleArray(ndcToWorld);
    pointerNdc = context.getTransformationMatrix(pointerNdcSlot).toDoubleArray(pointerNdc);
  	} catch (Exception e) {
		return null;
	}
  
  	double x = pointerNdc[3];
  	double y = pointerNdc[7];
  	
  	pointerNdc[0] = x+1;
  	pointerNdc[4] = y;
  	pointerNdc[8] = -1;
  	pointerNdc[12] = 1;
  	
  	pointerNdc[1] = x;
  	pointerNdc[5] = y+1;
  	pointerNdc[9] = -1;
  	pointerNdc[13] = 1;
  	
  	pointerNdc[2] = x;
  	pointerNdc[6] = y;
  	pointerNdc[10] = 1;
  	pointerNdc[14] = 1;
  	
  	pointerTrafo = Rn.times(pointerTrafo, ndcToWorld, pointerNdc);
	return new ToolEvent(context.getEvent().getSource(), context.getEvent().getTimeStamp(), outSlot, outArray);
  }

  public void initialize(List inputSlots, InputSlot result, Map configuration) {
    outSlot = result;
    ndcToWorldSlot = (InputSlot) inputSlots.get(0);
    pointerNdcSlot = (InputSlot) inputSlots.get(1);
  }

  public void dispose() {
  }

  public String getName() {
    return "RawMousePointerTrafo";
  }

  public String toString() {
    return "VirtualDevice: "+getName();
  }
  
 }
