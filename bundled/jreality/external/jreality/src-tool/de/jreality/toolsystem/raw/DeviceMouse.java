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
import java.awt.Container;
import java.awt.Cursor;
import java.awt.Insets;
import java.awt.KeyboardFocusManager;
import java.awt.Point;
import java.awt.event.ComponentAdapter;
import java.awt.event.ComponentEvent;
import java.awt.event.ComponentListener;
import java.awt.event.InputEvent;
import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.awt.event.MouseMotionListener;
import java.awt.event.MouseWheelEvent;
import java.awt.event.MouseWheelListener;
import java.util.Map;
import java.util.logging.Level;

import javax.swing.ImageIcon;

import de.jreality.math.Matrix;
import de.jreality.scene.Viewer;
import de.jreality.scene.data.DoubleArray;
import de.jreality.scene.tool.AxisState;
import de.jreality.scene.tool.InputSlot;
import de.jreality.toolsystem.ToolEvent;
import de.jreality.util.LoggingSystem;

/**
 * @author weissman
 *
 **/
public class DeviceMouse extends AbstractDeviceMouse implements RawDevice, MouseListener,
    MouseMotionListener, MouseWheelListener {

	  private static int MOUSE_GRAB_TOGGLE = KeyEvent.VK_F10;
	  private static int MOUSE_GRAB_TOGGLE_ALTERNATIVE = KeyEvent.VK_C;
  
  private Component component;
  
  public void mouseClicked(MouseEvent e) {
  }

  public void mouseEntered(MouseEvent e) {
  }

  public void mouseExited(MouseEvent e) {
  }

  public void mousePressed(MouseEvent e) {
    InputSlot button = findButton(e);
    if (button != null)
        queue.addEvent(new ToolEvent(DeviceMouse.this, System.currentTimeMillis(), button, AxisState.PRESSED));
  }

  public void mouseReleased(MouseEvent e) {
    InputSlot button = findButton(e);
    if (button != null)
        queue.addEvent(new ToolEvent(DeviceMouse.this, System.currentTimeMillis(), button, AxisState.ORIGIN));
  }

  public void mouseDragged(MouseEvent e) {
    mouseMoved(e);
  }

  Cursor emptyCursor;
  public void mouseMoved(MouseEvent e) {
    mouseMoved(e.getX(), e.getY());
  }
  public void mouseWheelMoved(MouseWheelEvent e) {
    int count = e.getWheelRotation();
    if (count > 0) {
      InputSlot slot = (InputSlot) usedSources.get("wheel_up");
      if (slot == null) return;
      for (int i = 0; i < count; i++) {
        queue.addEvent(new ToolEvent(DeviceMouse.this, System.currentTimeMillis(), slot, AxisState.PRESSED));
        queue.addEvent(new ToolEvent(DeviceMouse.this, System.currentTimeMillis(), slot, AxisState.ORIGIN));
      }
    }
    if (count < 0) {
      InputSlot slot = (InputSlot) usedSources.get("wheel_down");
      if (slot == null) return;
      for (int i = 0; i > count; i--) {
        queue.addEvent(new ToolEvent(DeviceMouse.this, System.currentTimeMillis(), slot, AxisState.PRESSED));
        queue.addEvent(new ToolEvent(DeviceMouse.this, System.currentTimeMillis(), slot, AxisState.ORIGIN));
      }
    }
  }

  // e.getButton() doesn't work properly on 1-button mouse, such as MacOS laptops
  public static int getRealButton(MouseEvent e) {
    int button = e.getButton();
    if (button == 0)  {   // Linux!
      int mods = e.getModifiersEx();
      if ((mods & InputEvent.BUTTON1_DOWN_MASK) != 0)   button = 1;
      else if ((mods & InputEvent.BUTTON2_DOWN_MASK) != 0)  button = 2;
      else button = 3;
    } else {          // Mac OS X Laptop (no 3-mouse button)!!
      int mods = e.getModifiers();
      if (e.isAltDown() && ((mods & InputEvent.BUTTON2_MASK) != 0) ) button = 2;
      else if (button == 1 &&  ((mods & InputEvent.BUTTON3_MASK) != 0) ) button = 3;
    }
    return button;
  }

  private InputSlot findButton(MouseEvent e) {
    int button = getRealButton(e); //e.getButton();
    if (button == MouseEvent.BUTTON1)
        return (InputSlot) usedSources.get("left");
    if (button == MouseEvent.BUTTON3)
        return (InputSlot) usedSources.get("right");
    if (button == MouseEvent.BUTTON2)
        return (InputSlot) usedSources.get("center");
    return null;
  }

  private ComponentListener componentListener = new ComponentAdapter() {
	  public void componentResized(ComponentEvent e) {
		  if (isCenter()) {
			  setCenter(false);
			  queue.addEvent(new ToolEvent(this, System.currentTimeMillis(), InputSlot.getDevice("LookSwitch"), AxisState.PRESSED, null));
			  queue.addEvent(new ToolEvent(this, System.currentTimeMillis(), InputSlot.getDevice("LookSwitch"), AxisState.ORIGIN, null));
		  }
		  requestFocus();
	  }
  };
  
  private KeyListener keyListener = new KeyListener() {
      public void keyTyped(KeyEvent e) {
      }
      public void keyPressed(KeyEvent e) {
        if (e.getKeyCode() == MOUSE_GRAB_TOGGLE ||
        		(e.getKeyCode() == MOUSE_GRAB_TOGGLE_ALTERNATIVE
        	&& e.isShiftDown() && e.isControlDown())
        		) {
          setCenter(!isCenter());
          // XXX: HACK
          queue.addEvent(new ToolEvent(this, System.currentTimeMillis(), InputSlot.getDevice("LookSwitch"), AxisState.PRESSED, null));
          queue.addEvent(new ToolEvent(this, System.currentTimeMillis(), InputSlot.getDevice("LookSwitch"), AxisState.ORIGIN, null));
        }
      }
      public void keyReleased(KeyEvent e) {
      }
    };
    
  public void setComponent(final Component component) {
    this.component = component;
    component.addMouseListener(this);
    component.addMouseMotionListener(this);
    component.addMouseWheelListener(this);
    component.addComponentListener(componentListener);
    component.addKeyListener(keyListener);
  }

  public ToolEvent mapRawDevice(String rawDeviceName, InputSlot inputDevice) {
    if (!knownSources.contains(rawDeviceName))
        throw new IllegalArgumentException("no such raw device");
    usedSources.put(rawDeviceName, inputDevice);
    if (rawDeviceName.equals("axes")) return new ToolEvent(this, System.currentTimeMillis(), inputDevice, new DoubleArray(new Matrix().getArray()));
    if (rawDeviceName.equals("axesEvolution")) {
      Matrix initM = new Matrix();
      initM.setEntry(2, 3, -1);
      return new ToolEvent(this, System.currentTimeMillis(), inputDevice, new DoubleArray(initM.getArray()));
    }
    return new ToolEvent(this, System.currentTimeMillis(), inputDevice, AxisState.ORIGIN);
  }

  public void dispose() {
    component.removeMouseListener(this);
    component.removeMouseMotionListener(this);
    component.removeMouseWheelListener(this);
    component.removeComponentListener(componentListener);
    component.removeKeyListener(keyListener);
  }

  public void initialize(Viewer viewer, Map<String, Object> config) {
    if (!viewer.hasViewingComponent() || !(viewer.getViewingComponent() instanceof Component) ) throw new UnsupportedOperationException("need AWT component");
    setComponent((Component) viewer.getViewingComponent());
  }

  public String getName() {
    return "Mouse";
  }

  public String toString() {
    return "RawDevice: Mouse AWT";
  }

  public void installGrabs() {
    try {
      if (emptyCursor == null) {
        ImageIcon emptyIcon = new ImageIcon(new byte[0]);
        emptyCursor = component.getToolkit().createCustomCursor(emptyIcon.getImage(), new Point(0, 0), "emptyCursor");
      }
      component.setCursor(emptyCursor);
      requestFocus();
    } catch (Exception e) {
      LoggingSystem.getLogger(this).log(Level.WARNING, "cannot grab mouse", e);
    }
  }

  public void uninstallGrabs() {
    try {
      component.setCursor(Cursor.getPredefinedCursor(Cursor.DEFAULT_CURSOR));
      requestFocus();
    } catch (Exception e) {
      LoggingSystem.getLogger(this).log(Level.WARNING, "cannot grab mouse", e);
    }
  }

  // XXX: howto do that?
  private void requestFocus() {
  	KeyboardFocusManager.getCurrentKeyboardFocusManager().clearGlobalFocusOwner();
  	boolean fow = component.requestFocusInWindow();
  	//component.requestFocus();
	}

	protected int getWidth() {
    return component.getWidth();
  }

  protected int getHeight() {
    return component.getHeight();
  }

  protected void calculateCenter() {
    Component currentCmp = component;
    winCenterX=getWidth() / 2;
    winCenterY=getHeight() / 2;
    while (currentCmp != null) {
      if (currentCmp instanceof Container) {
        Insets insets = ((Container)currentCmp).getInsets();
        winCenterX += insets.left;
        winCenterY += insets.top;
      }
      winCenterX += currentCmp.getX();
      winCenterY += currentCmp.getY();
      currentCmp = currentCmp.getParent();
    }
  }


}
