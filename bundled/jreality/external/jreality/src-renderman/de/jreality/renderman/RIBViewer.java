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


package de.jreality.renderman;

import java.awt.Dimension;

import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.Viewer;

/**
 * 
 * @version 1.0
 * @author <a href="mailto:hoffmann@math.tu-berlin.de">Tim Hoffmann</a>
 *
 */
public class RIBViewer implements Viewer {
    private SceneGraphPath cameraPath;
    private SceneGraphComponent sceneRoot;
    private int width=100;
    private int height=100;
    private String fileName="test.rib";
    private int maximumEyeSplits = 10;
    private int rendererType = TYPE_PIXAR;
    
    // features depend on the type of renderman renderer being used
    public final static int TYPE_PIXAR = 1;
    public final static int TYPE_3DELIGHT = 2;
    public final static int TYPE_AQSIS = 3;
    public final static int TYPE_PIXIE = 4;
    
    /**
     * 
     */
    public RIBViewer() {
        super();
    }

    /* (non-Javadoc)
     * At the moment there is no viewing component. It is planned to have a java displaydriver
     * for aqsis in the future.
     * @see de.jreality.soft.Viewer#getViewingComponent()
     */
    public Object getViewingComponent() {
        return null;
    }

    /* (non-Javadoc)
     * @see de.jreality.soft.Viewer#setSceneRoot(de.jreality.scene.SceneGraphComponent)
     */
    public void setSceneRoot(SceneGraphComponent c) {
        sceneRoot =c;
    }

    /* (non-Javadoc)
     * @see de.jreality.soft.Viewer#getSceneRoot()
     */
    public SceneGraphComponent getSceneRoot() {
        return sceneRoot;
    }


    /* (non-Javadoc)
     * @see de.jreality.soft.Viewer#render()
     */
    public void render() {
        RIBVisitor rv =new RIBVisitor();
        rv.setRendererType(rendererType);
        rv.setWidth(width);
        rv.setHeight(height);
        rv.setMaximumEyeSplits(maximumEyeSplits);
        System.out.print(" Rendering renderman RIB into "+fileName+"..");
        rv.visit(this,fileName); 
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
    }

    /* (non-Javadoc)
     * @see de.jreality.scene.Viewer#hasViewingComponent()
     */
    public boolean hasViewingComponent() {
        return false;
    }
  
    /* (non-Javadoc)
     * @see de.jreality.scene.Viewer#initializeFrom(de.jreality.scene.Viewer)
     */
    public void initializeFrom(Viewer v) {
        cameraPath = v.getCameraPath();
        sceneRoot = v.getSceneRoot();
        if (v.hasViewingComponent()){
			setHeight((int) v.getViewingComponentSize().getHeight());
			setWidth((int) v.getViewingComponentSize().getWidth());
            System.out.println(" w "+v.getViewingComponentSize().getWidth()+ " h "+v.getViewingComponentSize().getHeight());
        }
    }

     public int getMetric() {
        // TODO Auto-generated method stub
        return 0;
    }

     public void setMetric(int sig) {
     }

    public void setAuxiliaryRoot(SceneGraphComponent ar) {
        throw new UnsupportedOperationException("not implemented");
    }

    public SceneGraphComponent getAuxiliaryRoot() {
        throw new UnsupportedOperationException("not implemented");
    }
    
    public void setRendererType(int type)	{
    	rendererType = type;
    }
   /**
     * @return Returns the fileName.
     */
    public String getFileName() {
        return fileName;
    }
    /**
     * @param fileName The fileName to set.
     */
    public void setFileName(String fileName) {
        this.fileName = fileName;
    }
    /**
     * @return Returns the height.
     */
    public int getHeight() {
        return height;
    }
    /**
     * @param height The height to set.
     */
    public void setHeight(int height) {
        this.height = height;
    }
    /**
     * @return Returns the width.
     */
    public int getWidth() {
        return width;
    }
    /**
     * @param width The width to set.
     */
    public void setWidth(int width) {
        this.width = width;
    }

    public Dimension getViewingComponentSize() {
      return null;
    }

    public boolean canRenderAsync() {
      return false;
    }

    public void renderAsync() {
      throw new UnsupportedOperationException();
    }

}
