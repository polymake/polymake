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

import java.awt.Color;
import java.awt.Dimension;

import de.jreality.math.Rn;
import de.jreality.scene.Appearance;
import de.jreality.scene.Camera;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.Transformation;
import de.jreality.scene.Viewer;
import de.jreality.shader.CommonAttributes;
import de.jreality.util.DefaultMatrixSupport;

/**
 * This is an experimental PS viewer for jReality.
 * It is still verry rudimentary and rather a 
 * proof of concept thatn a full featured PS writer.
 * @version 1.0
 * @author <a href="mailto:hoffmann@math.tu-berlin.de">Tim Hoffmann</a>
 *
 */
public abstract class AbstractViewer implements Viewer {

    protected Camera camera;
    protected int argbBackground;
    protected SceneGraphComponent root;
    private SceneGraphPath cameraPath;
    private Transformation cameraWorld = new Transformation();

    private PolygonPipeline pipeline;
    protected PolygonRasterizer rasterizer;
    private RenderTraversal renderTraversal;

    int width, height;
    
    public AbstractViewer() {
        super();
    }

    public void setBackgroundColor(int c) {
        argbBackground=c;
        rasterizer.setBackground(c);
    }
    
    void render(int width, int height) {
        
        pipeline =new PolygonPipeline(rasterizer,true);
        
        renderTraversal = new RenderTraversal();
        renderTraversal.setPipeline(pipeline);
        
        if(root == null || camera == null)
            throw new IllegalStateException("need camera and root node");
        if(width == 0 || height == 0) return;

        //
        //make sure that the rasterizer knows is of correct size:
        //
        rasterizer.setWindow(0, width, 0, height);
        rasterizer.setSize(width, height);
        rasterizer.start();
        
        pipeline.getPerspective().setWidth(width);
        pipeline.getPerspective().setHeight(height);

        Appearance a = root.getAppearance();
        Color background;
        if(a != null) {
            Object o = a.getAttribute(CommonAttributes.BACKGROUND_COLOR);
            if( o instanceof Color) background = (Color) o;
            else background = Color.WHITE;
        } else 
            background = Color.WHITE;
        rasterizer.setBackground(background.getRGB());

        rasterizer.clear();
        //
        // set camera settings:
        //
        
        DefaultPerspective p =( (DefaultPerspective)pipeline.getPerspective());
        p.setFieldOfViewDeg(camera.getFieldOfView());
        p.setNear(camera.getNear());
        p.setFar(camera.getFar());
        DefaultMatrixSupport.getSharedInstance().restoreDefault(cameraWorld, true);
        cameraWorld.multiplyOnLeft(cameraPath.getMatrix(null));
        
        //
        // traverse   
        //
        pipeline.clearPipeline();
        double[] im = new double[16];
        Rn.inverse(im,cameraWorld.getMatrix());
        //cameraWorld.getInverseMatrix(im);
        cameraWorld.setMatrix(im);
        renderTraversal.setInitialTransformation(cameraWorld);
        renderTraversal.traverse(root);
        
        //
        // sort
        // TODO: make intersections work
        //Intersector.intersectPolygons(pipeline);
        
        pipeline.sortPolygons();
        
        //
        // render
        //
        pipeline.renderRemaining(rasterizer);
        // TODO should this (start and stop)
        // be in the pipeline?
        rasterizer.stop();
        
    }    
    
//    public Camera getCamera() {
//        return camera;
//    }
//
//    public void setCamera(Camera aCamera) {
//        camera=aCamera;
//        if(root!= null && camera !=null)
//            cameraPath = new SceneGraphPath(root,camera); 
//    }

    public SceneGraphComponent getSceneRoot() {
        return root;
    }

    public void setSceneRoot(SceneGraphComponent component) {
        root=component;
//        if(root!= null && camera !=null)
//            cameraPath = new SceneGraphPath(root,camera); 
    }
    
    
    
    /* (non-Javadoc)
     * @see de.jreality.soft.Viewer#getViewingComponent()
     */
    public Object getViewingComponent() {
        return null;
    }

    public Dimension getViewingComponentSize() {
      return null;
    }

    /* (non-Javadoc)
     * @see de.jreality.soft.Viewer#render()
     */
    public void render() {
        render(width,height);
    }

    /**
     * @param height The height to set.
     */
    public void setHeight(int height) {
        this.height = height;
    }

    /**
     * @param width The width to set.
     */
    public void setWidth(int width) {
        this.width = width;
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
        cameraPath =p;
        camera =(Camera) p.getLastElement();
        
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
        setSceneRoot(v.getSceneRoot());
        setCameraPath(v.getCameraPath());
    }

    public boolean canRenderAsync() {
      return false;
    }
    
    public void renderAsync() {
      throw new UnsupportedOperationException();
    }
}
