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


package de.jreality.ui.viewerapp;

import java.awt.Color;

import de.jreality.geometry.Primitives;
import de.jreality.math.FactoredMatrix;
import de.jreality.scene.Appearance;
import de.jreality.scene.Camera;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.Transformation;


/**
 * Test viewerApp's menu and selection management.
 * 
 * @author msommer
 */
public class ViewerAppTest {
  
  
  public static void main(String[] args) {
    
//  	SceneGraphComponent cmp = getSGC();
    ViewerApp viewerApp = new ViewerApp(Primitives.icosahedron());//cmp);
    viewerApp.setAttachNavigator(true);
    viewerApp.setExternalNavigator(false);
    viewerApp.setAttachBeanShell(true);
    viewerApp.setExternalBeanShell(false);
    
    
    ((Camera)viewerApp.getViewerSwitch().getCameraPath().getLastElement()).setPerspective(false);
    viewerApp.setBackgroundColor(Color.WHITE);
    
    viewerApp.update();
    viewerApp.display();
    
//    cmp.addTool(new SelectionTool(viewerApp));

  }
  
  
  private static SceneGraphComponent getSGC() {
  	
  	SceneGraphComponent cmp = new SceneGraphComponent("father");
    cmp.setAppearance(new Appearance());
    cmp.setTransformation(new Transformation());
    SceneGraphComponent cmp1 = new SceneGraphComponent("Icosahedron");
    cmp1.setGeometry(Primitives.icosahedron());
    SceneGraphComponent cmp2 = new SceneGraphComponent("Cube");
    cmp2.setGeometry(Primitives.coloredCube());
    cmp2.setAppearance(new Appearance());
    FactoredMatrix m = new FactoredMatrix();
    m.setTranslation(3,0,0);
    m.assignTo(cmp2);
    cmp.addChild(cmp1);
    cmp.addChild(cmp2);
    
    return cmp;
  }
  
}