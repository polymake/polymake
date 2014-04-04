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

import java.util.HashMap;
import java.util.HashSet;

import org.eclipse.swt.events.KeyEvent;
import org.eclipse.swt.events.KeyListener;
import org.eclipse.swt.widgets.Canvas;

import de.jreality.jogl.SwtQueue;
import de.jreality.jogl.SwtViewer;
import de.jreality.scene.Viewer;
import de.jreality.scene.tool.AxisState;
import de.jreality.scene.tool.InputSlot;
import de.jreality.toolsystem.ToolEvent;
import de.jreality.toolsystem.ToolEventQueue;
import de.jreality.toolsystem.raw.RawDevice;
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
public class DeviceKeyboardSWT implements RawDevice, KeyListener {
  
    private HashMap keysToVirtual = new HashMap();
    
    private ToolEventQueue queue;
    private Canvas component;
    
    // maps InputDevices to Timers performing "keyReleased"
    private HashMap pendingReleases=new HashMap();

    public void initialize(Viewer viewer) {
      if (!(viewer instanceof SwtViewer)) throw new RuntimeException("only for SWT viewer!");
      setComponent(((SwtViewer)viewer).getGLCanvas());
    }

    public void setComponent(final Canvas component) {
      this.component = component;
      Runnable r = new Runnable() {
        public void run() {
          component.addKeyListener(DeviceKeyboardSWT.this);
        }
      };
      System.out.println("adding key listener");
      SwtQueue.getInstance().waitFor(r);
      System.out.println("done");
    }

    HashSet pressed = new HashSet();
    
    public synchronized void keyPressed(KeyEvent e) {
      InputSlot id = (InputSlot) keysToVirtual.get(new Integer(e.keyCode));
      // we assume that the released event is not older than 1 ms
      if (id != null) {
        if (!pressed.contains(id)) {
          ToolEvent ev = new ToolEvent(this, id, AxisState.PRESSED);
          queue.addEvent(ev);
          LoggingSystem.getLogger(this).fine(this.hashCode()+" added key pressed ["+id+"] "+e.time);
          pressed.add(id);
        }
      }
    }
    
    public synchronized void keyReleased(final KeyEvent e) {
        InputSlot id = (InputSlot) keysToVirtual.get(new Integer(e.keyCode));
        pressed.remove(id);
        queue.addEvent(new ToolEvent(this, id, AxisState.ORIGIN));
    }

    public ToolEvent mapRawDevice(String rawDeviceName, InputSlot inputDevice) {
        // rawDeviceName = keyCode
        keysToVirtual.put(new Integer(Integer.parseInt(rawDeviceName)), inputDevice);
        return new ToolEvent(this, inputDevice, AxisState.ORIGIN);
    }
    
    public void setEventQueue(ToolEventQueue queue) {
        this.queue = queue; 
    }

    public void dispose() {
      Runnable r = new Runnable() {
        public void run() {
          component.removeKeyListener(DeviceKeyboardSWT.this);
        }
      };
      component.getDisplay().syncExec(r);
    }
    
    public String getName() {
        return "Keyboard";
    }
    
    public String toString() {
      return "RawDevice: Keyboard";
    }

}
