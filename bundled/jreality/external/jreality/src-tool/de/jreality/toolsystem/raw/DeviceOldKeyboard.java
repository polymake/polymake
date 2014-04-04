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

import java.awt.Component;
import java.awt.EventQueue;
import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.logging.Level;

import de.jreality.scene.Viewer;
import de.jreality.scene.tool.AxisState;
import de.jreality.scene.tool.InputSlot;
import de.jreality.toolsystem.ToolEvent;
import de.jreality.toolsystem.ToolEventQueue;
import de.jreality.util.LoggingSystem;

/**
 * This class contains an ugly workaround for linux keyboard auto-repeat.
 * 
 * When a key released event arrives, it is noted and rescheduled,
 * whith a short sleep - so that there is time for the corresponding keyTyped
 * event to check in.
 * 
 * in the keyTyped method we mark a matching release event so that it is not executed.
 * so neither the keyPressed nor the keyReleased are processed.
 * 
 * This works for me much better than the previous version - anyway,
 * I guess one needs to tweak the sleep value depending on the machine...
 * 
 * TODO: use configuration attributes to configure raw devices if needed.
 * 
 * @author weissman
 *
 **/
public class DeviceOldKeyboard implements RawDevice, KeyListener {
  
    private HashMap keysToVirtual = new HashMap();
    
    private ToolEventQueue queue;
    private Component component;
    
    public void initialize(Viewer viewer, Map<String, Object> config) {
      if (!viewer.hasViewingComponent() || !(viewer.getViewingComponent() instanceof Component) ) throw new UnsupportedOperationException("need AWT component");
      this.component = (Component) viewer.getViewingComponent();
      component.addKeyListener(this);
    }

    // store last release events
    HashMap lastReleased = new HashMap();
    HashSet cancelEvents = new HashSet();
    HashSet seenReleases = new HashSet();
    
    public synchronized void keyPressed(KeyEvent e) {
        InputSlot id = (InputSlot) keysToVirtual.get(new Integer(e.getKeyCode()));
        // we assume that the released event is not older than 1 ms
        if (id != null) {
            Long timestamp = new Long(e.getWhen());
            if (((HashMap)lastReleased.get(id)).containsKey(timestamp)) {
                KeyEvent releaseEvent = (KeyEvent) ((HashMap)lastReleased.get(id)).get(timestamp);
                cancelEvents.add(releaseEvent);
                return;
            } 
            timestamp = new Long(e.getWhen()-1);
            if (((HashMap)lastReleased.get(id)).containsKey(timestamp)) {
                KeyEvent releaseEvent = (KeyEvent) ((HashMap)lastReleased.get(id)).get(timestamp);
                cancelEvents.add(releaseEvent);
                return;
            } 
            ToolEvent ev = new ToolEvent(this, System.currentTimeMillis(), id, AxisState.PRESSED);
            queue.addEvent(ev);
            LoggingSystem.getLogger(this).fine(this.hashCode()+" added key pressed ["+id+"] "+e.getWhen());
        }
    }
    
    public synchronized void keyReleased(final KeyEvent e) {
        InputSlot id = (InputSlot) keysToVirtual.get(new Integer(e.getKeyCode()));
        if (id != null) {
            if (!seenReleases.contains(e)) {
                LoggingSystem.getLogger(this).log(Level.FINEST, "release first");
                seenReleases.add(e);
                ((HashMap)lastReleased.get(id)).put(new Long(e.getWhen()), e);
                try {
                    Thread.sleep(1);
                } catch (Exception e1) {
                    // TODO Auto-generated catch block
                    e1.printStackTrace();
                }
                EventQueue.invokeLater(new Runnable() {
                    public void run() {
                        DeviceOldKeyboard.this.keyReleased(e);
                    }
                });
            } else {
                LoggingSystem.getLogger(this).log(Level.FINEST, "release second");
                if (cancelEvents.contains(e)) cancelEvents.remove(id);
                else {
                    queue.addEvent(new ToolEvent(this, System.currentTimeMillis(), id, AxisState.ORIGIN));
                    LoggingSystem.getLogger(this).finer("added key released ["+id+"] "+e.getWhen());
                }
                ((HashMap)lastReleased.get(id)).remove(new Long(e.getWhen()));
            }
        }
    }

    public ToolEvent mapRawDevice(String rawDeviceName, InputSlot inputDevice) {
        // rawDeviceName = VK_W (e.g.)
        keysToVirtual.put(resolveKeyCode(rawDeviceName), inputDevice);
        lastReleased.put(inputDevice, new HashMap());
        return new ToolEvent(this, System.currentTimeMillis(), inputDevice, AxisState.ORIGIN);
    }
    
    private Integer resolveKeyCode(String fieldName) {
      try {
        int val = KeyEvent.class.getField(fieldName).getInt(KeyEvent.class);
        return new Integer(val);
      } catch (Exception e) {
        throw new IllegalArgumentException("no such key "+fieldName);
      }
      
    }

    public void setEventQueue(ToolEventQueue queue) {
        this.queue = queue; 
    }

    public void dispose() {
        component.removeKeyListener(this);   
    }
    
    public String getName() {
        return "Keyboard";
    }
    
    public String toString() {
      return "RawDevice: Keyboard";
    }

    public void keyTyped(KeyEvent e) {
        // not used
    }
    
}
