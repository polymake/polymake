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

import de.jreality.scene.tool.AxisState;
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
public class VirtualExtractPositive implements VirtualDevice {

  InputSlot inAxis, outSlot;
  
  AxisState state = null;
  
  double min=0.1;
  
  public ToolEvent process(VirtualDeviceContext context) throws MissingSlotException {
	if (state == null) {
		state = AxisState.ORIGIN;
		return new ToolEvent(this, outSlot, state);
	}
    double val = context.getAxisState(inAxis).doubleValue();
    if (updateState(val)) return new ToolEvent(context.getEvent().getSource(), context.getEvent().getTimeStamp(), outSlot, state);
    else return null;
  }

  /**
   * @param val
   * @return true if the state has changed
   */
  protected boolean updateState(double val) {
    if (state == AxisState.ORIGIN && val > min) {
      state = AxisState.PRESSED;
      return true;
    }
    if (state == AxisState.PRESSED && val <= min) {
      state = AxisState.ORIGIN;
      return true;
    }
    return false;
  }

  public void initialize(List inputSlots, InputSlot result, Map configuration) {
    inAxis = (InputSlot) inputSlots.get(0);
    outSlot = result;
    try {
    	min = ((Double)configuration.get("threshold")).doubleValue();
    } catch (NullPointerException npe) {}
    catch (NumberFormatException e) {}
  }

  public void dispose() {
  }

  public String getName() {
    return "ExtractPositive";
  }
  
  public String toString() {
    return "Virtual Device: "+getName();
  }

}
