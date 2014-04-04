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
import java.util.Collection;
import java.util.Collections;
import java.util.List;
import java.util.logging.Level;

import de.jreality.scene.SceneGraphNode;
import de.jreality.util.LoggingSystem;


/**
 *
 * THis class contains several tree nodes for the covered node.
 * It is intended to be used for synchronizing a proxy tree.
 * 
 * Implement the update mechanism for geometry/apperance/transformation
 * changes here - and update the SceneTreeNodes as needed.
 * 
 * So you need to calculate the local data for the proxies
 * only once, instead of converting a jreality event in every copy instance.
 * 
 * A typical application would be a transformation change,
 * so one calculates the needed proxy matrix/quaternion representation
 * only once and sets it for all proxies then by iterating the TreeNodes.
 * 
 * <b>Note: subclasses need to implement Geometry/Transformation/Appearance-Listener
 * if the EntityFactory is set to update on these events.
 * </b>
 * 
 * @author weissman
 *
 */
public class SceneGraphNodeEntity {
  
  private List<SceneTreeNode> treeNodes = new ArrayList<SceneTreeNode>();
  private SceneGraphNode node;
  
  protected SceneGraphNodeEntity(SceneGraphNode node) {
    this.node = node;
  }
  
  protected void addTreeNode(SceneTreeNode tn) {
    LoggingSystem.getLogger(this).log(Level.FINE,
        "adding new TreeNode[{1}] for {0}", new Object[]{node.getName(), new Integer(treeNodes.size())});
    if (tn == null) throw new IllegalStateException("tree node is null");
    treeNodes.add(tn);
    tn.setEntity(this);
  }
  
  protected void removeTreeNode(SceneTreeNode tn) {
  	LoggingSystem.getLogger(this).log(Level.FINE,
        "removing TreeNode for {0}", node.getName());
    if (tn == null) throw new IllegalStateException("tree node is null");
    treeNodes.remove(tn);
  }
  
  public Collection<SceneTreeNode> getTreeNodes() {
    return Collections.unmodifiableCollection(treeNodes);
  }
  
  boolean isEmpty() {
    return treeNodes.isEmpty();
  }
  
  int size() {
    return treeNodes.size();
  }

  protected void dispose() {
    treeNodes.clear();
    node = null;
  }
  
  public SceneGraphNode getNode() {
    return node;
  }
}
