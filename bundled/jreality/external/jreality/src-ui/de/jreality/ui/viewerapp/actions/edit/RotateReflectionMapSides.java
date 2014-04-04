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

import java.awt.Component;
import java.awt.event.ActionEvent;

import de.jreality.scene.Appearance;
import de.jreality.scene.data.AttributeEntityUtility;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.CubeMap;
import de.jreality.shader.ImageData;
import de.jreality.shader.TextureUtility;
import de.jreality.ui.viewerapp.SelectionEvent;
import de.jreality.ui.viewerapp.SelectionManager;
import de.jreality.ui.viewerapp.actions.AbstractSelectionListenerAction;


/**
 * Rotates the sides of the reflection map's cubemap 
 * of a selected SceneGraphComponent or Appearance 
 * to match its top and bottom.
 * 
 * @author msommer
 */
public class RotateReflectionMapSides extends AbstractSelectionListenerAction {

  public RotateReflectionMapSides(String name, SelectionManager sm, Component frame) {
    
    super(name, sm, frame);
    setShortDescription("Rotate reflection map sides to match its top and bottom");
  }


  public void actionPerformed(ActionEvent e) {
  	
  	//get appearance
  	Appearance app = null;
  	if (getSelection().isComponent())
  		app = getSelection().getLastComponent().getAppearance();
  	else app = (Appearance) getSelection().getLastNode();
  	if (app == null) {
  		System.err.println("No reflection map loaded.");
  		return;
  	}

  	final CubeMap cm = (CubeMap) AttributeEntityUtility.getAttributeEntity(CubeMap.class, CommonAttributes.POLYGON_SHADER+".reflectionMap", app, true);
  	if (cm.getFront() == null) {
  		System.err.println("No reflection map loaded.");
  		return;
  	}

  	ImageData[] imgs = TextureUtility.getCubeMapImages(cm);
  	//imgs[2]=up, imgs[3]=dn
  	int[] order = new int[]{5,4,2,3,0,1};
  	ImageData[] newImgs = new ImageData[6];
  	for (int i = 0; i < newImgs.length; i++)
  		newImgs[i] = imgs[order[i]];

  	TextureUtility.createReflectionMap(app, CommonAttributes.POLYGON_SHADER, newImgs);
  }


  @Override
  public boolean isEnabled(SelectionEvent e) {
    return (e.componentSelected() || e.appearanceSelected());
  }

}