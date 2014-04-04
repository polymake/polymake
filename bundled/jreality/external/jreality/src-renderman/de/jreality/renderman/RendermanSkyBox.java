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


package de.jreality.renderman;

import java.awt.Color;
import java.io.File;
import java.util.HashMap;
import java.util.Map;

import de.jreality.math.P3;
import de.jreality.math.Pn;
import de.jreality.math.Rn;
import de.jreality.renderman.shader.ConstantTexture;
import de.jreality.scene.Appearance;
import de.jreality.shader.CubeMap;
import de.jreality.shader.ImageData;
import de.jreality.shader.Texture2D;
import de.jreality.shader.TextureUtility;

/**
 * Render a sky box in RenderMan rib format.
 * @author Charles Gunn
 *
 */
class RendermanSkyBox {
	double stretch = 40.0;

  // TODO straighten out nomenclature on faces
	static private double[][][] cubeVerts3 =  
		{
			 {{1,1,1}, {1,1,-1}, {1, -1, -1}, {1, -1, 1}},		// right
			 { {-1, 1, -1}, {-1, 1, 1},{-1,-1,1}, {-1,-1,-1}}, // left
			 { {-1, 1,-1},  {1, 1,-1},{1, 1,1}, {-1, 1,1}},		// 	up
			 {  {-1,-1,1},{1,-1,1},{1,-1,-1}, {-1,-1,-1}},		// down
			 {{-1,1,1}, {1,1,1}, {1,-1,1},{-1,-1,1}},		// back
			 {   {1,1,-1},{-1,1,-1}, {-1,-1,-1},{1,-1,-1}}			// front
		 };	
	
  // TODO figure out texture coordinates 
	static private double[][] texCoords = {{0,0},{1,0},{1,1},{0,1}};
	
	static Appearance a=new Appearance();
  static {
 }

  static int[] nvertices = {4};
  static int[] vertices = {0,1,2,3};
  static void render(RIBVisitor ribv, double[] w2c, CubeMap cm)	{
    ImageData[] imgs=TextureUtility.getCubeMapImages(cm);
    
    ribv.ri.attributeBegin("SkyBox");    
    if(ribv.getRendererType()==RIBViewer.TYPE_3DELIGHT)
      ribv.ri.verbatim("Attribute \"visibility\"  \"string transmission\" \"transparent\"");            
    else 
      ribv.ri.verbatim("Attribute \"visibility\"  \"int transmission\" [0]");
    
    
    ribv.ri.color(Color.WHITE);
    ribv.ri.comment("Skybox ");
    ribv.ri.transformBegin();
    ribv.ri.concatTransform(RIBHelper.fTranspose(Rn.times(null, 
    		Rn.inverse(null, w2c),
    		P3.extractOrientationMatrix(null, w2c, P3.originP3, Pn.EUCLIDEAN))));
	ribv.ri.concatTransform(RIBHelper.fTranspose(P3.makeStretchMatrix(null, 500.0)));
	ConstantTexture ct = new ConstantTexture();
	float[] vvv = new float[12];
	float[] tc = new float[8];
	Map map = new HashMap();
	map.put("P", vvv);
	map.put("st",tc);
	int n = 0;
	for (int j = 0; j<4; ++j)	
		for (int k = 0; k<2; ++k) 
			tc[n++] = (float) texCoords[j][k];
	for (int i = 0; i<6; ++i)	{
		String name = ribv.writeTexture(imgs[i], Texture2D.GL_CLAMP_TO_EDGE, Texture2D.GL_CLAMP_TO_EDGE);
		ct.getAttributes().put("texturename", new File(name).getName());
		ribv.ri.shader(ct);
		int m = 0;
		for (int j = 0; j<4; ++j)	{
			for (int k = 0; k<3; ++k) 
				vvv[m++] = (float) cubeVerts3[i][j][k];
		}
		ribv.ri.pointsPolygons(1,nvertices, vertices, map);
	}
	ribv.ri.transformEnd();
  ribv.ri.attributeEnd("SkyBox");
	}
	
}
