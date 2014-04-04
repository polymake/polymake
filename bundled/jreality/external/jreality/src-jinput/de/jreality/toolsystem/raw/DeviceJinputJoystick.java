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


package de.jreality.toolsystem.raw;

import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;

import net.java.games.input.Component;
import net.java.games.input.Controller;
import net.java.games.input.ControllerEnvironment;
import de.jreality.scene.Viewer;
import de.jreality.scene.tool.AxisState;
import de.jreality.scene.tool.InputSlot;
import de.jreality.toolsystem.ToolEvent;
import de.jreality.toolsystem.ToolEventQueue;

/**
 * A device that utilizes the jinput library for polling mice and joysticks.
 * The k-th axis of the l-th device can be addressed via "axis_l_k"
 * 
 * TODO: rename this to DeviceJInput, since it provides access to ALL jinput devices
 * @author weissman/hoffmann
 *
 **/
public class DeviceJinputJoystick implements RawDevice, PollingDevice {

    private ToolEventQueue queue;
    
	private HashMap<Component,InputSlot> componentMap = new HashMap<Component,InputSlot>();
	private HashMap<Component,AxisState> lastValues = new HashMap<Component,AxisState>();
	private HashMap<Component,Float> maxValues = new HashMap<Component,Float>();
    
    private Controller controllers[];
	private net.java.games.input.Component[][] components;
	
	
	public DeviceJinputJoystick() {
		ControllerEnvironment env = ControllerEnvironment.getDefaultEnvironment();
		controllers = env.getControllers();
		for (Controller ctrl : controllers) {
			System.out.println(ctrl+" :: "+Arrays.toString(ctrl.getComponents()));
		}
		components = new net.java.games.input.Component[controllers.length][];
		for (int i = 0; i < controllers.length; i++) {
			components[i] = controllers[i].getComponents();
		}
	}
	
    public ToolEvent mapRawDevice(String rawDeviceName, InputSlot inputDevice) {
		String[] nums = rawDeviceName.split("_");
		if(nums.length < 3) throw new IllegalArgumentException("no such raw axis");
		try {
			int i = Integer.parseInt(nums[1]);
			int j = Integer.parseInt(nums[2]);
            net.java.games.input.Component c = components[i][j];
			componentMap.put(c,inputDevice);
            lastValues.put(c,AxisState.ORIGIN);
            float maxVal = 1;
            if (nums.length==4) try {
            	maxVal = Float.parseFloat(nums[3]);
            } catch (NumberFormatException nbfe) {
            	System.err.println("invalid max value: "+nums[3]);
            }
            maxValues.put(c, maxVal);
			return new ToolEvent(this, inputDevice, AxisState.ORIGIN);
		} catch (Exception e) {
			throw new IllegalArgumentException("no such raw axis");
		}
    }

    public void setEventQueue(ToolEventQueue queue) {
        this.queue = queue;
    }

    public void dispose() {
    }

    public void initialize(Viewer viewer, Map<String, Object> config) {
    }

    public String getName() {
        return "jinputJoystick";
    }
    
	public void poll(long when) {
		if (queue == null) return;
		for (int i = 0; i < controllers.length; i++) {
            controllers[i].poll();
        }
		//Set keys = componentMap.keySet();
		 Set<Map.Entry<Component, InputSlot>> entries = componentMap.entrySet();
		//for (Iterator iter = entries.iterator(); iter.hasNext();) {
         for (Map.Entry<Component, InputSlot> element : entries) {
            
            Component c = element.getKey();
			//InputSlot inputDevice = (InputSlot) element.getValue();
            float val = c.getPollData()/maxValues.get(c);
            AxisState newState = new AxisState(val);
			AxisState oldState = lastValues.get(c);
			if(!newState.isReleased() || newState.intValue() != oldState.intValue()) {
				//System.out.println("new event");
				queue.addEvent(
						new ToolEvent(this, when, element.getValue(), newState)
						);
                lastValues.put(c,newState);
			}
		}
	}

}
