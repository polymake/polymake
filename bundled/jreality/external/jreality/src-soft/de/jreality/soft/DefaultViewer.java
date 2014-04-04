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


package de.jreality.soft;

import java.awt.AWTEvent;
import java.awt.Color;
import java.awt.Component;
import java.awt.Dimension;
import java.awt.EventQueue;
import java.awt.Graphics;
import java.awt.Image;
import java.awt.Rectangle;
import java.awt.event.MouseEvent;
import java.awt.image.BufferedImage;
import java.util.logging.Level;

import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.Viewer;
import de.jreality.util.LoggingSystem;

/**
 * The default software renderer component.
 * @version 1.0
 * @author <a href="mailto:hoffmann@math.tu-berlin.de">Tim Hoffmann</a>
 * @author Holger Pietsch
 */
public class DefaultViewer extends Component implements Runnable, Viewer {

  // TODO: remove ENFORCE_... ?

  private static final boolean ENFORCE_PAINT_ON_MOUSEEVENTS= false;
  
  // synchronizes the render thread
  private final Object renderLock= new Object();
  //private Camera camera;
  private SceneGraphPath cameraPath;
  private SceneGraphComponent root;

  private transient BufferedImage offscreen;

  private Renderer renderer;

  private boolean upToDate= false;
  private boolean backgroundExplicitlySet;
  private boolean imageValid;
  private boolean useDouble;
  private boolean useFloat;
  private Image bgImage;
  
  public DefaultViewer(int type) {
      super();
      switch (type) {
    case 0:
        useDouble = false;
        useFloat = false;
        break;
    case 1:
        useFloat = true;
        useDouble = false;        
        break;
    case 2:
        useFloat = false;
        useDouble = true;        
        break;
        
    default:
        break;
    }
    //backgroundExplicitlySet=getBackground()!=null;
    setBackground(Color.white);
    if(ENFORCE_PAINT_ON_MOUSEEVENTS)
      enableEvents(AWTEvent.MOUSE_MOTION_EVENT_MASK);
    Thread renderThread = new Thread(this, "jReality render thread");
    //renderThread.setPriority(Thread.NORM_PRIORITY+1);
    renderThread.start();
  }
  public DefaultViewer() {
      this(0);
  }
  public DefaultViewer(boolean useDouble) {
    this(useDouble?2:0);
  }

  public boolean isFocusable() {
    return true;
  }
  
  //TODO should we claim to be opaque?
//  public boolean isOpaque() {
//    return true;
//}
/* (non-Javadoc)
   * @see de.jreality.soft.Viewer#getViewingComponent()
   */
  public Object getViewingComponent() {
    return this;
  }

  /* (non-Javadoc)
   * @see de.jreality.soft.Viewer#setSceneRoot(de.jreality.scene.SceneGraphComponent)
   */
  public void setSceneRoot(SceneGraphComponent c) {
    root=c;
    if(renderer!=null) renderer.setSceneRoot(c);
  }

  /* (non-Javadoc)
   * @see de.jreality.soft.Viewer#getSceneRoot()
   */
  public SceneGraphComponent getSceneRoot() {
    return root;
  }

  // notification when rendering has finished
  private final Object renderFinishLock=new Object();
  
  //synchronize render calls
  private final Object renderSynch=new Object();
  
  private boolean synchRendering;
  
  public void render() {
    synchronized (renderLock) {
      if (EventQueue.isDispatchThread() && synchRendering) {
        // avoid deadlock
        return;
      }
    }
    synchronized (renderSynch) { // block until finished
      synchronized (renderFinishLock) { // wait until finished
        synchronized(renderLock) { // synchronize with renderAsync() call
          synchRendering=true;
          if (upToDate) {
            upToDate=false;
            renderLock.notify();
          } else {
            // else someone else triggered a render
            // which did not start yet - ok
          }
        }
        while (synchRendering) {
          try {
            renderFinishLock.wait();
          } catch (InterruptedException e) {
            throw new Error();
          }
        }
        if (EventQueue.isDispatchThread()) run();
        else {
          try {
            EventQueue.invokeAndWait(this);
          } catch (Exception e) {
            e.printStackTrace();
          }
        }
      }
    }
  }

  public void invalidate() {
    super.invalidate();
    imageValid = false;
    upToDate = false;
  }

  public boolean isDoubleBuffered() {
    return true;
  }

  public void paint(Graphics g) {
  	if(!isShowing()) return;
  	Rectangle clip = g.getClipBounds();
    if(clip!=null && clip.isEmpty()) return;
    synchronized(this) {
      if(imageValid) {
        if(offscreen != null) {
            if(bgImage != null)
                g.drawImage(bgImage, 0, 0,Color.GREEN, null);
          g.drawImage(offscreen, 0, 0, null);
          return;
        } else System.err.println("paint: no offscreen in paint");
      } else if(!upToDate) synchronized(renderLock) {
        renderLock.notify();
      }
    }
  }

  public void update(Graphics g) {
    paint(g);
  }

  private boolean disposed;
  
  public void run() {
    if(EventQueue.isDispatchThread()) {
      paintImmediately();
    }
    else while (true) try {
      synchronized(renderLock) {
        while (upToDate) {
          try {
            renderLock.wait();
          } catch(InterruptedException e) {
            e.printStackTrace();
          }
          if (disposed) return;
        }
        upToDate=true;
      }
      renderImpl();
      if (!synchRendering) {
        if(imageValid) EventQueue.invokeLater(this);
      } else {
        synchronized (renderLock) {
          synchRendering=false;
          synchronized (renderFinishLock) {
            renderFinishLock.notify();
          }
        }
      }
    } catch(Exception ex) {
      Thread t= Thread.currentThread();
      t.getThreadGroup().uncaughtException(t, ex);
    }
  }
  // TODO: add a structure lock to the scene graph
  public final synchronized void renderSync() {
    renderImpl();
  }

  private synchronized void renderImpl() {
    Dimension d= getSize();
    if (d.width > 0 && d.height > 0) {
      //System.out.println("render: off="+offscreen);
      if (offscreen == null
        || offscreen.getWidth() != d.width
        || offscreen.getHeight() != d.height) {
        imageValid = false;
        if(useDouble) {
//            offscreen=
//                    new BufferedImage(d.width, d.height, BufferedImage.TYPE_3BYTE_BGR);
//        renderer=new Renderer.ByteArray(offscreen);
//            renderer = new Renderer.ByteArrayDouble(offscreen);
            offscreen=
                new BufferedImage(d.width, d.height, BufferedImage.TYPE_INT_ARGB);
            renderer=new Renderer.IntArrayDouble(offscreen);
        
        } else if(useFloat) {
            offscreen=
                new BufferedImage(d.width, d.height, BufferedImage.TYPE_INT_ARGB);
            renderer=new Renderer.IntArrayFloat(offscreen);
        } else {
            offscreen=
                new BufferedImage(d.width, d.height, BufferedImage.TYPE_INT_ARGB);
            renderer=new Renderer.IntArray(offscreen);
        }
        Color c = getBackground();
        renderer.setBackgroundColor(c !=null? c.getRGB(): 0);
        renderer.setCameraPath(cameraPath);
        renderer.setSceneRoot(root);
      } else if(!backgroundExplicitlySet) {
        Color c = getBackground();//inherited from parent
        renderer.setBackgroundColor(c !=null? c.getRGB(): 0);
      }
//      System.out.println("start rendering "+new java.util.Date());
      try {
        renderer.render();
      } catch (Exception e) {
        LoggingSystem.getLogger(this).log(Level.SEVERE, "renderer.render() failed! ", e);
        
      }
      synchronized(this) {
        //imageValid = false;
        renderer.update();
        imageValid=true;
      }
//      System.out.println("end rendering "+new java.util.Date());
    } else {
      //System.out.println("no area to paint");
    }
  }

  protected void processMouseMotionEvent(MouseEvent e) {
    super.processMouseMotionEvent(e);
    if (ENFORCE_PAINT_ON_MOUSEEVENTS)
      render();
  }

  public void addNotify() {
    super.addNotify();
    requestFocus();
  }

  protected void processMouseEvent(MouseEvent e) {
    super.processMouseEvent(e);
    switch(e.getID()) {
      case MouseEvent.MOUSE_CLICKED: case MouseEvent.MOUSE_PRESSED:
      case MouseEvent.MOUSE_RELEASED:
        requestFocus();
    }
    if (ENFORCE_PAINT_ON_MOUSEEVENTS)
      render();
  }

  public void setBackground(Color c) {
    super.setBackground(c);
    backgroundExplicitlySet=c!=null;
    if(backgroundExplicitlySet&&renderer!=null)
      renderer.setBackgroundColor( c.getRGB());
  }

  public Renderer getRenderer() {
    return renderer;
  }
  private void paintImmediately() {
    if(!isShowing()) return;

    Component c=this;
    Component parent;
    Rectangle bounds=new Rectangle(0, 0, getWidth(), getHeight());

    for(parent=c.getParent();
        parent!=null && c.isLightweight();
        parent=c.getParent()) {
      bounds.x+=c.getX();
      bounds.y+=c.getY();
      c=parent;
    }
    Graphics gfx=c.getGraphics();
    gfx.setClip(bounds);
    c.paint(gfx);
  }
/* (non-Javadoc)
 * @see de.jreality.scene.Viewer#getCameraPath()
 */
public SceneGraphPath getCameraPath() {
    return cameraPath;
}
/* (non-Javadoc)
 * @see de.jreality.scene.Viewer#setCameraPath(de.jreality.util.SceneGraphPath)
 */
public void setCameraPath(SceneGraphPath p) {
    cameraPath = p;
    //camera = (Camera) p.getLastElement();
    
}
/* (non-Javadoc)
 * @see de.jreality.scene.Viewer#hasViewingComponent()
 */
public boolean hasViewingComponent() {
    return true;
}
/* (non-Javadoc)
 * @see de.jreality.scene.Viewer#initializeFrom(de.jreality.scene.Viewer)
 */
public void initializeFrom(Viewer v) {
    setSceneRoot(v.getSceneRoot());
    setCameraPath(v.getCameraPath());
}
/* (non-Javadoc)
 * @see de.jreality.scene.Viewer#getMetric()
 */
public int getMetric() {
    // TODO Auto-generated method stub
    return 0;
}
/* (non-Javadoc)
 * @see de.jreality.scene.Viewer#setMetric(int)
 */
public void setMetric(int sig) {
    // TODO Auto-generated method stub
    
}
/* (non-Javadoc)
 * @see de.jreality.scene.Viewer#setAuxiliaryRoot(de.jreality.scene.SceneGraphComponent)
 */
public void setAuxiliaryRoot(SceneGraphComponent ar) {
    throw new UnsupportedOperationException("not implemented");
}
/* (non-Javadoc)
 * @see de.jreality.scene.Viewer#getAuxiliaryRoot()
 */
public SceneGraphComponent getAuxiliaryRoot() {
    throw new UnsupportedOperationException("not implemented");
}
public Image getBackgroundImage() {
    return bgImage;
}
public void setBackgroundImage(Image bgImage) {
    this.bgImage = bgImage;
}
public Dimension getViewingComponentSize() {
  return getSize();
}
public boolean canRenderAsync() {
  return true;
}

public void renderAsync() {
  synchronized (renderLock) {
    if (upToDate) {
      upToDate=false;
      renderLock.notify();
      // TODO: Bloch warns of using Thread.yield()... but this
      // makes the rendering smooth in oorange (with jgimmick timer)
      Thread.yield(); // encourage the render thread to do it's work
    }
  }
}

public void dispose() {
  synchronized (renderLock) {
    disposed=true;
    renderLock.notify();
  }
}
}
