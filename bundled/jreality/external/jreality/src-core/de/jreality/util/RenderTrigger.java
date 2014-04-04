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

import de.jreality.scene.Appearance;
import de.jreality.scene.Camera;
import de.jreality.scene.Geometry;
import de.jreality.scene.Light;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphNode;
import de.jreality.scene.SceneGraphVisitor;
import de.jreality.scene.Transformation;
import de.jreality.scene.Viewer;
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

/**
 * RenderTrigger is a class for managing render requests for a scene (or parts of it)
 * One can add subtrees of a scene (SceneGraphComponents and their children) to be watched
 * for changes. On the other side one can add Viewers on which render() will be called
 * for any change that is reported.
 * 
 * If one adds only Viewers and no scene it can be used to simply dispatch render requests
 * to several viewers (via the forceRender() method).
 * @author timh
 *
 * TODO: fix problems maybe use Proxy (remove/add doesn't work how it should)
 */
public class RenderTrigger implements SceneGraphComponentListener,
  TransformationListener, AppearanceListener, GeometryListener, LightListener, CameraListener {

  private boolean collect;
  private boolean async=true;
  
    private RenderTriggerCaster viewer;

    public boolean isAsync() {
      return async;
    }
    
    public void setAsync(boolean async) {
      this.async = async;
    }
    
    public void forceRender() {
        fireRender();
    }
    public void addSceneGraphComponent(SceneGraphComponent c) {
        registerNode(c);
    }
    public void removeSceneGraphComponent(SceneGraphComponent c) {
        unregisterNode(c);
    }
    private void registerNode(SceneGraphNode n) {
        SceneGraphVisitor v =new SceneGraphVisitor() {
            public void visit(SceneGraphComponent c) {
                c.childrenAccept(this);
                c.addSceneGraphComponentListener(RenderTrigger.this);
            }
            public void visit(Appearance a) {
                a.addAppearanceListener(RenderTrigger.this);
            }
            public void visit(Geometry g) {
                g.addGeometryListener(RenderTrigger.this);
            }
            public void visit(Transformation t) {
                t.addTransformationListener(RenderTrigger.this);
            }
            public void visit(Light l) {
              l.addLightListener(RenderTrigger.this);
            }
            public void visit(Camera c) {
                c.addCameraListener(RenderTrigger.this);
              }
        };
        n.accept(v);
    }
    private void unregisterNode(SceneGraphNode n) {
        SceneGraphVisitor v =new SceneGraphVisitor() {
            public void visit(SceneGraphComponent c) {
                c.removeSceneGraphComponentListener(RenderTrigger.this);
                c.childrenAccept(this);
          }
            public void visit(Appearance a) {
                a.removeAppearanceListener(RenderTrigger.this);
            }
            public void visit(Geometry g) {
                g.removeGeometryListener(RenderTrigger.this);
            }
            public void visit(Transformation t) {
                t.removeTransformationListener(RenderTrigger.this);
            }
            public void visit(Light l) {
                l.removeLightListener(RenderTrigger.this);
            }
            public void visit(Camera c) {
                c.removeCameraListener(RenderTrigger.this);
              }
        };
        n.accept(v);
    }
    
    boolean needsRender;
    
    private void fireRender() {
      if (collect) needsRender=true;
      else if(viewer!= null) viewer.render(async);
    }

    public void addViewer(Viewer v) {
        viewer = RenderTriggerCaster.add(viewer,v);
    }
    
    public void removeViewer(Viewer v) {
        viewer =RenderTriggerCaster.remove(viewer,v);
    }
    
    public void childAdded(SceneGraphComponentEvent ev) {
        registerNode(ev.getNewChildElement());
        fireRender();
    }

    public void childRemoved(SceneGraphComponentEvent ev) {
        unregisterNode(ev.getOldChildElement());
        fireRender();
    }

    public void childReplaced(SceneGraphComponentEvent ev) {
        unregisterNode(ev.getOldChildElement());
        registerNode(ev.getNewChildElement());
        fireRender();
    }

    public void visibilityChanged(SceneGraphComponentEvent ev) {
      fireRender();
    }

    public void transformationMatrixChanged(TransformationEvent ev) {
        fireRender();
    }


    public void appearanceChanged(AppearanceEvent ev) {
        fireRender();
    }

    public void geometryChanged(GeometryEvent ev) {
        fireRender();
    }

    static abstract class RenderTriggerCaster
    {
        abstract RenderTriggerCaster remove(Viewer oldl);
        abstract void render(boolean async);
        static RenderTriggerCaster add(RenderTriggerCaster a, Viewer b)
        {
          return add(a, new RenderTriggerSingleCaster(b));
        }
        static RenderTriggerCaster add(RenderTriggerCaster a, RenderTriggerCaster b)
        {
            final RenderTriggerCaster result;
            if(a==null) result=b; else if(b==null) result=a;
            else result=new RenderTriggerMulticaster(a, b);
            return result;
        }
        static RenderTriggerCaster remove(RenderTriggerCaster l, Viewer oldl)
        {
            return (l==null)? null: l.remove(oldl);
        }
    }
    static final class RenderTriggerSingleCaster extends RenderTriggerCaster
    {
        final Viewer v;
        private boolean rendering;
        RenderTriggerSingleCaster(Viewer viewer)
        {
          v=viewer;
        }
        RenderTriggerCaster remove(Viewer oldl)
        {
          return oldl!=v? this: null;
        }
        void render(boolean async)
        {
            if (async) { if (v.canRenderAsync()) v.renderAsync(); }
            else {
                synchronized (this) {
                    if (rendering) return;
                    else rendering=true;
                }
                v.render();
                synchronized (this) {
                    rendering=false;
                }
            }
        }
    }
    static final class RenderTriggerMulticaster extends RenderTriggerCaster
    {
        private final RenderTriggerCaster a, b;
        private RenderTriggerMulticaster(RenderTriggerCaster a, RenderTriggerCaster b) {
            this.a = a; this.b = b;
        }
        RenderTriggerCaster remove(Viewer oldl) {
            RenderTriggerCaster a2 = a.remove(oldl);
            RenderTriggerCaster b2 = b.remove(oldl);
            if(a2 == a && b2 == b) return this;
            return add(a2, b2);
        }
        void render(boolean async)
        {
            a.render(async); b.render(async);
        }
    }
    public void lightChanged(LightEvent ev) {
      // TODO Auto-generated method stub
      
    }
    
    public void startCollect() {
      collect = true;
      needsRender = false;
    }
    
    public void finishCollect() {
      collect = false;
      if (needsRender) {
        needsRender=false;
        fireRender();
      }
    }

    public void cameraChanged(CameraEvent ev) {
        fireRender();
        fireRender();
    }

}
