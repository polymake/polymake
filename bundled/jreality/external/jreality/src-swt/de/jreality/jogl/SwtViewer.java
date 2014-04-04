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


package de.jreality.jogl;

import java.awt.Dimension;
import java.util.logging.Level;

import javax.media.opengl.GL;
import javax.media.opengl.GLContext;
import javax.media.opengl.GLDrawableFactory;

import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.opengl.GLCanvas;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Listener;

import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;
import de.jreality.util.SceneGraphUtility;
/**
 * @author Charles Gunn
 *
 */
public class SwtViewer implements de.jreality.scene.Viewer, Runnable {
	
  protected SceneGraphComponent sceneRoot;
	SceneGraphComponent auxiliaryRoot;
	SceneGraphPath cameraPath;

	JOGLRenderer renderer;
	
  public static final int 	CROSS_EYED_STEREO = 0;
	public static final int 	RED_BLUE_STEREO = 1;
	public static final int 	RED_GREEN_STEREO = 2;
	public static final int 	RED_CYAN_STEREO =  3;
	public static final int 	HARDWARE_BUFFER_STEREO = 4;
	public static final int 	STEREO_TYPES = 5;
	int stereoType = 		CROSS_EYED_STEREO;	
	

  GLCanvas canvas;
  private int metric;
  
  private Dimension myDimension = new Dimension();

	public SwtViewer(GLCanvas canvas) {
		this(null, null, canvas);
	}
  
	public SwtViewer(SceneGraphPath camPath, SceneGraphComponent root, GLCanvas canvas) {
		super();
    this.canvas=canvas;
    setAuxiliaryRoot(SceneGraphUtility.createFullSceneGraphComponent("AuxiliaryRoot"));
		initializeFrom(root, camPath);
    SwtQueue.getInstance().waitFor(new Runnable() {
      public void run() {init();};
    });
	}

	public SceneGraphComponent getSceneRoot() {
		return sceneRoot;
	}

	public void setSceneRoot(SceneGraphComponent r) {
		if (r == null)	{
			JOGLConfiguration.getLogger().log(Level.WARNING,"Null scene root, not setting.");
			return;
		}
		sceneRoot = r;
	}

	public SceneGraphComponent getAuxiliaryRoot() {
		return auxiliaryRoot;
	}
	public void setAuxiliaryRoot(SceneGraphComponent auxiliaryRoot) {
		this.auxiliaryRoot = auxiliaryRoot;
		if (renderer != null) renderer.setAuxiliaryRoot(auxiliaryRoot);
	}

	public SceneGraphPath getCameraPath() {
		return cameraPath;
	}

	public void setCameraPath(SceneGraphPath p) {
		cameraPath = p;
	}

	public void renderAsync() {
//		if (isLinux)	{
//		    canvas.setIgnoreRepaint(false);
//			canvas.setNoAutoRedrawMode(false);			
//		}
	    synchronized (renderLock) {
				if (!pendingUpdate) {
          if (canvas.isDisposed()) return;
          if (Thread.currentThread() == canvas.getDisplay().getThread())
            run();
          else {
            canvas.getDisplay().asyncExec(this);
	  				pendingUpdate = true;
          }
				}
			}
	}

	public boolean hasViewingComponent() {
		return false;
	}


	public Object getViewingComponent() {
		return canvas;
	}

	/* (non-Javadoc)
	 * @see de.jreality.scene.Viewer#initializeFrom(de.jreality.scene.Viewer)
	 */
	public void initializeFrom(de.jreality.scene.Viewer v) {
		initializeFrom(v.getSceneRoot(), v.getCameraPath());
	}
	
	public int getMetric() {
		return metric;
	}
	public void setMetric(int metric) {
		this.metric = metric;
		SceneGraphUtility.setMetric(sceneRoot, metric);
		
	}
	
/*********** Non-standard set/get ******************/
	
  public void setStereoType(int type) {
    renderer.setStereoType(type);
  }
  
  // used in JOGLRenderer
  public int getStereoType()  {
    return renderer.getStereoType();
  }

//  public boolean isFlipped() {
//    return renderer.isFlipped();
//  }
//  public void setFlipped(boolean isFlipped) {
//    renderer.setFlipped(isFlipped);
//  }
//
  public JOGLRenderer getRenderer() {
    return renderer;
  }
		
		/****** Convenience methods ************/
		public void addAuxiliaryComponent(SceneGraphComponent aux)	{
			if (auxiliaryRoot == null)	{
				setAuxiliaryRoot(SceneGraphUtility.createFullSceneGraphComponent("AuxiliaryRoot"));
			}
			if (!auxiliaryRoot.isDirectAncestor(aux)) auxiliaryRoot.addChild(aux);
		}
		
		public void removeAuxiliaryComponent(SceneGraphComponent aux)	{
			if (auxiliaryRoot == null)	return;
			if (!auxiliaryRoot.isDirectAncestor(aux) ) return;
			auxiliaryRoot.removeChild(aux);
		}
		
	  private void initializeFrom(SceneGraphComponent r, SceneGraphPath p)	{
		  setSceneRoot(r);
		  setCameraPath(p);
      renderer=new JOGLRenderer(this);
    }
    
	private boolean pendingUpdate;
	
	private final Object renderLock=new Object();
	boolean autoSwapBuffers=true;
	
	public boolean isRendering() {
		synchronized(renderLock) {
			return pendingUpdate;
		}
	}
	
  private GLContext context;
  
  boolean init = true;
  
  int rot = 0;

  public void init() {
    canvas.addListener(SWT.Resize, new Listener() {

      public void handleEvent(Event event) {
        Rectangle bounds = canvas.getBounds();
        myDimension.width=bounds.width;
        myDimension.height=bounds.height;
        renderer.setSize(myDimension.width, myDimension.height);
      }
    });
    context = GLDrawableFactory.getFactory().createExternalGLContext();
    Rectangle bounds = canvas.getBounds();
    myDimension.width=bounds.width;
    myDimension.height=bounds.height;
    renderer.setSize(myDimension.width, myDimension.height);
    context.makeCurrent();
    GL gl = context.getGL();
    renderer.init(gl);
    context.release();
  }
  
	public void run() {
    if (canvas.isDisposed()) return;
		if (Thread.currentThread() != canvas.getDisplay().getThread())
			throw new IllegalStateException();
		synchronized (renderLock) {
			pendingUpdate = false;
      canvas.setCurrent();
      context.makeCurrent();
      GL gl = context.getGL();
      renderer.display(gl);
      canvas.swapBuffers();
      context.release();
			renderLock.notifyAll();
		}
	}

  public GLCanvas getGLCanvas() {
    return canvas;
  }
  
  public double getAspectRatio() {
    return renderer.getAspectRatio(); 
  }

  public Dimension getViewingComponentSize() {
    return new Dimension(myDimension);
  }

  public boolean canRenderAsync() {
    return true;
  }
  
  public void render() {
    SwtQueue.getInstance().waitFor(this);
  }

}
