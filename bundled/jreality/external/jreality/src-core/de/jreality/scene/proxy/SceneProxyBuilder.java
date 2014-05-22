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


package de.jreality.scene.proxy;

import java.util.ArrayList;
import java.util.IdentityHashMap;
import java.util.List;

import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphNode;
import de.jreality.scene.SceneGraphVisitor;

/**
 * Base class for builder that create some kind of mirroring structure
 * for a scene graph. This class does not make copies for each path
 * that reaches a node.
 * 
 */
public class SceneProxyBuilder {
    protected IdentityHashMap proxies = new IdentityHashMap();
    ProxyFactory proxyFactory;

    class Traversal extends SceneGraphVisitor {
        Object proxy, proxyParent;
        public void visit(SceneGraphComponent c) {
            boolean stopTraversing = proxies.containsKey(c);
        	super.visit(c);
        	if (stopTraversing) return;
            Object old=proxyParent;
            proxyParent=proxy;
            c.childrenAccept(this);
            proxy=proxyParent;
            proxyParent=old;
        }
        public void visit(SceneGraphNode m) {
            proxy=SceneProxyBuilder.this.getProxyImpl(m);
            add(proxyParent, proxy);
        }
    };
    Traversal traversal=new Traversal();

    protected SceneProxyBuilder() {}

    public ProxyFactory getProxyFactory() {
        return proxyFactory;
    }

    public void setProxyFactory(ProxyFactory factory) {
        proxyFactory=factory;
    }

    protected Object getProxyImpl(SceneGraphNode target) {
        Object proxy=proxies.get(target);
        if (proxy == null) {
            target.accept(proxyFactory);
            proxy=proxyFactory.getProxy();
            proxies.put(target, proxy);
        }
        return proxy;
    }

    public void add(Object parentProxy, Object childProxy) {}

    public Object createProxyScene(SceneGraphNode node) {
        if (proxyFactory == null)
            throw new IllegalStateException("no proxy factory set");
        boolean traverse=(!proxies.containsKey(node))&&(node instanceof SceneGraphComponent);
        Object proxy=getProxyImpl(node);
        if(traverse) {
            traversal.proxyParent=proxy;
            ((SceneGraphComponent)node).childrenAccept(traversal);
        }
        return proxy;
    }
    
    public Object getProxy(Object local) {
    	return proxies.get(local);
    }
    public List getProxies(List l) {
    	List ret = new ArrayList(l.size());
    	for (int i = 0; i < l.size(); i++) {
    		ret.add(proxies.get(l.get(i)));
    	}
    	System.out.println("[SceneProxyBuilder] converted "+ret.size()+" proxies.");
    	return ret;
    }

    public void disposeProxy(SceneGraphComponent root) {
      root.accept(new DisposeTraversal());
    }
    
    class DisposeTraversal extends SceneGraphVisitor {
      Object proxy, proxyParent;
      public void visit(SceneGraphComponent c) {
          boolean stopTraversing = proxies.containsKey(c);
        super.visit(c);
        if (stopTraversing) return;
          Object old=proxyParent;
          proxyParent=proxy;
          c.childrenAccept(this);
          proxy=proxyParent;
          proxyParent=old;
      }
      public void visit(SceneGraphNode m) {
        if (proxies.containsKey(m)) disposeProxyImpl(m);
      }
  }

    protected void disposeProxyImpl(SceneGraphNode target) {
      proxies.remove(target);
    }

}
