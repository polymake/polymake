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


package de.jreality.util;

import java.util.Iterator;
import java.util.Set;

import de.jreality.scene.Appearance;
import de.jreality.scene.Camera;
import de.jreality.scene.ClippingPlane;
import de.jreality.scene.Cylinder;
import de.jreality.scene.DirectionalLight;
import de.jreality.scene.Geometry;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.IndexedLineSet;
import de.jreality.scene.Light;
import de.jreality.scene.PointLight;
import de.jreality.scene.PointSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphNode;
import de.jreality.scene.SceneGraphVisitor;
import de.jreality.scene.Sphere;
import de.jreality.scene.SpotLight;
import de.jreality.scene.Transformation;

/**
 * Creates a copy of the visited SceneGraphNode. The copy may be accessed with getCopy().
 * Note: The copied attributes do not include children (for SceneGraphComponents).
 *  
 * @author Steffen Weissmann
 *
 */
public class CopyVisitor extends SceneGraphVisitor {

    SceneGraphNode created;

    public SceneGraphNode getCopy() {
        return created;
    }

    public void visit(Appearance a) {
        Appearance newApp= new Appearance();
        copyAttr(a, newApp);
        created=newApp;
    }

    public void visit(Camera c) {
        Camera newCamera= new Camera();
        copyAttr(c, newCamera);
        created=newCamera;
    }

    public void visit(Cylinder c) {
        Cylinder newCyl= new Cylinder();
        copyAttr(c, newCyl);
        created=newCyl;
    }

    public void visit(DirectionalLight l) {
        DirectionalLight newLight=new DirectionalLight();
        copyAttr(l, newLight);
        created=newLight;
    }

    public void visit(IndexedFaceSet i) {
        IndexedFaceSet newIFS=new IndexedFaceSet();
        copyAttr(i, newIFS);
        created=newIFS;
    }

    public void visit(IndexedLineSet g) {
        IndexedLineSet newILS=new IndexedLineSet();
        copyAttr(g, newILS);
        created=newILS;
    }

    public void visit(PointSet p) {
        PointSet newPS=new PointSet();
        copyAttr(p, newPS);
        created=newPS;
    }

    public void visit(SceneGraphComponent c) {
        SceneGraphComponent newSGC=new SceneGraphComponent();
        copyAttr(c, newSGC);//Note: attributes does not include children
        created=newSGC;
    }

    public void visit(Sphere s) {
        Sphere newSphere= new Sphere();
        copyAttr(s, newSphere);
        created=newSphere;
    }

    public void visit(SpotLight l) {
        SpotLight newLight=new SpotLight();
        copyAttr(l, newLight);
        created=newLight;
    }

    public void visit(ClippingPlane c)
    {
        ClippingPlane newCP=new ClippingPlane();
        copyAttr(c, newCP);
        created=newCP;
    }

    public void visit(PointLight l)
    {
        PointLight newLight=new PointLight();
        copyAttr(l, newLight);
        created=newLight;
    }

    public void visit(Transformation t) {
        Transformation newTrans=new Transformation();
        copyAttr(t, newTrans);
        created=newTrans;
    }

  public void copyAttr(SceneGraphNode src, SceneGraphNode dst) {
    dst.setName(src.getName());
  }

  public void copyAttr(SceneGraphComponent src, SceneGraphComponent dst) {
      copyAttr((SceneGraphNode)src, (SceneGraphNode)dst);
      dst.setVisible(src.isVisible());
      dst.setPickable(src.isPickable());
      dst.setOwner(src.getOwner());
  }

 public void copyAttr(Appearance src, Appearance dst) {
      Set lst = src.getStoredAttributes();
      for (Iterator i = lst.iterator(); i.hasNext(); ) {
        String aName = (String) i.next();
        dst.setAttribute(aName, src.getAttribute(aName));
      }
      copyAttr((SceneGraphNode) src, (SceneGraphNode) dst);
  }

  public void copyAttr(Transformation src, Transformation dst) {
      dst.setMatrix(src.getMatrix());
      copyAttr((SceneGraphNode)src, (SceneGraphNode)dst);
  }

  public void copyAttr(Light src, Light dst) {
      dst.setColor(src.getColor());
      dst.setIntensity(src.getIntensity());
      copyAttr((SceneGraphNode)src, (SceneGraphNode)dst);
  }

  public void copyAttr(DirectionalLight src, DirectionalLight dst) {
      copyAttr((Light)src, (Light)dst);
  }

  public void copyAttr(SpotLight src, SpotLight dst) {
      dst.setConeAngle(src.getConeAngle());
      dst.setConeDeltaAngle(src.getConeDeltaAngle());
      dst.setFalloffA0(src.getFalloffA0());
      dst.setFalloffA1(src.getFalloffA1());
      dst.setFalloffA2(src.getFalloffA2());
      dst.setDistribution(src.getDistribution());
      dst.setUseShadowMap(src.isUseShadowMap());
      dst.setShadowMapX(src.getShadowMapX());
      dst.setShadowMapY(src.getShadowMapY());
      dst.setShadowMap(src.getShadowMap());
      copyAttr((Light)src, (Light)dst);
  }

  public void copyAttr(Geometry src, Geometry dst) {
  	  dst.setGeometryAttributes(src.getGeometryAttributes());
      copyAttr((SceneGraphNode)src, (SceneGraphNode)dst);
  }

  public void copyAttr(Sphere src, Sphere dst) {
      copyAttr((Geometry)src, (Geometry)dst);
  }
  
  public void copyAttr(Cylinder src, Cylinder dst) {
      copyAttr((Geometry)src, (Geometry)dst);
  }

  public void copyAttr(PointSet src, PointSet dst) {
    dst.setVertexCountAndAttributes(src.getVertexAttributes());
      copyAttr((Geometry)src, (Geometry)dst);
  }

  public void copyAttr(IndexedLineSet src, IndexedLineSet dst) {
    dst.setEdgeCountAndAttributes(src.getEdgeAttributes());
      copyAttr((PointSet)src, (PointSet)dst);
  }

  public void copyAttr(IndexedFaceSet src, IndexedFaceSet dst) {
    dst.setFaceCountAndAttributes(src.getFaceAttributes());
      copyAttr((IndexedLineSet)src, (IndexedLineSet)dst);
  }

//  public void copyAttr(QuadMeshShape src, QuadMeshShape dst) {
//      copyAttr((IndexedFaceSet)src, (IndexedFaceSet)dst);
//  }

  public void copyAttr(Camera src, Camera dst) {
//  	dst.setAspectRatio(src.getAspectRatio());
  	dst.setEyeSeparation(src.getEyeSeparation());
  	dst.setFar(src.getFar());
  	dst.setFieldOfView(src.getFieldOfView());
  	dst.setFocus(src.getFocus());
  	dst.setNear(src.getNear());
  	dst.setOnAxis(src.isOnAxis());
  	dst.setOrientationMatrix(src.getOrientationMatrix());
  	dst.setPerspective(src.isPerspective());
//  	dst.setMetric(src.getMetric());
  	dst.setStereo(src.isStereo());
  	if (!src.isOnAxis()) dst.setViewPort(src.getViewPort());
    copyAttr((SceneGraphNode)src, (SceneGraphNode)dst);
  }

    public void visit(SceneGraphNode m) {
        throw new IllegalStateException(
          m.getClass()+" not handled by "+getClass().getName());
    }

}
