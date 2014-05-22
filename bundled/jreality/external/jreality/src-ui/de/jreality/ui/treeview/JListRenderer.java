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

import javax.swing.DefaultListCellRenderer;
import javax.swing.ImageIcon;
import javax.swing.JList;
import javax.swing.JTree;

import de.jreality.scene.Appearance;
import de.jreality.scene.Camera;
import de.jreality.scene.Geometry;
import de.jreality.scene.Light;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphNode;
import de.jreality.scene.SceneGraphVisitor;
import de.jreality.scene.Transformation;

/**
 * This class is OUT OF DATE - TODO: merge with JTreeRenderer or delete
 */
public class JListRenderer extends DefaultListCellRenderer
  //implements TreeCellRenderer, TableCellRenderer, ListCellRenderer
{

  protected static ImageIcon createImageIcon(String path) {
    java.net.URL imgURL = JListRenderer.class.getResource(path);
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
  static final ImageIcon appIcon = createImageIcon("icons/AppearanceIcon.png");
  static final ImageIcon lightIcon = createImageIcon("icons/LightIcon.png");

  final SceneGraphVisitor iconSelector = new SceneGraphVisitor() {
    public void visit(Appearance a) {
      setIcon(appIcon);
    }
    public void visit(Geometry g) {
      setIcon(geomIcon);
    }
    public void visit(SceneGraphComponent c) {
      setIcon(sgcIcon);
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
  };
  
  final StringBuffer buffer=new StringBuffer(30);
  /**
   * Constructor for SimpleSGCellRenderer.
   */
  public JListRenderer()
  {
    super();
  }

  /**
   * @see javax.swing.tree.TreeCellRenderer#getTreeCellRendererComponent(JTree, Object, boolean, boolean, boolean, int, boolean)
   */
  public Component getListCellRendererComponent(JList list, Object value,
    int index, boolean selected, boolean focus)
  {
    SceneGraphNode m=(SceneGraphNode)value;
    buffer.append(m.getName());
    final Component c=super.getListCellRendererComponent(
      list, buffer.toString(), index, selected, focus);
    buffer.setLength(0);
    m.accept(iconSelector);
    return c;
  }
}
