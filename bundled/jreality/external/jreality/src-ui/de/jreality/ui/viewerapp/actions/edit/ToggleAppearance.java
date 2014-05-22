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


package de.jreality.ui.viewerapp.actions.edit;

import java.awt.event.ActionEvent;
import java.awt.event.KeyEvent;
import java.util.Iterator;

import de.jreality.scene.Appearance;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.shader.CommonAttributes;
import de.jreality.ui.viewerapp.SelectionEvent;
import de.jreality.ui.viewerapp.SelectionManager;
import de.jreality.ui.viewerapp.actions.AbstractSelectionListenerAction;


/**
 * Toggles appearance atributes of a selected SceneGraphComponent or Appearance 
 * (if something different is selected, this action is disabled).<br>
 * Note that this action does not create new Appearances in the scene tree.
 * 
 * @author msommer
 */
@SuppressWarnings("serial")
public class ToggleAppearance extends AbstractSelectionListenerAction {

  private String attribute;
  private boolean defaultValue;
  
  
  public ToggleAppearance(String name, String attribute, SelectionManager sm) {
    super(name, sm);
    this.attribute = attribute;
    
    if (attribute.equals(CommonAttributes.VERTEX_DRAW)) {
      defaultValue = CommonAttributes.VERTEX_DRAW_DEFAULT;
      setShortDescription("Toggle vertex drawing of responsible appearance");
      setShortCut(KeyEvent.VK_V, 0, true);
    }
    else if (attribute.equals(CommonAttributes.EDGE_DRAW)) {
      defaultValue = CommonAttributes.EDGE_DRAW_DEFAULT;
      setShortDescription("Toggle edge drawing of responsible appearance");
      setShortCut(KeyEvent.VK_E, 0, true);
    }
    else if (attribute.equals(CommonAttributes.FACE_DRAW)) {
      defaultValue = CommonAttributes.FACE_DRAW_DEFAULT;
      setShortDescription("Toggle face drawing of responsible appearance");
      setShortCut(KeyEvent.VK_F, 0, true);
    }
  }

//  public ToggleAppearance(String name, String attribute, ViewerApp v) {
//    this(name, attribute, v.getSelectionManager());
//  }
  
  
  /**
   * Toggles the specified appearance attribute of the responsible appearance 
   * - the first one existing along the path from the selected component to the scene root. 
   * If there is no appearance along the path, nothing is toggled.
   * In particular, no new Appearances are created in the scene tree.
   */
  @Override
  public void actionPerformed(ActionEvent e) {
    
    Object value = null;
    SceneGraphComponent cmp = null;
    Appearance a, app = null;
    //get responsible appearance app and (inherited) value
    for (Iterator it = getSelection().reverseIterator(); it.hasNext(); ) {
      try { cmp = (SceneGraphComponent) it.next(); } 
      catch (ClassCastException cce) {
				continue;  //appearance selected
			}
      a = cmp.getAppearance();
      if (a != null) {
        value = a.getAttribute(attribute);
        if (app == null) app = a;  //responsible appearance
      }
      if (value instanceof Boolean) break;  //responsible value
    }
    if (app == null) return;  //no appearance along path
    if (!(value instanceof Boolean)) value = defaultValue;
        
    app.setAttribute(attribute, !((Boolean)value));
  }

  
  @Override
  public boolean isEnabled(SelectionEvent e) {
    return (e.componentSelected() || e.appearanceSelected());
  }
  
}