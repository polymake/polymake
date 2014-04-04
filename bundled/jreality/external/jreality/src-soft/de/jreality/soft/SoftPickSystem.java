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

import java.util.List;

import de.jreality.math.MatrixBuilder;
import de.jreality.math.Pn;
import de.jreality.math.Rn;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.Transformation;
import de.jreality.scene.pick.PickSystem;

/**
 * 
 * @version 1.0
 * @author timh
 *
 */
public class SoftPickSystem implements PickSystem {
    private SceneGraphComponent root;
    private PolygonPipeline pipeline;
    private PickPerspective perspective = new PickPerspective();
    private NewDoublePolygonRasterizer rasterizer;
    private PickVisitor pickVisitor = new PickVisitor();

    private Transformation cameraWorld = new Transformation();
    
    public SoftPickSystem() {
        super();
        int pixels[] = new int[1];
        
        rasterizer = new NewDoublePolygonRasterizer(pixels,true,true,pickVisitor.getHitDetector());
        pipeline = new PolygonPipeline(rasterizer);
        pipeline.setPerspective(perspective);
        pickVisitor.setPipeline(pipeline);
    }
    
    /* (non-Javadoc)
     * @see de.jreality.scene.tool.PickSystem#setSceneRoot(de.jreality.scene.SceneGraphComponent)
     */
    public void setSceneRoot(SceneGraphComponent root) {
        this.root = root;

    }

    public List computePick(double[] from, double[] to) {
      if (to.length < 4 || to[3] == 0) return computePickImpl(from, to, 1000);
      double[] dir = new double[3];
      Pn.dehomogenize(from, from);
      Pn.dehomogenize(to, to);
      dir[0] = to[0]-from[0];
      dir[1] = to[1]-from[1];
      dir[2] = to[2]-from[2];
      return computePickImpl(from, dir, Rn.euclideanNorm(dir));
      
    }
    
    /* (non-Javadoc)
     * @see de.jreality.scene.tool.PickSystem#computePick(double[], double[])
     */
    public List computePickImpl(double[] foot, double[] direction, double far) {
        rasterizer.setWindow(0, 1, 0, 1);
        rasterizer.setSize(1, 1);
        rasterizer.start();
        int wh = 200;
        int hh = 200;
        perspective.setWidth(2*wh);
        perspective.setHeight(2*hh);
        
        perspective.setPickPoint(wh, hh);
        
        rasterizer.clear();
        //
        // set camera settings:
        //
        

        perspective.setFieldOfViewDeg(0.1);
        perspective.setNear(1);
        perspective.setFar(far);
        pickVisitor.getHitDetector().setNdcToCamera(perspective.getInverseMatrix(null));
        
//        double dd[] = (double[]) direction.clone(); 
//        normalToEuler(dd);
//        VecMat.assignRotationZ(tmp  = new double[16],disk[5]);
//        t.multiplyOnRight(tmp);
//        
//        VecMat.assignRotationY(tmp,disk[4]);
//        t.multiplyOnRight(tmp);
//        
//        VecMat.assignRotationX(tmp,disk[3]+Math.PI/2.);
//        t.multiplyOnRight(tmp);
        
        MatrixBuilder mb = MatrixBuilder.euclidean();
        mb.translate(foot[0],foot[1],foot[2]);
        mb.rotateFromTo(new double[] {0,0,-1},direction);
        //cameraWorld.resetMatrix();
        //cameraWorld.multiplyOnLeft(cameraPath.getMatrix(null));
        
        //
        // traverse   
        //
        pipeline.clearPipeline();
        double[] im = new double[16];
        Rn.inverse(im,mb.getMatrix().getArray());
        //cameraWorld.getInverseMatrix(im);
        cameraWorld.setMatrix(im);
        pickVisitor.setInitialTransformation(cameraWorld);
        pickVisitor.traverse(root);
        
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
        
        return pickVisitor.getHitDetector().getHitList();
    }
    
    
    /**
     * changes the normal (i.e. components 3,4,5) of the i-th result of
     * centerNormalRadius into euler rotation angles.
     */
    private static void normalToEuler(double r[]) {
        double x = r[0];
        double y = r[1];
        double z = r[2];
        double xrot = 0;
        double zrot = 0;
        double yrot = 0;
        
//  if(x*x+y*y -0.0001> 0.) {
//      xrot =  -Math.acos(z);
//      zrot =  Math.atan2(y,x);
//  }
        if(z*z +x*x -0.000001> 0.) {
            xrot =  Math.acos(y);
            yrot =  Math.atan2(x,z);
        }
        //e.set(xrot,yrot,zrot);
        r[0] = xrot;
        r[0] = yrot;
        r[0] = zrot;
        //e.set(Math.PI/2,0,Math.PI/2.);
        //System.err.println("rot "+e+ "   "+ x+ " "+y+" "+z);
    }


}
