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


package de.jreality.scene.tool;

import java.awt.Color;

import de.jreality.geometry.IndexedFaceSetUtility;
import de.jreality.geometry.Primitives;
import de.jreality.math.P3;
import de.jreality.plugin.JRViewer;
import de.jreality.plugin.content.ContentAppearance;
import de.jreality.plugin.content.ContentLoader;
import de.jreality.plugin.content.ContentTools;
import de.jreality.scene.DirectionalLight;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.util.SceneGraphUtility;

/**
 * @author brinkman
 * 
 * TODO To change the template for this generated type comment go to Window -
 * Preferences - Java - Code Style - Code Templates
 */
public class ToolTestScene {

  public static SceneGraphComponent makeLights() {
    SceneGraphComponent lights = new SceneGraphComponent();
    lights.setName("lights");
    //SpotLight pl = new SpotLight();
    de.jreality.scene.PointLight pl = new de.jreality.scene.PointLight();
    //DirectionalLight pl = new DirectionalLight();
    pl.setFalloff(1.0, 0.0, 0.0);
    pl.setColor(new Color(170, 170, 120));
    //pl.setConeAngle(Math.PI);

    pl.setIntensity(0.6);
    SceneGraphComponent l0 = SceneGraphUtility
        .createFullSceneGraphComponent("light0");
    l0.setLight(pl);
    lights.addChild(l0);
    DirectionalLight dl = new DirectionalLight();
    dl.setColor(new Color(200, 150, 200));
    dl.setIntensity(0.6);
    l0 = SceneGraphUtility.createFullSceneGraphComponent("light1");
    double[] zaxis = { 0, 0, 1 };
    double[] other = { 1, 1, 1 };
    l0.getTransformation().setMatrix(P3.makeRotationMatrix(null, zaxis, other));
    l0.setLight(dl);
    lights.addChild(l0);

    return lights;
  }

  SceneGraphComponent createScene() {
    SceneGraphComponent scene = new SceneGraphComponent();
   
    /************ CREATE SCENE ***********/
    //scene.addChild(new JOGLSkyBox().makeWorld());
    
    IndexedFaceSet ifs = Primitives.torus(2, .5, 10, 10);
	IndexedFaceSetUtility.calculateAndSetEdgesFromFaces(ifs);
    
    IndexedFaceSetUtility.calculateAndSetFaceNormals(ifs);
    //AABBTree obbTree = AABBTree.constructEdgeAABB(ifs, 0.1);
    //ifs.setGeometryAttributes(Attribute.attributeForName("AABBTreeEdge"), obbTree);
    //AABBTree.constructAndRegister(ifs, null, 5);
    SceneGraphComponent comp = new SceneGraphComponent();
    comp.setGeometry(ifs);
    //comp.addChild(obbTree.display());
    
    scene.addChild(comp);
    return scene;
  }

  public static void main(String[] args) {
    ToolTestScene tts = new ToolTestScene();
    JRViewer v = new JRViewer();
	v.addBasicUI();
	v.setContent(tts.createScene());
	v.registerPlugin(new ContentAppearance());
	v.registerPlugin(new ContentLoader());
	v.registerPlugin(new ContentTools());
	v.startup();
  }
}
