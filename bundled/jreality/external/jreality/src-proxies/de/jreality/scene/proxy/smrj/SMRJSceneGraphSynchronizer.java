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

import java.io.Serializable;
import java.util.Iterator;

import de.jreality.scene.Appearance;
import de.jreality.scene.Camera;
import de.jreality.scene.Geometry;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.IndexedLineSet;
import de.jreality.scene.Light;
import de.jreality.scene.PointLight;
import de.jreality.scene.PointSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphVisitor;
import de.jreality.scene.SpotLight;
import de.jreality.scene.Transformation;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.ByteBufferList;
import de.jreality.scene.data.DataList;
import de.jreality.scene.event.AppearanceEvent;
import de.jreality.scene.event.AppearanceListener;
import de.jreality.scene.event.CameraEvent;
import de.jreality.scene.event.CameraListener;
import de.jreality.scene.event.GeometryEvent;
import de.jreality.scene.event.GeometryListener;
import de.jreality.scene.event.LightEvent;
import de.jreality.scene.event.LightListener;
import de.jreality.scene.event.SceneGraphComponentEvent;
import de.jreality.scene.event.SceneGraphComponentListener;
import de.jreality.scene.event.TransformationEvent;
import de.jreality.scene.event.TransformationListener;
import de.jreality.scene.proxy.scene.RemoteAppearance;
import de.jreality.scene.proxy.scene.RemoteCamera;
import de.jreality.scene.proxy.scene.RemoteGeometry;
import de.jreality.scene.proxy.scene.RemoteIndexedFaceSet;
import de.jreality.scene.proxy.scene.RemoteIndexedLineSet;
import de.jreality.scene.proxy.scene.RemoteLight;
import de.jreality.scene.proxy.scene.RemotePointLight;
import de.jreality.scene.proxy.scene.RemotePointSet;
import de.jreality.scene.proxy.scene.RemoteSceneGraphComponent;
import de.jreality.scene.proxy.scene.RemoteSceneGraphNode;
import de.jreality.scene.proxy.scene.RemoteSpotLight;
import de.jreality.scene.proxy.scene.RemoteTransformation;

/**
 * 
 * This class registers itself to all nodes for keeping the remote scenegraph up-to-date.
 * 
 * TODO: implement this as 1-1 in the factory
 * 
 * @author weissman
 */
public class SMRJSceneGraphSynchronizer extends SceneGraphVisitor implements TransformationListener, AppearanceListener, GeometryListener, SceneGraphComponentListener, CameraListener, LightListener {
		
	SMRJMirrorScene rmc;
	SMRJMirrorFactory factory;

	public SMRJSceneGraphSynchronizer(SMRJMirrorScene rmc) {
		this.rmc = rmc;
		factory = (SMRJMirrorFactory) rmc.getProxyFactory();
	}
	
  boolean detatch=false;
  
  public void visit(Camera c) {
    if (detatch) c.removeCameraListener(this);
    else c.addCameraListener(this);
  }
  
  public void visit(Light l) {
    if (detatch) l.removeLightListener(this);
    else l.addLightListener(this);
  }
  
	public void visit(final Transformation t) {
    if (detatch) t.removeTransformationListener(this);
    else t.addTransformationListener(this);
	}

	public void visit(final Appearance a) {
    if (detatch) a.removeAppearanceListener(this);
    else a.addAppearanceListener(this);
	}

	public void visit(final Geometry g) {
		if (detatch) g.removeGeometryListener(this);
    else g.addGeometryListener(this);
	}
	
	public void visit(SceneGraphComponent sg) {
    if (detatch) sg.removeSceneGraphComponentListener(this);
    else sg.addSceneGraphComponentListener(this);
	}
	
	public void transformationMatrixChanged(TransformationEvent ev) {
	rmc.writeLock.writeLock();
   	((RemoteTransformation)rmc.getProxy(ev.getSourceNode())).setMatrix(ev.getTransformationMatrix());
   	rmc.writeLock.writeUnlock();
	}

	public void appearanceChanged(AppearanceEvent ev) {
    Appearance src = (Appearance) ev.getSourceNode();
    RemoteAppearance dst = (RemoteAppearance) rmc.getProxy(src);
    Object aa = src.getAttribute(ev.getKey());
    rmc.writeLock.writeLock();
    if (aa == Appearance.INHERITED) aa = de.jreality.scene.proxy.scene.Appearance.INHERITED;
    if (aa == Appearance.DEFAULT) aa = de.jreality.scene.proxy.scene.Appearance.DEFAULT;
    try {
    	dst.setAttribute(ev.getKey(), aa);
    } catch (Exception nse) {
    	System.out.println("Object in appearance not serializable: "+ev.getKey()+"  "+aa);
    }
    rmc.writeLock.writeUnlock();
  }

	public void geometryChanged(GeometryEvent ev) {
        Geometry src = ev.getGeometry();
        RemoteGeometry dst = (RemoteGeometry) rmc.getProxy(src);
        for (Iterator i = ev.getChangedFaceAttributes().iterator(); i.hasNext();) {
            Attribute a = (Attribute) i.next();
            DataList dl = ((IndexedFaceSet) src).getFaceAttributes(a);
            if (ByteBufferList.canCopy(dl)) {
            	ByteBufferList copy = ByteBufferList.createByteBufferCopy(dl);
                rmc.writeLock.writeLock();
                ((RemoteIndexedFaceSet) dst).setFaceAttributes(a, copy);
                rmc.writeLock.writeUnlock();
                ByteBufferList.releaseList(copy);
            } else {
                rmc.writeLock.writeLock();
                ((RemoteIndexedFaceSet) dst).setFaceAttributes(a, dl);
                rmc.writeLock.writeUnlock();
            }            
        }
        for (Iterator i = ev.getChangedEdgeAttributes().iterator(); i.hasNext();) {
            Attribute a = (Attribute) i.next();
            DataList dl = ((IndexedLineSet) src).getEdgeAttributes(a);
            if (ByteBufferList.canCopy(dl)) {
            	ByteBufferList copy = ByteBufferList.createByteBufferCopy(dl);
                rmc.writeLock.writeLock();
                ((RemoteIndexedLineSet) dst).setEdgeAttributes(a, copy);
                rmc.writeLock.writeUnlock();
                ByteBufferList.releaseList(copy);
            } else {
                rmc.writeLock.writeLock();
                ((RemoteIndexedLineSet) dst).setEdgeAttributes(a, dl);
                rmc.writeLock.writeUnlock();
            }
        }
        for (Iterator i = ev.getChangedVertexAttributes().iterator(); i
                .hasNext();) {
            Attribute a = (Attribute) i.next();
            if (a.getName().equals("rungeKuttaData")) {
            	System.out.println("transfering RK data!");
            }
            DataList dl = ((PointSet) src).getVertexAttributes(a);
            if (ByteBufferList.canCopy(dl)) {
            	ByteBufferList copy = ByteBufferList.createByteBufferCopy(dl);
                rmc.writeLock.writeLock();
                ((RemotePointSet) dst).setVertexAttributes(a, copy);
                rmc.writeLock.writeUnlock();
                ByteBufferList.releaseList(copy);
            } else {
                rmc.writeLock.writeLock();
                ((RemotePointSet) dst).setVertexAttributes(a, dl);
                rmc.writeLock.writeUnlock();
            }
        }
        for (String key : ev.getChangedGeometryAttributes()) {
            if (src.getGeometryAttributes(key) instanceof Serializable) {
              rmc.writeLock.writeLock();
              dst.setGeometryAttributes(key, src.getGeometryAttributes(key));
              rmc.writeLock.writeUnlock();
            }
        }
    }

		public void childAdded(SceneGraphComponentEvent ev) {
   		    rmc.writeLock.writeLock();
   			RemoteSceneGraphNode createProxyScene = (RemoteSceneGraphNode) rmc.createProxyScene(ev.getNewChildElement());
			((RemoteSceneGraphComponent)rmc.getProxyImpl(ev.getSceneGraphComponent())).add(createProxyScene);
		    rmc.writeLock.writeUnlock();
	}

	public void childRemoved(SceneGraphComponentEvent ev) {
	    rmc.writeLock.writeLock();
        ((RemoteSceneGraphComponent)rmc.getProxyImpl(ev.getSceneGraphComponent()))
        .remove((RemoteSceneGraphNode) rmc.getProxyImpl(ev.getOldChildElement()));
        rmc.writeLock.writeUnlock();
	}

	public void childReplaced(SceneGraphComponentEvent ev) {
		childRemoved(ev); childAdded(ev);
	}

  public void visibilityChanged(SceneGraphComponentEvent ev) {
    rmc.writeLock.writeLock();
    ((RemoteSceneGraphComponent)rmc.getProxyImpl(ev.getSceneGraphComponent())).setVisible(ev.getSceneGraphComponent().isVisible());
    rmc.writeLock.writeUnlock();
  }

  public void cameraChanged(CameraEvent ev) {
    Camera src = ev.getCamera();
    RemoteCamera dst = (RemoteCamera) rmc.getProxyImpl(ev.getCamera());
    rmc.writeLock.writeLock();
    dst.setEyeSeparation(src.getEyeSeparation());
    dst.setFar(src.getFar());
    dst.setFieldOfView(src.getFieldOfView());
    dst.setFocus(src.getFocus());
    dst.setNear(src.getNear());
    dst.setOnAxis(src.isOnAxis());
    dst.setOrientationMatrix(src.getOrientationMatrix());
    dst.setPerspective(src.isPerspective());
//    dst.setMetric(src.getMetric());
    dst.setStereo(src.isStereo());
    if (src.getViewPort() != null)
      dst.setViewPort(src.getViewPort().getX(), src.getViewPort().getY(),
        src.getViewPort().getWidth(), src.getViewPort().getHeight());
    rmc.writeLock.writeUnlock();
  }

  public void lightChanged(LightEvent ev) {
    Light src = ev.getLight();
    RemoteLight dst = (RemoteLight) rmc.getProxyImpl(src);
    rmc.writeLock.writeLock();
    if (src instanceof SpotLight) {
      SpotLight src1 = (SpotLight) src;
      RemoteSpotLight dst1 = (RemoteSpotLight) dst;
      dst1.setConeAngle(src1.getConeAngle());
      dst1.setConeDeltaAngle(src1.getConeDeltaAngle());
      dst1.setDistribution(src1.getDistribution());
    }
    if (src instanceof PointLight) {
      PointLight src1 = (PointLight) src;
      RemotePointLight dst1 = (RemotePointLight) dst;
      dst1.setFalloffA0(src1.getFalloffA0());
      dst1.setFalloffA1(src1.getFalloffA1());
      dst1.setFalloffA2(src1.getFalloffA2());
      dst1.setUseShadowMap(src1.isUseShadowMap());
      dst1.setShadowMapX(src1.getShadowMapX());
      dst1.setShadowMapY(src1.getShadowMapY());
      dst1.setShadowMap(src1.getShadowMap());
    }
    dst.setColor(src.getColor());
    dst.setIntensity(src.getIntensity());
    rmc.writeLock.writeUnlock();
  }
  
}
