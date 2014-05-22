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


package de.jreality.swing;

import java.awt.AWTEvent;
import java.awt.Component;
import java.awt.EventQueue;
import java.awt.Point;
import java.awt.event.MouseEvent;

import de.jreality.scene.Geometry;
import de.jreality.scene.pick.PickResult;
import de.jreality.scene.tool.AbstractTool;
import de.jreality.scene.tool.InputSlot;
import de.jreality.scene.tool.ToolContext;

/**
 * A tool to use with the JRJComponent. It transforms tool events 
 * into Java awt MouseEvents and sends them to the component given a
 * creation time for dispatching. The mouse coordinates are derived form 
 * the texture coordinates of the geometry this tool is attached to.
 * 
 * TODO: add support for different mouse buttons and keyboard events.
 * TODO: add support for texture transformation
 * 
 * @version 1.0
 * @author timh
 *
 */
class MouseEventTool extends AbstractTool {
  private Point oldPoint;
  private Component comp;
  
  private Geometry current;
  
  private static InputSlot drag0 = InputSlot.getDevice("PanelAction");
  private static InputSlot drag2 = InputSlot.getDevice("PanelSelection");
  private static InputSlot drag1 = InputSlot.getDevice("PanelMenu");
  
  private int width;
  private int height;
  
  private boolean dispatchLater = true;
  long lastActivationTime;
  
  public MouseEventTool(Component c, boolean dispatchLater) {
    super(drag0, drag1, drag2);
    this.comp = c;
    this.dispatchLater = dispatchLater;
    addCurrentSlot(InputSlot.getDevice("PointerTransformation"), "moves the mouse pointer");
  }
  
  public MouseEventTool(Component c) {
    this(c,true);
  }
  
  int currentButton=0;
  boolean doubleClick;
  int doubleClickDelay=400;
  
  public void activate(ToolContext e) {
    PickResult currentPick = e.getCurrentPick();
	try {
      current = (Geometry) currentPick.getPickPath().getLastElement();
    } catch (Exception ex) {
      // TODO:
    }
    int lastButton=currentButton;
    if (e.getSource() == drag0) currentButton=0;
    if (e.getSource() == drag1) currentButton=1;
    if (e.getSource() == drag2) currentButton=2;
  	long t = System.currentTimeMillis();
  	if (lastButton == currentButton && t-lastActivationTime<=doubleClickDelay) {
  		doubleClick=true;
  		lastActivationTime=0;
    } else {
    	lastActivationTime = t;
    	doubleClick=false;
    }
    Point newPoint = generatePoint(currentPick);
    oldPoint = newPoint;
    dispatchMouseEvent(newPoint, MouseEvent.MOUSE_PRESSED, currentButton);
  }

  public void perform(ToolContext e) {
    try {
      PickResult currentPick = e.getCurrentPick();
	if (currentPick != null && current == (Geometry) currentPick.getPickPath().getLastElement()) {
        Point newPoint = generatePoint(currentPick);
        dispatchMouseEvent(newPoint, MouseEvent.MOUSE_DRAGGED, currentButton);
      }
    } catch (Exception ex) {
      // TODO:
    }
  }

  public void deactivate(ToolContext e) {
    // TODO: maybe adapt click count to "real" AWT behavior - also in perform()
    PickResult currentPick = e.getCurrentPick();
	boolean sameGeom = (currentPick != null && current == (Geometry) currentPick.getPickPath().getLastElement());
    Point newPoint = sameGeom ? generatePoint(currentPick) : null;
    dispatchMouseEvent(sameGeom ? newPoint : oldPoint, MouseEvent.MOUSE_RELEASED, currentButton);
    if(oldPoint.equals(newPoint)) {
      dispatchMouseEvent(newPoint, MouseEvent.MOUSE_CLICKED, currentButton);
    }
    current = null;
  }

  private Point generatePoint(PickResult pr) {
    Point newPoint = null;
    if(pr != null) {
      double tc[] = pr.getTextureCoordinates();
      // if there are no texture coordinates use x, y in object space:
      if(tc== null || tc.length<2) {
        tc =  pr.getObjectCoordinates();
        newPoint = new Point((int) ((1 - tc[0]) * width), (int) ((1 - tc[1]) * height));
      } else {
        newPoint = new Point((int) ((tc[0]) * width), (int) ((tc[1]) * height));
      }
    } else {// TODO better to remember the last Point and use that?
      newPoint = new Point(0,0);
    }
    return newPoint;
  }    
  
  /**
   * 
   * @param newPoint
   * @param type
   * @param button 0, 1 or 2
   */
  void dispatchMouseEvent(Point newPoint, int type, int button) {
//	  int xAbs = newPoint.x+comp.getLocation().x;
//	  int yAbs = newPoint.y+comp.getLocation().y;
    final MouseEvent newEvent = new MouseEvent(comp,
        (int) type, System.currentTimeMillis(), /*InputEvent.BUTTON1_DOWN_MASK*/ 1 << (10+button), newPoint.x,
        newPoint.y, doubleClick ? 2 : 1, false, MouseEvent.BUTTON1+button); // TODO: is this what we want?
    dispatchEvent(newEvent);
  }

  public void setSize(int width, int height) {
    this.width = width;
    this.height = height;
  }
    
  /**
   * this method is safe as it executes in the DispatchThread.
   * @param e
   */
  public void dispatchEvent(final AWTEvent e) {
    if (dispatchLater) {
      EventQueue.invokeLater(new Runnable() {
        public void run() {
          comp.dispatchEvent(e);
        }
      });
    } else {
      comp.dispatchEvent(e);
    }
  }

}
