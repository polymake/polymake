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


package de.jreality.scene.proxy.tree;

import java.util.WeakHashMap;

import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphNode;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.SceneGraphVisitor;

/**
 * class for generating proxy trees.
 * 
 * Limitation: this class must have a fixed root for all operations.
 * 
 * @author weissman
 *  
 */
public class SceneProxyTreeBuilder {

  protected SceneGraphComponent root;
  protected SceneTreeNode treeRoot;
  
  protected WeakHashMap proxyToTreeNodes = new WeakHashMap();
  
  protected ProxyTreeFactory proxyTreeFactory=new ProxyTreeFactory();
  private ProxyConnector connector=new ProxyConnector();
  
  public SceneProxyTreeBuilder(SceneGraphComponent root) {
    this.root = root;
  }

  Traversal traversal = new Traversal();
  protected class Traversal extends SceneGraphVisitor {

    SceneTreeNode proxy, proxyParent;
    SceneGraphPath path;
    
    // assume that proxyParent is null or contains
    // a tree node for the parent component of c
    public void visit(SceneGraphComponent c) {
      super.visit(c); // sets proxy to created proxy for the component
                      // and adds it to proxyParent as child
      if (treeRoot == null) treeRoot = proxy;
      SceneTreeNode oldParent = proxyParent;
      proxyParent = proxy;
      c.childrenAccept(this);
      proxy = proxyParent;
      proxyParent = oldParent;
    }

    /**
     * create a tree proxy node
     * for m (which includes a proxy already)
     */
    public void visit(SceneGraphNode m) {
      proxy=proxyTreeFactory.createProxyTreeNode(m);
      // add(proxyParent, proxy); this happens here in the tree
      // as we remove objects there too
      proxy.setConnector(connector);
      if (proxyParent != null) proxyParent.addChild(proxy);
      proxyToTreeNodes.put(proxy.getProxy(), proxy);
      postCreate(proxy);
    }
    
    void setRoot(SceneTreeNode root) {
      proxyParent = root;
    }
  };
  
  protected void postCreate(SceneTreeNode proxy) {
    // handle the scene tree node after creation
    // attatch listeners etc...
  }

  public SceneTreeNode createProxyTree() {
    if (proxyTreeFactory == null) throw new IllegalStateException("tree proxy factory not set!");
    if (treeRoot == null) traversal.visit(root);
    else throw new IllegalStateException("proxy tree already created");
    return treeRoot;
  }
  
  /**
   * traverses the tree along the given path
   * and returns the assigned proxy element
   */
  public Object getProxy(SceneGraphPath path) {
    SceneTreeNode n = treeRoot.findNodeForPath(path.iterator());
    return n.getProxy();
  }
  
  /**
   * proxies are stored in a WeakHashMap - so they need
   *  to have a working equals-method
   * 
   * @return the representing tree element
   */
  public SceneTreeNode getTreeNodeForProxy(Object proxy) {
    if (proxy == null) throw new IllegalStateException("proxy is null");
    if (!proxyToTreeNodes.containsKey(proxy)) throw new IllegalStateException("unknown proxy");
    return (SceneTreeNode) proxyToTreeNodes.get(proxy);
  }
  
  public SceneTreeNode getTreeRoot() {
    return treeRoot;
  }
  public ProxyTreeFactory getProxyTreeFactory() {
    return proxyTreeFactory;
  }
  public void setProxyTreeFactory(ProxyTreeFactory proxyTreeFactory) {
    this.proxyTreeFactory = proxyTreeFactory;
  }
  public ProxyConnector getConnector() {
    return connector;
  }
  public void setProxyConnector(ProxyConnector connector) {
    this.connector = connector;
  }
}
