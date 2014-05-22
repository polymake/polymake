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

import de.jreality.scene.data.DoubleArray;
import de.jreality.scene.tool.InputSlot;
import de.jreality.toolsystem.MissingSlotException;
import de.jreality.toolsystem.ToolEvent;
import de.jreality.toolsystem.VirtualDevice;
import de.jreality.toolsystem.VirtualDeviceContext;

/**
 * This virtual device translates the 3 of the axes of a GameTrack (as reported
 * by the DevicejinputJoystick) into a translation. here are some snippets for
 * the toolconfig.xml asuming that the GameTrack is the 4th device found by the
 * jinput library:<br/> <code>
 * <rawdevices>
 * ...
 * <rawdevice id="Joystick" type="de.jreality.toolsystem.raw.DeviceJinputJoystick"/>
 </rawdevices>
 
 <rawslots>
 ...
 <mapping device="Joystick" src="axis_3_0" target="GtXaxis"/>
 <mapping device="Joystick" src="axis_3_1" target="GtYaxis"/>
 <mapping device="Joystick" src="axis_3_2" target="GtZaxis"/>
 <mapping device="Joystick" src="axis_3_3" target="GtUaxis"/>
 <mapping device="Joystick" src="axis_3_4" target="GtVaxis"/>
 <mapping device="Joystick" src="axis_3_5" target="GtWaxis"/>
 </rawslots>
 
 <virtualdevices>
 ...
 <virtualdevice type="de.jreality.toolsystem.virtual.VirtualGameTrackTranslation">
 <inputslot>GtXaxis</inputslot>
 <inputslot>GtYaxis</inputslot>
 <inputslot>GtZaxis</inputslot>
 <outputslot>GameTrackTranslationL</outputslot>
 <prop name="offset">
 <double>0.05</double>
 </prop>
 </virtualdevice>

 <virtualdevice type="de.jreality.toolsystem.virtual.VirtualGameTrackTranslation">
 <inputslot>GtUaxis</inputslot>
 <inputslot>GtVaxis</inputslot>
 <inputslot>GtWaxis</inputslot>
 <outputslot>GameTrackTranslationR</outputslot>
 <prop name="offset">
 <double>-0.05</double>
 </prop>
 </virtualdevice>
 </virtualdevices>
 </code>
 * The offset parameter for the virtual device is an offset in x direction and
 * can be used to place the two GameTrack points at an distance equivalent to
 * the real world spacing.
 * The device has some heuristic for not sending only changes of all axes at once if possible.
 * @author weissman
 */
public class VirtualGameTrackTranslation implements VirtualDevice {
    private double offset = .0;
    
    InputSlot in1;
    InputSlot in2;
    InputSlot in3;

    InputSlot out;
    
    boolean initialized;
    
    double oldPhi;
    int n = 0;
// boolean phiCh, thCh, hCh;
    
    double[] trafo = new double[16];
    DoubleArray outArray = new DoubleArray(trafo);
    
    
    public ToolEvent process(VirtualDeviceContext context)
            throws MissingSlotException {
        
        double phi = ( (context.getAxisState(in1).doubleValue())*Math.PI/4.f);
        if (!initialized  || phi != oldPhi || n>2) {
            initialized = true;
            oldPhi = phi;
            n = 0;
            double theta = ( (context.getAxisState(in2).doubleValue())*Math.PI/4.f);
            double h = 1-context.getAxisState(in3).doubleValue();

            trafo[3] =  - offset+(float) (h*Math.sin(phi)*Math.cos(theta));
            trafo[7] = (float) (h*Math.cos(phi)*Math.cos(theta));
            trafo[11] = (float) (-h*Math.sin(theta));
            
            return new ToolEvent(context.getEvent().getSource(), out, outArray);
            
        } else {
            n++;
            return null;
        }
    }

    public void initialize(List inputSlots, InputSlot result,
            Map configuration) {
      in1 = (InputSlot) inputSlots.get(0);
      in2 = (InputSlot) inputSlots.get(1);
      in3 = (InputSlot) inputSlots.get(2);
      out = result;
      trafo[15] = 1;
      
      try {
          offset = ((Double)configuration.get("offset")).doubleValue();
        } catch (Exception e) {
          // than we have the default value
        }
    }

    public void dispose() {
        // TODO Auto-generated method stub

    }

    public String getName() {
        return "GameTrackTranslation";
    }

    public String toString() {
        return "Virtual Device: "+getName();
    }
}
