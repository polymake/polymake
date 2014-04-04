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


package de.jreality.tutorial.app;

import static de.jreality.shader.CommonAttributes.POLYGON_SHADER;

import java.awt.Color;
import java.io.IOException;

import de.jreality.examples.CatenoidHelicoid;
import de.jreality.geometry.Primitives;
import de.jreality.math.MatrixBuilder;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.Appearance;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.shader.DefaultGeometryShader;
import de.jreality.shader.DefaultPolygonShader;
import de.jreality.shader.ImageData;
import de.jreality.shader.ShaderUtility;
import de.jreality.shader.Texture2D;
import de.jreality.shader.TextureUtility;
import de.jreality.tutorial.util.SimpleTextureFactory;
import de.jreality.util.Input;

public class TextureExample {
  
/**
 * This tutorial shows how to apply a 2D texture image to a jReality geometry.  It also shows how to disable this
 * texture for nodes in the subgraph of the node containing the texture (that is, how to disable inheriting the texture).  
 * @param args
 * @throws IOException
 */
public static void main(String[] args) throws IOException {
	  	IndexedFaceSet geom = new CatenoidHelicoid(40);
		SceneGraphComponent sgc = new SceneGraphComponent("TextureExample");
		sgc.setGeometry(geom);
		
		Appearance ap = new Appearance();
		sgc.setAppearance(ap);
		DefaultGeometryShader dgs = (DefaultGeometryShader) ShaderUtility.createDefaultGeometryShader(ap, true);
		dgs.setShowLines(false);
		dgs.setShowPoints(false);
		DefaultPolygonShader dps = (DefaultPolygonShader) dgs.createPolygonShader("default");
		dps.setDiffuseColor(Color.white);
		ImageData id = null;
		double scale = 1;
		// get the image for the texture first
		if (args.length > 0) {
			id = ImageData.load(Input.getInput(args[0]));
		} else { // use a procedural texture
			SimpleTextureFactory stf = new SimpleTextureFactory();
			stf.setColor(0, new Color(0,0,0,0));	// gap color in weave pattern is totally transparent
			stf.setColor(1, new Color(255,0,100));
			stf.setColor(2, new Color(255,255,0));
			stf.update();
			id = stf.getImageData();
			scale = 10;
			dps.setDiffuseColor(Color.white);
		}
		Texture2D tex = TextureUtility.createTexture(ap, POLYGON_SHADER, id);
		System.err.println(ap.toString());
		tex.setTextureMatrix(MatrixBuilder.euclidean().scale(scale).getMatrix());
		// Attach a node below the textured one and show how to turn off texturing in this node
		SceneGraphComponent sgc1 = new SceneGraphComponent("UnTextureExample");
		sgc.addChild(sgc1);
		sgc1.setGeometry(Primitives.texturedQuadrilateral());
		MatrixBuilder.euclidean().scale(5).translate(-.5, -.5, 0).assignTo(sgc1);
		ap = new Appearance();
		sgc1.setAppearance(ap);
		// TODO: there should be a method in TextureUtility to do this
		// comment the following line out in order to apply the texture to the square.
   		ap.setAttribute("polygonShader.texture2d", Appearance.DEFAULT);
		JRViewer.display(sgc);
  }
}