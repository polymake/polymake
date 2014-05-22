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


package de.jreality.toolsystem;

import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphNode;
import de.jreality.scene.event.ToolEvent;
import de.jreality.scene.event.ToolListener;
import de.jreality.scene.proxy.tree.EntityFactory;
import de.jreality.scene.proxy.tree.SceneGraphNodeEntity;
import de.jreality.scene.proxy.tree.SceneTreeNode;
import de.jreality.scene.proxy.tree.UpToDateSceneProxyBuilder;
import de.jreality.scene.tool.Tool;

public class ToolUpdateProxy {

  private ToolSystem toolSystem;

  private Builder builder;

  public ToolUpdateProxy(ToolSystem ts) {
    toolSystem = ts;
  }

  void dispose() {
    if (builder != null) {
      builder.dispose();
      builder = null;
    }
  }
  
  public void setSceneRoot(SceneGraphComponent comp) {
    if (builder != null) {
      throw new IllegalStateException("twice called");
    }
    if (comp == null) return;
    builder = new Builder(comp);
    builder.createProxyTree();
  }

  private class Builder extends UpToDateSceneProxyBuilder {
    Builder(SceneGraphComponent root) {
      super(root);
      setEntityFactory(new EntityFactory() {
        public SceneGraphNodeEntity produceSceneGraphNodeEntity(
            SceneGraphNode node) {
          if (node instanceof SceneGraphComponent) {
            return new SceneGraphComponentEntity((SceneGraphComponent) node);
          } else
            return super.produceSceneGraphNodeEntity(node);
        }
      });
    }
  }

  private class SceneGraphComponentEntity extends SceneGraphNodeEntity
      implements ToolListener {

    private final SceneGraphComponent comp;

    protected SceneGraphComponentEntity(SceneGraphComponent node) {
      super(node);
      this.comp = node;
      comp.addToolListener(this);
    }

    protected void addTreeNode(SceneTreeNode tn) {
      super.addTreeNode(tn);
      if (toolSystem != null) {
    	  for (Tool tool : comp.getTools()) {
    		  toolSystem.addTool(tool, tn.toPath());
    	  }
      }
    }

    protected void removeTreeNode(SceneTreeNode tn) {
      super.removeTreeNode(tn);
      if (toolSystem != null) {
    	  for (Tool tool : comp.getTools()) {
    		  toolSystem.removeTool(tool, tn.toPath());
    	  }
      }
    }

    public void toolAdded(ToolEvent ev) {
      if (toolSystem != null) {
    	  for (SceneTreeNode node : getTreeNodes()) {
    		  toolSystem.addTool(ev.getTool(), node.toPath());
    	  }
      }
    }

    public void toolRemoved(ToolEvent ev) {
    	if (toolSystem != null) {
    		for (SceneTreeNode node : getTreeNodes()) {
    			toolSystem.removeTool(ev.getTool(), node.toPath());
    		}
    	}
    }

    protected void dispose() {
      comp.removeToolListener(this);
      super.dispose();
    }

  }

}