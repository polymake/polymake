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

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;

import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.Viewer;

/**
 * This is an experimental SVG viewer for jReality.
 * It is still verry rudimentary and rather a 
 * proof of concept thatn a full featured SVG writer.
 * @version 1.0
 * @author <a href="mailto:hoffmann@math.tu-berlin.de">Tim Hoffmann</a>
 *
 */
public class SVGViewer extends AbstractViewer implements Viewer {


    
    //private SVGRasterizer rasterizer;
    
    private String fileName;
    
    /**
     * 
     */
    public SVGViewer(String file) {
        super();
        fileName =file;
    }

    public void setBackgroundColor(int c) {
        argbBackground=c;
        rasterizer.setBackground(c);
    }
    
    void render(int width, int height) {
        File f=new File(fileName);
        PrintWriter w;
        try {
            w = new PrintWriter(new FileWriter(f));
            rasterizer =new SVGRasterizer(w);
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
//        pipeline =new PolygonPipeline(rasterizer,true);
//        
//        renderTraversal = new RenderTraversal();
//        renderTraversal.setPipeline(pipeline);
//        
//        if(root == null || camera == null)
//            throw new IllegalStateException("need camera and root node");
//        if(width == 0 || height == 0) return;
//
//        //
//        //make sure that the buffered image is of correct size:
//        //
////    if( (width != w || height != h) ) {
//        rasterizer.setWindow(0, width, 0, height);
//        rasterizer.setSize(width, height);
//        rasterizer.start();
//        
//        pipeline.getPerspective().setWidth(width);
//        pipeline.getPerspective().setHeight(height);
////    }
//        rasterizer.clear();
//        //
//        // set camera settings:
//        //
//        
//        DefaultPerspective p =( (DefaultPerspective)pipeline.getPerspective());
//        p.setFieldOfViewDeg(camera.getFieldOfView());
//        p.setNear(camera.getNear());
//        p.setFar(camera.getFar());
//        cameraWorld.resetMatrix();
//        cameraPath.applyEffectiveTransformation(cameraWorld);
//        //SceneGraphUtilities.applyEffectiveTransformation(cameraWorld,(SceneGraphComponent) camera.getParentNode(),root);
//        
//        //
//        // traverse   
//        //
//        pipeline.clearPipeline();
//        double[] im = new double[16];
//        Rn.inverse(im,cameraWorld.getMatrix());
//        //cameraWorld.getInverseMatrix(im);
//        cameraWorld.setMatrix(im);
//        renderTraversal.setInitialTransformation(cameraWorld);
//        renderTraversal.traverse(root);
//        
//        //
//        // sort
//        // TODO: make sorting customizable
//        pipeline.sortPolygons();
//        
//        //
//        // render
//        //
//        pipeline.renderRemaining(rasterizer);
//        // TODO should this (start and stop)
//        // be in the pipeline?
//        rasterizer.stop();
//        
        super.render(width,height);
        w.close();
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
    
}
