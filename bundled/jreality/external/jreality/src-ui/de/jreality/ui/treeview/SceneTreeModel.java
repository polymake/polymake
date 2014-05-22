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


package de.jreality.ui.treeview;

import java.beans.BeanInfo;
import java.beans.IntrospectionException;
import java.beans.Introspector;
import java.beans.PropertyDescriptor;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.WeakHashMap;

import de.jreality.scene.Appearance;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphNode;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.data.AttributeEntity;
import de.jreality.scene.data.AttributeEntityUtility;
import de.jreality.scene.event.SceneGraphComponentEvent;
import de.jreality.scene.event.SceneGraphComponentListener;
import de.jreality.scene.event.ToolEvent;
import de.jreality.scene.event.ToolListener;
import de.jreality.scene.proxy.tree.ProxyTreeFactory;
import de.jreality.scene.proxy.tree.SceneGraphNodeEntity;
import de.jreality.scene.proxy.tree.SceneTreeNode;
import de.jreality.scene.proxy.tree.UpToDateSceneProxyBuilder;
import de.jreality.scene.tool.Tool;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.HapticShader;
import de.jreality.shader.RootAppearance;
import de.jreality.shader.ShaderUtility;
import de.jreality.ui.viewerapp.Selection;

public class SceneTreeModel extends AbstractTreeModel {
  
  private UpToDateSceneProxyBuilder builder;
  
  WeakHashMap<Object, Object[]> entities = new WeakHashMap<Object, Object[]>();
  WeakHashMap<Object, Object> parents = new WeakHashMap<Object, Object>();
  
  public SceneTreeModel(SceneGraphComponent root) {
    super(null);
    setSceneRoot(root);
  }
  
  public void dispose() {
    builder.dispose();
    builder = null;
  }
  
  public UpToDateSceneProxyBuilder getBuilder() {
	  return builder;
  }
  
  void setSceneRoot(SceneGraphComponent comp) {
    if (builder != null) {
      throw new IllegalStateException("twice called");
    }
    if (comp == null) return;
    builder = new UpToDateSceneProxyBuilder(comp);
    builder.setProxyTreeFactory(new ProxyTreeFactory() {
      public SceneTreeNode createProxyTreeNode(SceneGraphNode n) {
        return new SceneTreeNodeWithToolListener(n);
      }
      
    });
    super.root = builder.createProxyTree();
  }
  
  public Object getChild(Object parent, int index) {
    if (parent instanceof SceneTreeNode) {
      SceneTreeNode sn = (SceneTreeNode) parent;
      if (sn.getNode() instanceof SceneGraphComponent) {
        if (index < sn.getChildren().size()) return sn.getChildren().get(index);
        int newInd = index-sn.getChildren().size();
        SceneGraphComponent comp = (SceneGraphComponent) sn.getNode();
        Tool t = (Tool) comp.getTools().get(newInd);
        return new TreeTool(sn,t);
      }
    }
    Object[] childEntities = (Object[]) entities.get(parent);
    return childEntities[index];
  }
  
  public int getChildCount(Object parent) {
    if (parent instanceof TreeTool) return 0;
    if (parent instanceof SceneTreeNode) {
      SceneTreeNode sn = (SceneTreeNode)parent;
      if ((sn.getNode() instanceof Appearance)) {
        Object[] ents = (Object[]) entities.get(sn);
        if (ents == null) {
        	LinkedList<AttributeEntity> entitiesList = new LinkedList<AttributeEntity>();
        	entitiesList.add(ShaderUtility.createDefaultGeometryShader((Appearance) sn.getNode(), false));
        	entitiesList.add(ShaderUtility.createDefaultRenderingHintsShader((Appearance) sn.getNode(), false));
        	if (AttributeEntityUtility.hasAttributeEntity(RootAppearance.class, "", (Appearance)sn.getNode()))
          	  entitiesList.add(ShaderUtility.createRootAppearance((Appearance) sn.getNode()));
        	if (AttributeEntityUtility.hasAttributeEntity(HapticShader.class, CommonAttributes.HAPTIC_SHADER, (Appearance)sn.getNode()))
          	  entitiesList.add(ShaderUtility.createHapticShader((Appearance) sn.getNode()));
            
          ents = entitiesList.toArray();
          entities.put(sn, ents);
          for (int i = 0; i < ents.length; i++)
            parents.put(ents[i], sn);
        }
        return ents.length;
      }
      int ret = sn.getChildren().size(); 
      if (sn.getNode() instanceof SceneGraphComponent) {
        ret += ((SceneGraphComponent)sn.getNode()).getTools().size();
      }
      return ret;
    }
    // entity
    Object[] ents = (Object[]) entities.get(parent);
    if (ents == null) {
      BeanInfo bi=null;
      try {
        bi = Introspector.getBeanInfo(parent.getClass());
      } catch (IntrospectionException e) {
        e.printStackTrace();
      }
      PropertyDescriptor[] pd=bi.getPropertyDescriptors();
      List<AttributeEntity> childEntities = new LinkedList<AttributeEntity>();
      for (int i = 0; i < pd.length; i++) {
        if (!AttributeEntity.class.isAssignableFrom(pd[i].getPropertyType())) continue;
        try {
          AttributeEntity ae = (AttributeEntity) pd[i].getReadMethod().invoke(parent, (Object[]) null);
          if (ae != null) {
            childEntities.add(ae);
            parents.put(ae, parent);
          }
        } catch (Exception e) {
          e.printStackTrace();
        }
      }
      ents = childEntities.toArray();
      entities.put(parent, ents);
    }
    return ents.length;
  }
  
  public Object getParent(Object o) {
    if (o instanceof SceneTreeNode )
      return ((SceneTreeNode)o).getParent();
    if (o instanceof TreeTool) return ((TreeTool)o).getTreeNode();
    else return parents.get(o);
  }
  
  public static class TreeTool {
    
    private final Tool tool;
    private SceneTreeNode node;
    
    private TreeTool(SceneTreeNode n, Tool t) {
      node = n;
      tool = t;
    }
    
    public Tool getTool() {
      return tool;
    }
    
    public SceneTreeNode getTreeNode() {
      return node;
    }
    
    @Override
    public int hashCode() {
      return node.hashCode()^(tool.hashCode()*31);
    }
    
    @Override
    public boolean equals(Object obj) {
      if (obj instanceof TreeTool) {
        TreeTool tt = (TreeTool) obj;
        return (node == tt.node && tool == tt.tool);
      }
      return false;
    }
  }
  
  private class SceneTreeNodeWithToolListener extends SceneTreeNode implements ToolListener, SceneGraphComponentListener {
    
    SceneGraphComponent cmp;
    List<Tool> tools=new LinkedList<Tool>();
    
    protected SceneTreeNodeWithToolListener(SceneGraphNode node) {
      super(node);
      if (isComponent) {
        cmp = (SceneGraphComponent) node;
        cmp.addToolListener(this);
        cmp.addSceneGraphComponentListener(this);
        tools.addAll(cmp.getTools());
      }
    }
    
    // addChild and removeChild is not really correct!
    // both methods would cause a deadlock when calling super.... in the runnable
    // and executed by invokeAndWait(..)
    
    public int addChild(final SceneTreeNode child) {
      final int[] ret = new int[1];
//      Runnable runner = new Runnable(){
//        public void run() {
          ret[0] = SceneTreeNodeWithToolListener.super.addChild(child);
          fireNodesAdded(SceneTreeNodeWithToolListener.this, new Object[]{child});
//        }
//      };
//      if (EventQueue.isDispatchThread()) runner.run();
//      else try {
//    	  EventQueue.invokeAndWait(runner);
//     //EventQueue.invokeLater(runner);
//      } catch (Exception e) {
//        throw new Error(e);
//      }
      return ret[0];
    }
    
    protected int removeChild(final SceneTreeNode prevChild) {
      final int[] ret = new int[1];
//      Runnable runner = new Runnable(){
//        public void run() {
          ret[0] = SceneTreeNodeWithToolListener.super.removeChild(prevChild);
          fireNodesRemoved(SceneTreeNodeWithToolListener.this, new int[]{ret[0]}, new Object[]{prevChild});
//        }
//      };
//      if (EventQueue.isDispatchThread()) runner.run();
//      else try {
//			EventQueue.invokeAndWait(runner);
//		} catch (InterruptedException e) {
//			throw new Error(e);
//		} catch (InvocationTargetException e) {
//			throw new Error(e);
//		}
      return ret[0];
    }
    
    public void toolAdded(final ToolEvent ev) {
//      Runnable runner = new Runnable(){
//        public void run() {
          int idx = getChildren().size()+tools.size();
          tools.add(ev.getTool());
          fireNodesAdded(SceneTreeNodeWithToolListener.this, new int[]{idx}, new Object[]{new TreeTool(SceneTreeNodeWithToolListener.this,ev.getTool())});
//        }
//      };
//      if (EventQueue.isDispatchThread()) runner.run();
//      else try {
//        EventQueue.invokeAndWait(runner);
////      EventQueue.invokeLater(runner);
//      } catch (Exception e) {
//        throw new Error(";-(");
//      }
    }
    
    public void toolRemoved(final ToolEvent ev) {
//      Runnable runner = new Runnable(){
//        public void run() {
          int idx = getChildren().size();
          int tind = tools.indexOf(ev.getTool());
          tools.remove(tind);
          fireNodesRemoved(SceneTreeNodeWithToolListener.this, new int[]{idx+tind}, new Object[]{new TreeTool(SceneTreeNodeWithToolListener.this,ev.getTool())});
//        }
//      };
//      if (EventQueue.isDispatchThread()) runner.run();
//      else try {
//        EventQueue.invokeAndWait(runner);
////      EventQueue.invokeLater(runner);
//      } catch (Exception e) {
//        throw new Error(";-(");
//      }
    }
    
    protected void dispose(ArrayList<SceneGraphNodeEntity> disposedEntities) {
      super.dispose(disposedEntities);
      if (isComponent) {
        cmp.removeToolListener(this);
        cmp.removeSceneGraphComponentListener(this);
      }
    }

		public void childAdded(SceneGraphComponentEvent ev) {
			// TODO Auto-generated method stub
			
		}

		public void childRemoved(SceneGraphComponentEvent ev) {
			// TODO Auto-generated method stub
			
		}

		public void childReplaced(SceneGraphComponentEvent ev) {
			// TODO Auto-generated method stub
			
		}

		public void visibilityChanged(SceneGraphComponentEvent ev) {
//      Runnable runner = new Runnable(){
//        public void run() {
          fireNodesChanged(SceneTreeNodeWithToolListener.this.getParent(), new Object[]{SceneTreeNodeWithToolListener.this});
//        }
//      };
//      if (EventQueue.isDispatchThread()) runner.run();
//      else try {
//        EventQueue.invokeAndWait(runner);
////      EventQueue.invokeLater(runner);
//      } catch (Exception e) {
//        throw new Error(";-(");
//      }
		}
    
  }

	public SceneTreeNode[] convertSceneGraphPath(SceneGraphPath selection) {
		ArrayList<SceneTreeNode> al = new ArrayList<SceneTreeNode>(selection.getLength());
		SceneTreeNode parent=builder.getTreeRoot();
		al.add(parent);
		for (Iterator<SceneGraphNode> it = selection.iterator(1); it.hasNext(); ) {
			SceneTreeNode child = parent.getTreeNodeForChild(it.next());
			al.add(child);
			parent=child;
		}
		return al.toArray(new SceneTreeNode[]{});
	}
  
	public SceneTreeNode[] convertSelection(Selection selection) {
		return convertSceneGraphPath(selection.getSGPath());
	}
}