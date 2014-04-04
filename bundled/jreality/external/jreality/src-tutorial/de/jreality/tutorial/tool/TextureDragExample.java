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


package de.jreality.tutorial.tool;

import static de.jreality.shader.CommonAttributes.POLYGON_SHADER;

import java.awt.Color;
import java.io.IOException;

import de.jreality.examples.CatenoidHelicoid;
import de.jreality.math.Matrix;
import de.jreality.math.MatrixBuilder;
import de.jreality.math.P3;
import de.jreality.math.Pn;
import de.jreality.math.Rn;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.Appearance;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.pick.PickResult;
import de.jreality.scene.tool.AbstractTool;
import de.jreality.scene.tool.InputSlot;
import de.jreality.scene.tool.Tool;
import de.jreality.scene.tool.ToolContext;
import de.jreality.shader.DefaultGeometryShader;
import de.jreality.shader.DefaultPolygonShader;
import de.jreality.shader.ImageData;
import de.jreality.shader.ShaderUtility;
import de.jreality.shader.Texture2D;
import de.jreality.shader.TextureUtility;
import de.jreality.tutorial.util.SimpleTextureFactory;
import de.jreality.util.Input;

public class TextureDragExample {

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
	final Texture2D tex = TextureUtility.createTexture(sgc.getAppearance(), POLYGON_SHADER,id);
	tex.setTextureMatrix(MatrixBuilder.euclidean().scale(scale).getMatrix());
    
    Tool t = new AbstractTool(InputSlot.getDevice("AllDragActivation")) {

    	private double[] origTexCoords;
    	Matrix origTexMatrix;

		{
   		addCurrentSlot(InputSlot.getDevice("PointerTransformation"), "drags the texture");
    	}
    	
		public void activate(ToolContext tc) {
			PickResult currentPick = tc.getCurrentPick();
			origTexCoords = currentPick.getTextureCoordinates();
			origTexMatrix = tex.getTextureMatrix();
		}

		public void perform(ToolContext tc) {
			PickResult currentPick = tc.getCurrentPick();
			if (currentPick == null) return;
			double[] texCoords = currentPick.getTextureCoordinates();
			if (texCoords == null || texCoords.length < 2) return;
			System.err.println("tc = "+Rn.toString(texCoords));
			double[] diff = Rn.subtract(null, origTexCoords, texCoords);
			double[] diff4 = {diff[0], diff[1], 0, 1.0};
			double[] trans = P3.makeTranslationMatrix(null, diff4, Pn.EUCLIDEAN);
			tex.setTextureMatrix(new Matrix(Rn.times(null, origTexMatrix.getArray(), trans)));
		}

		public String getDescription(InputSlot slot) {
			return null;
		}

		public String getDescription() {
			return "A tool which drags a texture around";
		}
    	
    };
	sgc.addTool(t);		

    JRViewer.display(sgc);
  }
}