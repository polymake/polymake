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


package de.jreality.ui.viewerapp;

import java.awt.BorderLayout;
import java.awt.Component;
import java.awt.Dimension;
import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.awt.event.MouseMotionListener;
import java.awt.event.MouseWheelEvent;
import java.awt.event.MouseWheelListener;
import java.beans.Statement;

import javax.swing.JPanel;

import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.Viewer;
import de.jreality.toolsystem.ToolSystem;
import de.jreality.util.LoggingSystem;

/**
 * hides several viewer implementations and provides the same
 * viewing component for all. Using this class an application
 * doesn't need to care about component replacement when switching
 * from one viewer to another...
 * 
 * @author Steffen Weissmann
 *
 */
public class ViewerSwitch implements Viewer {
  
  private Viewer[] viewers;
  private String[] viewerNames;
  
  private Viewer currentViewer;
  private EventDispatcher eventDispatcher=new EventDispatcher();
  
  private JPanel component = new JPanel(new BorderLayout());

  private static String[] createViewerNames(Viewer[] viewers) {
    String[] viewerNames = new String[viewers.length];
    for (int i = 0; i < viewerNames.length; i++)
      viewerNames[i] = "["+i+"] "+viewers[i].getClass().getName();
    return viewerNames;
  }
  
  public ViewerSwitch(Viewer[] viewers) {
    this(viewers, createViewerNames(viewers));
  }
  
  public ViewerSwitch(Viewer[] viewers, String[] names) {
    this.viewers = viewers;
    this.viewerNames = names;
    currentViewer = viewers[0];
    if (currentViewer.hasViewingComponent() && currentViewer.getViewingComponent() instanceof Component) {
      registerComponent((Component) currentViewer.getViewingComponent());
    }
  }

  public Viewer getCurrentViewer() {
    return currentViewer;
  }
  
  private void registerComponent(Component c) {
    c.addKeyListener(eventDispatcher);
    c.addMouseListener(eventDispatcher);
    c.addMouseMotionListener(eventDispatcher);
    c.addMouseWheelListener(eventDispatcher);
    component.add("Center", c);
    component.revalidate();
  }
    
  private void unregisterComponent(Component c) {
    c.removeKeyListener(eventDispatcher);
    c.removeMouseListener(eventDispatcher);
    c.removeMouseMotionListener(eventDispatcher);
    c.removeMouseWheelListener(eventDispatcher);
    component.removeAll();
  }
  
  public void selectViewer(int i) {
    if (viewers[i] == currentViewer) return;
    Viewer newViewer = viewers[i];
    newViewer.setSceneRoot(currentViewer.getSceneRoot());
    newViewer.setCameraPath(currentViewer.getCameraPath());
    try {
      newViewer.setAuxiliaryRoot(currentViewer.getAuxiliaryRoot());
 //     newViewer.setMetric(currentViewer.getMetric());
    } catch (Exception e) {}
    
    if (currentViewer.hasViewingComponent()  && currentViewer.getViewingComponent() instanceof Component)
    	unregisterComponent((Component) currentViewer.getViewingComponent());
    
    if (newViewer.hasViewingComponent()  && newViewer.getViewingComponent() instanceof Component) {
      if (currentViewer.hasViewingComponent()  && currentViewer.getViewingComponent() instanceof Component)
        ((Component) newViewer.getViewingComponent()).setSize(currentViewer.getViewingComponentSize());
      registerComponent((Component) newViewer.getViewingComponent());
    }
    currentViewer = newViewer;
  }
  
  public int getNumViewers() {
    return viewers.length;
  }
  
  public Viewer[] getViewers() {
  	return (Viewer[]) viewers.clone();
  }
  
  public String[] getViewerNames() {
    return (String[]) viewerNames.clone();
  }
  
  public void selectViewer(String viewerName) {
    for (int i = 0; i < viewerNames.length; i++) {
      if (viewerNames[i].equals(viewerName)) {
        selectViewer(i);
        return;
      }
    }
    throw new IllegalArgumentException("no such viewer "+viewerName);
  }

  public SceneGraphComponent getSceneRoot() {
    return currentViewer.getSceneRoot();
  }

  public SceneGraphComponent getAuxiliaryRoot() {
    return currentViewer.getAuxiliaryRoot();
  }

  public SceneGraphPath getCameraPath() {
    return currentViewer.getCameraPath();
  }

//  public int getMetric() {
//    return currentViewer.getMetric();
//  }

  public Component getViewingComponent() {
    return component;
  }

  public boolean hasViewingComponent() {
    return true;
  }

  public void render() {
    currentViewer.render();
  }

  public void setAuxiliaryRoot(SceneGraphComponent ar) {
    currentViewer.setAuxiliaryRoot(ar);
  }

  public void setCameraPath(SceneGraphPath p) {
    currentViewer.setCameraPath(p);
  }

  public void setSceneRoot(SceneGraphComponent r) {
    currentViewer.setSceneRoot(r);
  }

//  public void setMetric(int sig) {
//    currentViewer.setMetric(sig);
//  }
  
  private class EventDispatcher implements KeyListener, MouseListener, MouseMotionListener, MouseWheelListener {
    
      public void keyPressed(KeyEvent e) {
        component.dispatchEvent(e);
      }
      public void keyReleased(KeyEvent e) {
        component.dispatchEvent(e);
      }
      public void keyTyped(KeyEvent e) {
        component.dispatchEvent(e);
      }
      public void mouseClicked(MouseEvent e) {
        component.dispatchEvent(e);
      }
      public void mouseEntered(MouseEvent e) {
        component.dispatchEvent(e);
      }
      public void mouseExited(MouseEvent e) {
        component.dispatchEvent(e);
      }
      public void mousePressed(MouseEvent e) {
        component.dispatchEvent(e);
      }
      public void mouseReleased(MouseEvent e) {
        component.dispatchEvent(e);
      }
      public void mouseDragged(MouseEvent e) {
        component.dispatchEvent(e);
      }
      public void mouseMoved(MouseEvent e) {
        component.dispatchEvent(e);
      }
      public void mouseWheelMoved(MouseWheelEvent e) {
        component.dispatchEvent(e);
      }
  }

  public boolean canRenderAsync() {
    return currentViewer.canRenderAsync();
  }

  public Dimension getViewingComponentSize() {
    return currentViewer.getViewingComponentSize();
  }

  public void renderAsync() throws UnsupportedOperationException {
    currentViewer.renderAsync();
  }
  
  public void dispose() {
	  System.out.println("calling dispose method");
    if (currentViewer.hasViewingComponent()  && currentViewer.getViewingComponent() instanceof Component) unregisterComponent((Component) currentViewer.getViewingComponent());
    for (int i = 0; i < viewers.length; i++) {
      Statement stm = new Statement(viewers[i], "dispose", null);
      try {
    	  System.out.println("calling viewers.dispose");
        stm.execute();
      } catch (Exception e) {
    	  System.out.println("exception when trying to dispose viewer "+this.viewerNames[i]);
        //e.printStackTrace();
      }
    }
  }

	/**
	 * Set all the viewers and the ViewerSwitch to have the same ToolSystem
	 * Would it be better to look up the tool system via the scene root, rather than per viewer?
	 * @param toolSystem
	 */
  public void setToolSystem(ToolSystem toolSystem) {
		if (ToolSystem.getToolSystemForViewer(this) != toolSystem)
			ToolSystem.setToolSystemForViewer(this,toolSystem);
		for (Viewer v : viewers)	{
			if (ToolSystem.getToolSystemForViewer(v) != toolSystem)
				ToolSystem.setToolSystemForViewer(v,toolSystem);
			LoggingSystem.getLogger(this).fine("Tool system for "+v+" is "+ToolSystem.getToolSystemForViewer(v));
		}
	}

}