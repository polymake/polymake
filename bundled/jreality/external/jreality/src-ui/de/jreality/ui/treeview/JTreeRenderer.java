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

import java.awt.Component;

import javax.swing.ImageIcon;
import javax.swing.JTree;
import javax.swing.tree.DefaultTreeCellRenderer;

import de.jreality.scene.Appearance;
import de.jreality.scene.AudioSource;
import de.jreality.scene.Camera;
import de.jreality.scene.Geometry;
import de.jreality.scene.Light;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphNode;
import de.jreality.scene.SceneGraphVisitor;
import de.jreality.scene.Transformation;
import de.jreality.scene.data.AttributeEntity;
import de.jreality.scene.proxy.tree.SceneTreeNode;
import de.jreality.ui.treeview.SceneTreeModel.TreeTool;

/**
 * Render a node by showing the simple name and an (optional) icon.
 * @author holger
 */
public class JTreeRenderer extends DefaultTreeCellRenderer
  //implements TreeCellRenderer, TableCellRenderer, ListCellRenderer
{

  protected static ImageIcon createImageIcon(String path) {
    java.net.URL imgURL = JTreeRenderer.class.getResource(path);
    if (imgURL != null) {
        return new ImageIcon(imgURL);
    } else {
        System.err.println("Couldn't find file: " + path);
        return null;
    }
}


  static final ImageIcon trafoIcon = createImageIcon("icons/TrafoIcon.png");
  static final ImageIcon camIcon = createImageIcon("icons/CamIcon.png");
  static final ImageIcon geomIcon = createImageIcon("icons/GeometryIcon.png");
  static final ImageIcon sgcIcon = createImageIcon("icons/SceneGraphComponentIcon.png");
  static final ImageIcon sgcOwnedIcon = createImageIcon("icons/SceneGraphComponentOwnedIcon.png");
  static final ImageIcon sgcInvisibleIcon = createImageIcon("icons/SceneGraphComponentInvisibleIcon.png");
  static final ImageIcon appIcon = createImageIcon("icons/AppearanceIcon.png");
  static final ImageIcon lightIcon = createImageIcon("icons/LightIcon.png");
  static final ImageIcon shaderIcon = createImageIcon("icons/ShaderIcon.png");
  static final ImageIcon toolIcon = createImageIcon("icons/ToolIcon.png");
  static final ImageIcon audioIcon = createImageIcon("icons/AudioSourceIcon.png");

  final SceneGraphVisitor iconSelector = new SceneGraphVisitor() {
    public void visit(Appearance a) {
      setIcon(appIcon);
    }
    public void visit(Geometry g) {
      setIcon(geomIcon);
    }
    public void visit(SceneGraphComponent c) {
    	if (c.getOwner()!=null) setIcon(sgcOwnedIcon);
    	else if (c.isVisible()) setIcon(sgcIcon);
      else setIcon(sgcInvisibleIcon);
    }
    public void visit(Transformation t) {
      setIcon(trafoIcon);
    }
    public void visit(Camera c) {
      setIcon(camIcon);
    }
    public void visit(Light l) {
      setIcon(lightIcon);
    }
    public void visit(AudioSource a) {
    	setIcon(audioIcon);
    }
  };
  
  final StringBuffer buffer=new StringBuffer(30);
  /**
   * Constructor for SimpleSGCellRenderer.
   */
  public JTreeRenderer()
  {
    super();
  }

  /**
   * @see javax.swing.tree.TreeCellRenderer#getTreeCellRendererComponent(JTree, Object, boolean, boolean, boolean, int, boolean)
   */
  public Component getTreeCellRendererComponent(JTree tree, Object value,
    boolean selected, boolean expanded, boolean leaf, int row, boolean focus)
  {
    if (value instanceof SceneTreeNode) {
      SceneGraphNode m=((SceneTreeNode)value).getNode();
      buffer.append(m.getName());
    } else if (value instanceof AttributeEntity){
      String ifName = value.getClass().getInterfaces()[0].getName();
      buffer.append(ifName.substring(ifName.lastIndexOf(".")+1));
    }
    else if (value instanceof TreeTool){
      String ifName = ((TreeTool)value).getTool().getClass().getName();
      buffer.append(ifName.substring(ifName.lastIndexOf(".")+1));
    }
    final Component c=super.getTreeCellRendererComponent(
        tree, buffer.toString(), selected, expanded, leaf, row, focus);
    buffer.setLength(0);
    if (value instanceof SceneTreeNode) {
      ((SceneTreeNode)value).getNode().accept(iconSelector);
    } else if (value instanceof AttributeEntity) {
      setIcon(shaderIcon);
    } else if (value instanceof TreeTool) {
      setIcon(toolIcon);
    }
    return c;
  }
}
