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
import java.util.HashSet;
import java.util.IdentityHashMap;
import java.util.logging.Level;

import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphNode;
import de.jreality.scene.SceneGraphVisitor;
import de.jreality.scene.event.SceneGraphComponentEvent;
import de.jreality.scene.event.SceneGraphComponentListener;
import de.jreality.util.LoggingSystem;


/**
 *
 * This class creates a proxy tree for the given sceneGraph from the root node
 * and cares about adding and removing of objects.
 * 
 * Updating the proxy tree for other changes is handled differently,
 * as there is no need for the factory mechanism then. just forward the
 * events to the implemetation
 *
 *
 * TODO: thread issues!!
 * 
 * @author weissman
 *
 */
public class UpToDateSceneProxyBuilder extends SceneProxyTreeBuilder implements SceneGraphComponentListener {

	private final Object mutex = new Object();

	private IdentityHashMap<SceneGraphNode, SceneGraphNodeEntity> nodeEntityMap = new IdentityHashMap<SceneGraphNode, SceneGraphNodeEntity>();

	private EntityFactory entityFactory;

	protected final Level loglevel = Level.FINE;		// somebody set this to INFO, which generates just too much -gunn

	public UpToDateSceneProxyBuilder(SceneGraphComponent root) {
		super(root);
	}

	protected SceneGraphVisitor attatchListeners = new SceneGraphVisitor() {
		public void visit(SceneGraphComponent c) {
			c.addSceneGraphComponentListener(UpToDateSceneProxyBuilder.this);
		}
	};

	protected SceneGraphVisitor detatchListeners = new SceneGraphVisitor() {
		public void visit(SceneGraphComponent c) {
			c.removeSceneGraphComponentListener(UpToDateSceneProxyBuilder.this);
		}
	};

	/**
	 * TODO: synchronize and signal error when called twice?
	 */
	public SceneTreeNode createProxyTree() {
		if (entityFactory == null)
			entityFactory = new EntityFactory();
		return super.createProxyTree();
	}

	/**
	 * registers this class as container listener for each component.
	 * so new childs are automatically added to the proxy tree
	 * and removed childs get removed from it
	 */
	protected void postCreate(SceneTreeNode proxy2) {
		SceneGraphNode node = proxy2.getNode();
		SceneGraphNodeEntity sge = (SceneGraphNodeEntity) nodeEntityMap.get(node);
		if (sge == null) {
			sge = entityFactory.createEntity(node);
			nodeEntityMap.put(node, sge);
			node.accept(attatchListeners);
			LoggingSystem.getLogger(this).log(loglevel, 
					"adding entity+listener for {0}", node.getName());
		}
		sge.addTreeNode(proxy2);
	}

	public void childAdded(SceneGraphComponentEvent ev) {
		synchronized (mutex) {
			SceneGraphComponent parent = (SceneGraphComponent) ev.getSceneGraphComponent();
			SceneGraphNode newChild = (SceneGraphNode) ev.getNewChildElement();
			LoggingSystem.getLogger(this).log(loglevel, 
					"handling add event: {0} added to {1} [ {2} ]", new Object[]{newChild.getName(), parent.getName(), ev.getSourceNode().getName()});
			SceneGraphNodeEntity sge = (SceneGraphNodeEntity) nodeEntityMap.get(parent);
			if (sge == null) {
				LoggingSystem.getLogger(this).warning("entity for registered component is null -> was disposed by other thread...");
				// maybe check if this class is no longer a listener
				return;
			}
			if (sge.isEmpty()) throw new IllegalStateException("empty entity node");
			// iterate parent tree nodes, assign it as the traversal's
			// parent and visit the new child -
			// this results in attatching the whole 
			// tree below the new child to each tree node
			// of the parent entity
			for (SceneTreeNode node : sge.getTreeNodes()) {
				traversal.proxyParent = node;
				LoggingSystem.getLogger(this).log(loglevel, 
						"attatching child {0} to {1}", new Object[]{newChild.getName(), parent.getName()});
				ev.getNewChildElement().accept(traversal);
			}
		}
	}

	public void childRemoved(SceneGraphComponentEvent ev) {
		synchronized (mutex) {
			SceneGraphComponent parent = (SceneGraphComponent) ev.getSceneGraphComponent();
			SceneGraphNode prevChild = (SceneGraphNode) ev.getOldChildElement();
			SceneGraphNodeEntity parentEntity = (SceneGraphNodeEntity) nodeEntityMap.get(parent);
			LoggingSystem.getLogger(this).log(loglevel, 
					"handling remove event: {0} removed from {1}", new Object[]{prevChild.getName(), parent.getName()});
			if (parentEntity == null) {
				throw new Error("event from unknown container");
			}
			// remove child from all parent tree nodes
			ArrayList<SceneGraphNodeEntity> disposedEntities = new ArrayList<SceneGraphNodeEntity>();
			for (SceneTreeNode node : parentEntity.getTreeNodes()) {
				SceneTreeNode deleted = node.removeChildForNode(prevChild);
				deleted.dispose(disposedEntities);
			}
			for (SceneGraphNodeEntity entity : disposedEntities)
				disposeEntity(entity, true);
		}
	}

	/**
	 * move the listener from this class to the entity itself?
	 * NO: so we can synchronize all changes in this class!
	 * @param entity
	 */
	private void disposeEntity(SceneGraphNodeEntity entity, boolean assertEmpty) {
		if (assertEmpty && !entity.isEmpty()) throw new IllegalStateException("not empty");
		LoggingSystem.getLogger(this).log(loglevel, 
				"disposing entity+listener for removed child {0}", new Object[]{entity.getNode().getName()});
		nodeEntityMap.remove(entity.getNode());
		entity.getNode().accept(detatchListeners);
		entityFactory.disposeEntity(entity);
	}

	public void childReplaced(SceneGraphComponentEvent ev) {
		synchronized (mutex) {
			childRemoved(ev);
			childAdded(ev);
		}
	}

	public EntityFactory getEntityFactory() {
		return entityFactory;
	}
	public void setEntityFactory(EntityFactory ef) {
		if (treeRoot != null) throw new IllegalStateException("can't change policy after proxy creation");
		this.entityFactory = ef;
	}
	public void visibilityChanged(SceneGraphComponentEvent ev) {
	}

	public void dispose() {
		SceneGraphVisitor disposeVisitor = new SceneGraphVisitor() {
			final HashSet<SceneGraphNode> encountered=new HashSet<SceneGraphNode>();
			public void visit(SceneGraphNode n) {
				if (encountered.add(n)) dispose(n);
			};
			public void visit(SceneGraphComponent c) {
				if (encountered.add(c)) {
					dispose(c);
					c.childrenAccept(this);
				}
			}
			private void dispose(SceneGraphNode n) {
				SceneGraphNodeEntity entity = (SceneGraphNodeEntity) nodeEntityMap.get(n);
				disposeEntity(entity, false);
			}
		};
		root.accept(disposeVisitor);
	}
}
