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

import java.util.ArrayList;
import java.util.Collections;
import java.util.IdentityHashMap;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.ListIterator;
import java.util.logging.Level;

import de.jreality.scene.Appearance;
import de.jreality.scene.AudioSource;
import de.jreality.scene.Camera;
import de.jreality.scene.Geometry;
import de.jreality.scene.Light;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphNode;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.Transformation;
import de.jreality.util.LoggingSystem;


/**
 * This class represents a tree version of a SceneGraphNode.
 * 
 * Such a proxy class exists for each unique path to a SceneGraphNode.
 * 
 * Typical application would be calculating a BoundingBox in
 * world coordinates - in contrast to the BoundingBox in local
 * coordinates, which would typically belong to the corresponding
 * Entity. 
 *
 * @author weissman
 *
 */
public class SceneTreeNode {

	private SceneTreeNode parent=null;
	private SceneGraphNode node;
	private IdentityHashMap<SceneGraphNode, SceneTreeNode> childrenMap = new IdentityHashMap<SceneGraphNode, SceneTreeNode>();
	private List<SceneTreeNode> childList=Collections.emptyList();
	private List<SceneTreeNode> childrenRO=Collections.unmodifiableList(childList);
	private ProxyConnector connector;
	private Object proxy;
	private SceneGraphNodeEntity entity;

	protected final boolean isComponent;
	private boolean hasTrafo;
	private boolean hasApp;
	private boolean hasCam;
	private boolean hasLight;
	private boolean hasGeom;
	private boolean hasAudio;
	

	protected SceneTreeNode(SceneGraphNode node) {
		this.node = node;
		isComponent = node instanceof SceneGraphComponent;
		if (isComponent) {
			childList = new ArrayList<SceneTreeNode>(((SceneGraphComponent)node).getChildComponentCount()+5);
			childrenRO = Collections.unmodifiableList(childList);
		}
	}

	private void setParent(SceneTreeNode parent) {
		if (this.parent != null) throw new IllegalStateException("parent already set!");
		this.parent = parent;
	}

	public boolean isLeaf() {
		return childrenMap.isEmpty();
	}

	public List<SceneTreeNode> getChildren() {
		return childrenRO;
	}

	public SceneGraphNode getNode() {
		return node;
	}
	
	public SceneTreeNode getParent() {
		return parent;
	}
	
	public Object getProxy() {
		return proxy;
	}
	
	public void setProxy(Object proxy) {
		this.proxy = proxy;
	}
	
	SceneTreeNode findNodeForPath(Iterator i) {
		if (!i.hasNext()) return this;
		Object nextSGN = i.next();
		if (!isComponent || !childrenMap.containsKey(nextSGN)) throw new IllegalStateException("path doesn't match!");
		return ((SceneTreeNode) childrenMap.get(nextSGN)).findNodeForPath(i);
	}
	
	public int addChild(SceneTreeNode child) {
		int idx = 0;
		childrenMap.put(child.getNode(), child);
		child.setParent(this);

		//computeChildren();
		if (child.getNode() instanceof SceneGraphComponent) {
			childList.add(child);
			idx=childList.size()-1;
		} else {
			SceneGraphNode ch = child.getNode();
			if (ch instanceof Transformation) {
				if (hasTrafo) {
					childList.set(idx, child);
				} else {
					childList.add(idx, child);
				}
				hasTrafo = true;
			} else if (ch instanceof Appearance) {
				if (hasTrafo) idx++;
				if (hasApp) {
					childList.set(idx, child);
				} else {
					childList.add(idx, child);
				}
				hasApp = true;
			} else if (ch instanceof Camera) {
				if (hasTrafo) idx++;
				if (hasApp) idx++;
				if (hasCam) {
					childList.set(idx, child);
				} else {
					childList.add(idx, child);
				}
				hasCam = true;
			} else if (ch instanceof Light) {
				if (hasTrafo) idx++;
				if (hasApp) idx++;
				if (hasCam) idx++;
				if (hasLight) {
					childList.set(idx, child);
				} else {
					childList.add(idx, child);
				}
				hasLight = true;
			} else if (ch instanceof Geometry) {
				if (hasTrafo) idx++;
				if (hasApp) idx++;        
				if (hasCam) idx++;
				if (hasLight) idx++;
				if (hasGeom) {
					childList.set(idx, child);
				} else {
					childList.add(idx, child);
				}
				hasGeom = true;
			} else if (ch instanceof AudioSource) {
				if (hasTrafo) idx++;
				if (hasApp) idx++;        
				if (hasCam) idx++;
				if (hasLight) idx++;
				if (hasGeom) idx++;
				if (hasAudio) {
					childList.set(idx, child);
				} else {
					childList.add(idx, child);
				}
				hasAudio = true;
			}
		}
		connector.add(getProxy(), child.getProxy());
		return idx;
	}
	
	public SceneGraphPath toPath() {
		// fill list in reverse order
		LinkedList<SceneGraphNode> ll = new LinkedList<SceneGraphNode>();
		ll.add(this.getNode());
		for (SceneTreeNode n = this; n.getParent()!= null; n = n.getParent())
			ll.add(n.getParent().getNode());
		// fill arraylist in correct oder
		ArrayList<SceneGraphNode> al = new ArrayList<SceneGraphNode>(ll.size());
		ListIterator<SceneGraphNode> litar = ll.listIterator(ll.size());
		for (int i = 0; litar.hasPrevious(); i++) {
			al.add(i, litar.previous());
		}
		return SceneGraphPath.fromList(al);
	}
	
	void setConnector(ProxyConnector connector) {
		this.connector = connector;
	}
	
	public SceneGraphNodeEntity getEntity() {
		return entity;
	}
	
	void setEntity(SceneGraphNodeEntity entity) {
		this.entity = entity;
	}
	
	SceneTreeNode removeChildForNode(SceneGraphNode prevChild) {
		if (!childrenMap.containsKey(prevChild)) throw new IllegalStateException("unknown child!");
		SceneTreeNode ret = getTreeNodeForChild(prevChild);
		removeChild(ret);
		if (prevChild instanceof Transformation) hasTrafo = false;
		if (prevChild instanceof Appearance) hasApp = false;
		if (prevChild instanceof Camera) hasCam = false;
		if (prevChild instanceof Light) hasLight = false;
		if (prevChild instanceof Geometry) hasGeom = false;
		if (prevChild instanceof AudioSource) hasAudio = false;
		return ret;
	}
	
	protected int removeChild(SceneTreeNode prevChild) {
		int ret = childList.indexOf(prevChild);
		SceneTreeNode fromMap = childrenMap.remove(prevChild.getNode());
		SceneTreeNode fromList = childList.remove(ret);
		assert (fromMap == prevChild && fromList == prevChild);
		return ret;
	}

	public SceneTreeNode getTreeNodeForChild(SceneGraphNode prevChild) {
		SceneTreeNode ret = (SceneTreeNode) childrenMap.get(prevChild);
		return ret;
	}
	
	public SceneTreeNode getTransformationTreeNode() {
		if (!isComponent) throw new UnsupportedOperationException("no component");
		return getTreeNodeForChild(((SceneGraphComponent)node).getTransformation());
	}
	
	public SceneTreeNode getAppearanceTreeNode() {
		if (!isComponent) throw new UnsupportedOperationException("no component");
		return getTreeNodeForChild(((SceneGraphComponent)node).getAppearance());
	}
	
	public SceneTreeNode getGeometryTreeNode() {
		if (!isComponent) throw new UnsupportedOperationException("no component");
		return getTreeNodeForChild(((SceneGraphComponent)node).getGeometry());
	}
	public SceneTreeNode getLightTreeNode() {
		if (!isComponent) throw new UnsupportedOperationException("no component");
		return getTreeNodeForChild(((SceneGraphComponent)node).getLight());
	}

	/**
	 * disposes the whole tree from this node on
	 * works recursively. also disposes the entity
	 * if it is impty
	 * 
	 * @param disposedEntities a list where to put in all empty entities
	 */
	protected void dispose(ArrayList<SceneGraphNodeEntity> disposedEntities) {
		for (SceneTreeNode node : getChildren())
			node.dispose(disposedEntities);
		
		int prevSize=getEntity().size();
		getEntity().removeTreeNode(this);
		LoggingSystem.getLogger(this).log(Level.FINE, "entity size: prev="+prevSize+" new="+getEntity().size());
		if (getEntity().isEmpty()) {
			disposedEntities.add(getEntity());
		}
	}

	public boolean isComponent() {
		return isComponent;
	}

}