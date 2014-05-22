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

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.MouseMoveListener;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Listener;

import de.jreality.jogl.SwtQueue;
import de.jreality.jogl.SwtViewer;
import de.jreality.math.Matrix;
import de.jreality.scene.Viewer;
import de.jreality.scene.data.DoubleArray;
import de.jreality.scene.tool.AxisState;
import de.jreality.scene.tool.InputSlot;
import de.jreality.toolsystem.ToolEvent;
import de.jreality.toolsystem.ToolEventQueue;
import de.jreality.toolsystem.raw.AbstractDeviceMouse;
import de.jreality.toolsystem.raw.RawDevice;

/**
 * @author weissman
 *
 **/
public class DeviceMouseSWT extends AbstractDeviceMouse implements RawDevice, MouseListener, MouseMoveListener, Listener {

  private Canvas component;

  public void mouseDown(MouseEvent e) {
    InputSlot button = findButton(e);
    ToolEvent toolEvent = new ToolEvent(DeviceMouseSWT.this, button,
            AxisState.PRESSED);
    if (button != null) {
      queue.addEvent(toolEvent);
    }
  }

  public void mouseUp(MouseEvent e) {
    InputSlot button = findButton(e);
    if (button != null)
        queue.addEvent(new ToolEvent(DeviceMouseSWT.this, button, AxisState.ORIGIN));
  }

  public void mouseMove(MouseEvent e) {
    mouseMoved(e.x, e.y);
  }

//  public void mouseWheelMoved(MouseWheelEvent e) {
//    int count = e.getWheelRotation();
//    if (count > 0) {
//      InputSlot slot = (InputSlot) usedSources.get("wheel_up");
//      if (slot == null) return;
//      for (int i = 0; i < count; i++) {
//        queue.addEvent(new ToolEvent(DeviceMouseSWT.this, slot, AxisState.PRESSED));
//        queue.addEvent(new ToolEvent(DeviceMouseSWT.this, slot, AxisState.ORIGIN));
//      }
//    }
//    if (count < 0) {
//      InputSlot slot = (InputSlot) usedSources.get("wheel_down");
//      if (slot == null) return;
//      for (int i = 0; i > count; i--) {
//        queue
//            .addEvent(new ToolEvent(DeviceMouseSWT.this, slot, AxisState.PRESSED));
//        queue.addEvent(new ToolEvent(DeviceMouseSWT.this, slot, AxisState.ORIGIN));
//      }
//    }
//  }

  private InputSlot findButton(MouseEvent e) {
    if (e.button == 1)
        return (InputSlot) usedSources.get("left");
    if (e.button == 3)
        return (InputSlot) usedSources.get("right");
    if (e.button == 2)
        return (InputSlot) usedSources.get("center");
    return null;
  }

  public void setComponent(final Canvas component) {
    this.component = component;
    Runnable r = new Runnable() {
      public void run() {
        component.addMouseListener(DeviceMouseSWT.this);
        component.addMouseMoveListener(DeviceMouseSWT.this);
        component.addListener(SWT.MouseWheel, DeviceMouseSWT.this);
      }
    };
    SwtQueue.getInstance().waitFor(r);
  }

  public ToolEvent mapRawDevice(String rawDeviceName, InputSlot inputDevice) {
    if (!knownSources.contains(rawDeviceName))
        throw new IllegalArgumentException("no such raw device");
    usedSources.put(rawDeviceName, inputDevice);
    if (rawDeviceName.equals("axes")) return new ToolEvent(this, inputDevice, new DoubleArray(new Matrix().getArray()));
    return new ToolEvent(this, inputDevice, AxisState.ORIGIN);
  }

  public void setEventQueue(ToolEventQueue queue) {
    this.queue = queue;
  }

  public void dispose() {
    Runnable r = new Runnable() {
      public void run() {
        component.removeMouseListener(DeviceMouseSWT.this);
        component.removeMouseMoveListener(DeviceMouseSWT.this);
        component.removeListener(SWT.MouseWheel, DeviceMouseSWT.this);
      }
    };
    component.getDisplay().syncExec(r);
  }

  public void initialize(Viewer viewer) {
    if (!(viewer instanceof SwtViewer)) throw new RuntimeException("only for SWT viewer!");
    setComponent(((SwtViewer)viewer).getGLCanvas());
  }

  public String getName() {
    return "Mouse";
  }

  public String toString() {
    return "RawDevice: Mouse SWT";
  }

  public void mouseDoubleClick(MouseEvent arg0) {
  }

  public void handleEvent(Event arg0) {
    System.out.println(arg0);
  }

  protected void uninstallGrabs() {
    // TODO Auto-generated method stub
    
  }

  protected void installGrabs() {
    // TODO Auto-generated method stub
    
  }

  protected int getWidth() {
    return component.getSize().x;
  }

  protected int getHeight() {
    return component.getSize().y;
  }

  protected void calculateCenter() {
    throw new UnsupportedOperationException("Not yet implemented");
  }
  
}
