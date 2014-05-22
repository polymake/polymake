/*
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

package de.jreality.softviewer;

import java.awt.Color;
import java.util.List;

import de.jreality.math.Rn;
import de.jreality.scene.Appearance;
import de.jreality.scene.Camera;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.Transformation;
import de.jreality.scene.data.AttributeEntityUtility;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.CubeMap;
import de.jreality.util.DefaultMatrixSupport;
import de.jreality.util.SceneGraphUtility;

public abstract class AbstractRenderer {

    protected Camera camera;
    protected SceneGraphComponent root;
    protected SceneGraphComponent auxiliaryRoot;
    private SceneGraphPath cameraPath;
    private Transformation cameraWorld = new Transformation();
    protected TrianglePipeline pipeline;
    protected TriangleRasterizer rasterizer;
    protected RenderingVisitor renderTraversal;
    private Color leftlower;
    private Color rightupper;
    private Color leftupper;
    private Color rightlower;

    public AbstractRenderer(TriangleRasterizer r,boolean intersecting,boolean sortAll) {
        super();
        rasterizer = r;
        if(intersecting)
            pipeline = new IntersectingPipeline(rasterizer,sortAll);
        //    pipeline = new CutTrianglesPipeline(rasterizer);
        else
            pipeline = new TrianglePipeline(rasterizer, sortAll);
            //pipeline = new IntersectingPipeline(rasterizer,sortAll);
        renderTraversal = new RenderingVisitor();
        renderTraversal.setPipeline(pipeline);
    }

    public void setBackgroundColor(int c) {
            rasterizer.setBackground(c);
      }

    public int getBackgroundColor() {
        return rasterizer.getBackground();
      }

    protected void render(int width, int height) {
        //
        //make sure that the buffered image is of correct size:
        //
    //    if( (width != w || height != h) ) {
          rasterizer.setWindow(0, width, 0, height);
          rasterizer.setSize(width, height);
          
          //moved the following two behind the setting of perspective/orthographic:
          //pipeline.getPerspective().setWidth(width);
          //pipeline.getPerspective().setHeight(height);
    //    }
        Appearance a = root == null ? null : root.getAppearance();
          Color background;
          CubeMap sky= null;
          if(a != null) {
              // if there is a sky box read it
              if (AttributeEntityUtility.hasAttributeEntity(CubeMap.class,
                      CommonAttributes.SKY_BOX, a)) {
                  sky = (CubeMap) AttributeEntityUtility
                  .createAttributeEntity(CubeMap.class,
                          CommonAttributes.SKY_BOX, a, true);
              }
              
              Object o = a.getAttribute(CommonAttributes.BACKGROUND_COLORS);
              if(o instanceof Color[]) {
                  Color[] colors = (Color[] ) o;
                      rasterizer.setBackgroundColors(colors);
              } else
                  rasterizer.setBackgroundColors(null);
              o = a.getAttribute(CommonAttributes.BACKGROUND_COLOR);
    
              if( o instanceof Color) background = (Color) o;
              //else background = Color.WHITE;
              else background = CommonAttributes.BACKGROUND_COLOR_DEFAULT;
              
          } else {
              background = Color.WHITE;
              rasterizer.setBackgroundColors(null);
          }
        rasterizer.setBackground(background.getRGB());
        // only clear screen if sky != null
        rasterizer.clear(sky== null);
        //rasterizer.clear(true);
        rasterizer.start();
        //
        // set camera settings:
        //
        
        if (root != null && camera != null) {
            boolean isPerspective = camera.isPerspective();
            CameraProjection p =pipeline.getPerspective();
            if(isPerspective) {
                PerspectiveProjection pp = null;
                if(! (p instanceof PerspectiveProjection)) {
                    pp = new PerspectiveProjection();
                    pipeline.setPerspective(pp);
                } else
                    pp = (PerspectiveProjection) p;
                pp.setFieldOfViewDeg(camera.getFieldOfView());
                pp.setNear(camera.getNear());
                pp.setFar(camera.getFar());
                //pipeline.setPerspective(pp);
            } else {
                OrthographicProjection pp = null;
                if(! (p instanceof OrthographicProjection)) {
                    pp = new OrthographicProjection();
                    pipeline.setPerspective(pp);
                } else
                    pp = (OrthographicProjection) p;
                pp.setFieldOfViewDeg(camera.getFieldOfView());
                //pp.setFieldOfViewDeg(1);
                pp.setNear(camera.getNear());
                pp.setFar(camera.getFar());
                pp.setFocus(camera.getFocus());
                //pipeline.setPerspective(pp);
            }
            pipeline.getPerspective().setWidth(width);
            pipeline.getPerspective().setHeight(height);
          DefaultMatrixSupport.getSharedInstance().restoreDefault(cameraWorld, true);
          //cameraPath.applyEffectiveTransformation(cameraWorld);
          cameraWorld.setMatrix(cameraPath.getMatrix(null));
          //SceneGraphUtilities.applyEffectiveTransformation(cameraWorld,(SceneGraphComponent) camera.getParentNode(),root);
          
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
          if(auxiliaryRoot!= null)
              renderTraversal.traverse(auxiliaryRoot);
        
          //finally if there is a sky box raster it
          if(sky != null) {
              PrimitiveCache.renderSky(pipeline,sky);
          }
        }
        
        pipeline.finish();
        rasterizer.stop();
      }

    public SceneGraphPath getCameraPath() {
        return cameraPath;
      }

    public void setCameraPath(SceneGraphPath p) {
        cameraPath = p;
        camera= p == null ? null : (Camera) p.getLastElement();
      }

    public SceneGraphComponent getSceneRoot() {
        return root;
      }

    public void setSceneRoot(SceneGraphComponent component) {
        root=component;
        if(root!= null && camera !=null) {
            //cameraPath = new SceneGraphPath(root,camera); 
            List camPaths = SceneGraphUtility.getPathsBetween(root, camera);
            if (camPaths.size() > 0) cameraPath = (SceneGraphPath) camPaths.get(0);
            else {
              camera = null;
              cameraPath = null;
            }
        }
      }

    public SceneGraphComponent getAuxiliaryRoot() {
        return auxiliaryRoot;
      }

    public void setAuxiliaryRoot(SceneGraphComponent component) {
              auxiliaryRoot=component;
            if(root!= null && camera !=null) {
                //cameraPath = new SceneGraphPath(root,camera); 
                List camPaths = SceneGraphUtility.getPathsBetween(root, camera);
                if (camPaths.size() > 0) cameraPath = (SceneGraphPath) camPaths.get(0);
                else {
                  camera = null;
                  cameraPath = null;
                }
            }
      }
    
    public abstract void render();
    public abstract void update();
    
    public void setBestQuality(boolean b) {
        
        renderTraversal.setBestQuality(b);
        
    }


}
