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
 * changed the state of its output axis from ORIGIN to
 * PRESSED whenever the switch (first) axis is PRESSED.
 * inverts its current state as long as the invert axis is pressed
 * 
 * @author weissman
 *
 **/
public class VirtualSwitchAndInvertAxis implements VirtualDevice {
    
    InputSlot switchIS;
    InputSlot invertIS;
    InputSlot out;
    
    boolean lastVal;
    boolean invert;
    
    Boolean value;
    
    public ToolEvent process(VirtualDeviceContext context)
            throws MissingSlotException {
      if (value == null) {
        invert = context.getAxisState(invertIS).isPressed();
        value = Boolean.valueOf(context.getAxisState(switchIS).isPressed());
        lastVal = value.booleanValue();
        if (invert) lastVal = !lastVal;
        return new ToolEvent(this, context.getEvent().getTimeStamp(), out, lastVal ? AxisState.PRESSED : AxisState.ORIGIN);
      }

      if (context.getEvent().getInputSlot() == switchIS && context.getAxisState(switchIS).isPressed())
        value = Boolean.valueOf(!value.booleanValue());

      if (context.getEvent().getInputSlot() == invertIS)
        invert = context.getAxisState(invertIS).isPressed();
      
      boolean newVal = value.booleanValue();
      if (invert) newVal = !newVal;
      if (newVal != lastVal) {
        lastVal = newVal;
        return new ToolEvent(this, context.getEvent().getTimeStamp(), out, lastVal ? AxisState.PRESSED : AxisState.ORIGIN);
      }
      return null;
    }

    public void initialize(List inputSlots, InputSlot result,
            Map configuration) {
      switchIS = (InputSlot) inputSlots.get(0);
      invertIS = (InputSlot) inputSlots.get(1);
      out = result;
    }

    public void dispose() {
        // TODO Auto-generated method stub

    }

    public String getName() {
        return "InvertAxis";
    }

    public String toString() {
        return "Virtual Device: "+getName();
    }
}
