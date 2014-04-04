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


package de.jreality.scene.proxy.smrj;

import java.io.IOException;
import java.io.Serializable;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Set;

import de.jreality.scene.data.ByteBufferList;
import de.jreality.scene.data.DataListSet;
import de.jreality.scene.proxy.ProxyFactory;
import de.jreality.scene.proxy.scene.Appearance;
import de.jreality.scene.proxy.scene.Camera;
import de.jreality.scene.proxy.scene.ClippingPlane;
import de.jreality.scene.proxy.scene.Cylinder;
import de.jreality.scene.proxy.scene.DirectionalLight;
import de.jreality.scene.proxy.scene.IndexedFaceSet;
import de.jreality.scene.proxy.scene.IndexedLineSet;
import de.jreality.scene.proxy.scene.PointLight;
import de.jreality.scene.proxy.scene.PointSet;
import de.jreality.scene.proxy.scene.RemoteAppearance;
import de.jreality.scene.proxy.scene.RemoteCamera;
import de.jreality.scene.proxy.scene.RemoteClippingPlane;
import de.jreality.scene.proxy.scene.RemoteCylinder;
import de.jreality.scene.proxy.scene.RemoteDirectionalLight;
import de.jreality.scene.proxy.scene.RemoteGeometry;
import de.jreality.scene.proxy.scene.RemoteIndexedFaceSet;
import de.jreality.scene.proxy.scene.RemoteIndexedLineSet;
import de.jreality.scene.proxy.scene.RemoteLight;
import de.jreality.scene.proxy.scene.RemotePointLight;
import de.jreality.scene.proxy.scene.RemotePointSet;
import de.jreality.scene.proxy.scene.RemoteSceneGraphComponent;
import de.jreality.scene.proxy.scene.RemoteSceneGraphNode;
import de.jreality.scene.proxy.scene.RemoteSphere;
import de.jreality.scene.proxy.scene.RemoteSpotLight;
import de.jreality.scene.proxy.scene.RemoteTransformation;
import de.jreality.scene.proxy.scene.SceneGraphComponent;
import de.jreality.scene.proxy.scene.Sphere;
import de.jreality.scene.proxy.scene.SpotLight;
import de.jreality.scene.proxy.scene.Transformation;
import de.smrj.RemoteFactory;

/**
 * this class should work like the inherited copy factory but copying objects on remote places
 * 
 * @author weissman
 */
public class SMRJMirrorFactory extends ProxyFactory {

    RemoteFactory rf;
    Object created;
    
    public SMRJMirrorFactory(RemoteFactory rf) {
        this.rf = rf;
     }
    
    public Object getProxy() {
        return created;
    }

    private Object createRemote(Class clazz) {
        try {
            return rf.createRemote(clazz);
        } catch (IOException ie) {
            throw new IllegalStateException("IO Error");
        }
    }

    public void visit(de.jreality.scene.Appearance a) {
        created=createRemote(Appearance.class);
        copyAttr(a, (RemoteAppearance) created);
    }

    public void visit(de.jreality.scene.Camera c) {
        created=createRemote(Camera.class);
        copyAttr(c, (RemoteCamera)created);
    }

    public void visit(de.jreality.scene.Cylinder c) {
        created=createRemote(Cylinder.class);
        copyAttr(c, (RemoteCylinder)created);
    }

    public void visit(de.jreality.scene.DirectionalLight l) {
        created=createRemote(DirectionalLight.class);
        copyAttr(l, (RemoteDirectionalLight) created);
    }

    public void visit(de.jreality.scene.IndexedFaceSet i) {
        created=createRemote(IndexedFaceSet.class);
        copyAttr(i, (RemoteIndexedFaceSet)created);
    }

    public void visit(de.jreality.scene.IndexedLineSet ils) {
        created=createRemote(IndexedLineSet.class);
        copyAttr(ils, (RemoteIndexedLineSet)created);
    }

    public void visit(de.jreality.scene.PointSet p) {
        created=createRemote(PointSet.class);
        copyAttr(p, (RemotePointSet)created);
    }

    public void visit(de.jreality.scene.SceneGraphComponent c) {
        created=createRemote(SceneGraphComponent.class);
        copyAttr(c, (RemoteSceneGraphComponent)created);
    }

    public void visit(de.jreality.scene.Sphere s) {
        created=createRemote(Sphere.class);
        copyAttr(s, (RemoteSphere)created);
    }

    public void visit(de.jreality.scene.SpotLight l) {
        created=createRemote(SpotLight.class);
        copyAttr(l, (RemoteSpotLight)created);
    }

    public void visit(de.jreality.scene.ClippingPlane c) {
        created=createRemote(ClippingPlane.class);
        copyAttr(c, (RemoteClippingPlane)created);
    }

    public void visit(de.jreality.scene.PointLight l) {
        created=createRemote(PointLight.class);
        copyAttr(l, (RemotePointLight)created);
    }

    public void visit(de.jreality.scene.Transformation t) {
        created=createRemote(Transformation.class);
        copyAttr(t, (RemoteTransformation)created);
    }

    public void visit(de.jreality.scene.SceneGraphNode m) {
        throw new IllegalStateException(m.getClass() + " not handled by "
                + getClass().getName());
    }

    public void copyAttr(de.jreality.scene.SceneGraphNode src,
            RemoteSceneGraphNode dst) {
        dst.setName(src.getName());
    }

    public void copyAttr(de.jreality.scene.SceneGraphComponent src,
            RemoteSceneGraphComponent dst) {
        copyAttr((de.jreality.scene.SceneGraphNode) src,
                (RemoteSceneGraphNode) dst);
        dst.setVisible(src.isVisible());
    }

    public void copyAttr(de.jreality.scene.Appearance src, RemoteAppearance dst) {
        copyAttr((de.jreality.scene.SceneGraphNode) src,
                (RemoteSceneGraphNode) dst);
        Set lst = src.getStoredAttributes();
        for (Iterator i = lst.iterator(); i.hasNext(); ) {
          String aName = (String) i.next();
            Object aa = src.getAttribute(aName);
            if (aa == Appearance.INHERITED) aa = de.jreality.scene.proxy.scene.Appearance.INHERITED;
            if (aa == Appearance.DEFAULT) aa = de.jreality.scene.proxy.scene.Appearance.DEFAULT;            
			dst.setAttribute(aName, aa);
        }
    }

    public void copyAttr(de.jreality.scene.Transformation src,
            RemoteTransformation dst) {
        copyAttr((de.jreality.scene.SceneGraphNode) src,
                (RemoteSceneGraphNode) dst);
        dst.setMatrix(src.getMatrix());
    }

    public void copyAttr(de.jreality.scene.Light src, RemoteLight dst) {
        copyAttr((de.jreality.scene.SceneGraphNode) src,
                (RemoteSceneGraphNode) dst);
        dst.setColor(src.getColor());
        dst.setIntensity(src.getIntensity());
    }

    public void copyAttr(de.jreality.scene.DirectionalLight src,
            RemoteDirectionalLight dst) {
        copyAttr((de.jreality.scene.Light) src, (RemoteLight) dst);
    }

    public void copyAttr(de.jreality.scene.PointLight src, RemotePointLight dst) {
      
      copyAttr((de.jreality.scene.Light) src, (RemoteLight) dst);

      dst.setFalloffA0(src.getFalloffA0());
      dst.setFalloffA1(src.getFalloffA1());
      dst.setFalloffA2(src.getFalloffA2());
      dst.setUseShadowMap(src.isUseShadowMap());
      dst.setShadowMapX(src.getShadowMapX());
      dst.setShadowMapY(src.getShadowMapY());
      dst.setShadowMap(src.getShadowMap());
    }
    
    public void copyAttr(de.jreality.scene.SpotLight src, RemoteSpotLight dst) {
        
      copyAttr((de.jreality.scene.PointLight) src, (RemotePointLight) dst);
        
      dst.setConeAngle(src.getConeAngle());
      dst.setConeDeltaAngle(src.getConeDeltaAngle());
      dst.setDistribution(src.getDistribution());
    }

    public void copyAttr(de.jreality.scene.Geometry src, RemoteGeometry dst) {
        copyAttr((de.jreality.scene.SceneGraphNode) src,
                (RemoteSceneGraphNode) dst);
        HashMap serializableGeometryAttributes = new HashMap();
        for (Iterator i = src.getGeometryAttributes().keySet().iterator(); i.hasNext(); ) {
          Object key = i.next();
          Object attr = src.getGeometryAttributes().get(key);
          if (attr instanceof Serializable)
        	  serializableGeometryAttributes.put(key, attr);
        }
        dst.setGeometryAttributes(serializableGeometryAttributes);
    }

    public void copyAttr(de.jreality.scene.Sphere src, RemoteSphere dst) {
        copyAttr((de.jreality.scene.Geometry) src, (RemoteGeometry) dst);
    }

    public void copyAttr(de.jreality.scene.Cylinder src, RemoteCylinder dst) {
        copyAttr((de.jreality.scene.Geometry) src, (RemoteGeometry) dst);
    }

    public void copyAttr(de.jreality.scene.PointSet src, RemotePointSet dst) {
        copyAttr((de.jreality.scene.Geometry) src, (RemoteGeometry) dst);
        DataListSet dls = ByteBufferList.prepareDataListSet(src.getVertexAttributes());
        dst.setVertexCountAndAttributes(dls);
        ByteBufferList.releaseDataListSet(dls);
    }

    public void copyAttr(de.jreality.scene.IndexedLineSet src,
            RemoteIndexedLineSet dst) {
        copyAttr((de.jreality.scene.PointSet) src, (RemotePointSet) dst);
        DataListSet dls = ByteBufferList.prepareDataListSet(src.getEdgeAttributes());
        dst.setEdgeCountAndAttributes(dls);
        ByteBufferList.releaseDataListSet(dls);
    }

    public void copyAttr(de.jreality.scene.IndexedFaceSet src,
            RemoteIndexedFaceSet dst) {
        copyAttr((de.jreality.scene.IndexedLineSet) src,
                (RemoteIndexedLineSet) dst);
        DataListSet dls = ByteBufferList.prepareDataListSet(src.getFaceAttributes());
        dst.setFaceCountAndAttributes(dls);
        ByteBufferList.releaseDataListSet(dls);
    }

    public void copyAttr(de.jreality.scene.Camera src, RemoteCamera dst) {
        copyAttr((de.jreality.scene.SceneGraphNode) src,
                (RemoteSceneGraphNode) dst);
//        dst.setAspectRatio(src.getAspectRatio());
        dst.setEyeSeparation(src.getEyeSeparation());
        dst.setFar(src.getFar());
        dst.setFieldOfView(src.getFieldOfView());
        dst.setFocus(src.getFocus());
        dst.setNear(src.getNear());
        dst.setOnAxis(src.isOnAxis());
        dst.setOrientationMatrix(src.getOrientationMatrix());
        dst.setPerspective(src.isPerspective());
//        dst.setMetric(src.getMetric());
        dst.setStereo(src.isStereo());
        if (src.getViewPort() != null)
          dst.setViewPort(src.getViewPort().getX(), src.getViewPort().getY(),
            src.getViewPort().getWidth(), src.getViewPort().getHeight());
    }

}
