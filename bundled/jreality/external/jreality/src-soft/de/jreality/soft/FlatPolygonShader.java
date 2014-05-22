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


package de.jreality.soft;

import de.jreality.scene.Geometry;
import de.jreality.scene.data.AttributeEntityUtility;
import de.jreality.shader.EffectiveAppearance;
import de.jreality.shader.ShaderUtility;
import de.jreality.shader.Texture2D;

/**
 * 
 * @version 1.0
 * @author <a href="mailto:hoffmann@math.tu-berlin.de">Tim Hoffmann</a>
 *
 */
public class FlatPolygonShader implements PolygonShader {
    //private boolean interpolateColor = false;
    private VertexShader vertexShader;
    private double[] v = new double[Polygon.VERTEX_LENGTH]; 
    private boolean outline = false;
    private Texture texture;

    private double[] d1 =new double[3];
    private double[] d2 =new double[3];
    private double[] d3 =new double[3];
    
    public FlatPolygonShader() {
        super();
        vertexShader = new DefaultVertexShader();
    }
    
    public  FlatPolygonShader(VertexShader v, boolean outline) {
        super();
        vertexShader =  v;
        setOutline(outline);
    }

    //TODO Make this use the face normals.
    // for that to work the pipeline should set those if the
    // polygonshader is flat and the facenormals are present...
    public final void shadePolygon(Polygon p, double vertexData[], Environment environment) {
		v[Polygon.SX] = 0;
		v[Polygon.SY] = 0;
		v[Polygon.SZ] = 0;
		v[Polygon.R] = 0;
		v[Polygon.G] = 0;
        v[Polygon.B] = 0;
        v[Polygon.A] = 0;
        v[Polygon.NX] = 0;
		v[Polygon.NY] = 0;
		v[Polygon.NZ] = 0;
		for(int i = 0; i< p.length;i++) {
			int pos = p.vertices[i];
			v[Polygon.SX] += vertexData[pos+Polygon.SX];
			v[Polygon.SY] += vertexData[pos+Polygon.SY];
			v[Polygon.SZ] += vertexData[pos+Polygon.SZ];
			v[Polygon.R] += vertexData[pos+Polygon.R];
			v[Polygon.G] += vertexData[pos+Polygon.G];
            v[Polygon.B] += vertexData[pos+Polygon.B];
            v[Polygon.A] += vertexData[pos+Polygon.A];
//			v[Polygon.NX] += vertexData[pos+Polygon.NX];
//			v[Polygon.NY] += vertexData[pos+Polygon.NY];
//			v[Polygon.NZ] += vertexData[pos+Polygon.NZ];
		}
        v[Polygon.SX] /=p.length;
        v[Polygon.SY] /=p.length;
        v[Polygon.SZ] /=p.length;
        v[Polygon.R ] /=p.length;
        v[Polygon.G ] /=p.length;
        v[Polygon.B ] /=p.length;
        v[Polygon.A ] /=p.length;
        
//        d1[0] = vertexData[p.vertices[2]+Polygon.SX]-vertexData[p.vertices[1]+Polygon.SX];
//        d1[1] = vertexData[p.vertices[2]+Polygon.SY]-vertexData[p.vertices[1]+Polygon.SY];
//        d1[2] = vertexData[p.vertices[2]+Polygon.SZ]-vertexData[p.vertices[1]+Polygon.SZ];
//        
//        d2[0] = vertexData[p.vertices[0]+Polygon.SX]-vertexData[p.vertices[1]+Polygon.SX];
//        d2[1] = vertexData[p.vertices[0]+Polygon.SY]-vertexData[p.vertices[1]+Polygon.SY];
//        d2[2] = vertexData[p.vertices[0]+Polygon.SZ]-vertexData[p.vertices[1]+Polygon.SZ];
//        
//        
//        VecMat.cross(d1,d2,d3);
//        v[Polygon.NX] = d3[0];
//        v[Polygon.NY] = d3[1];
//        v[Polygon.NZ] = d3[2];
//        VecMat.normalize(v,Polygon.NX);
        int pos = p.vertices[0];
        v[Polygon.NX] = vertexData[pos+Polygon.NX];
        v[Polygon.NY] = vertexData[pos+Polygon.NY];
        v[Polygon.NZ] = vertexData[pos+Polygon.NZ];
        
        
		vertexShader.shadeVertex(v,0,environment);
		
		for(int i = 0; i< p.length;i++) {
		    pos = p.vertices[i];
			vertexData[pos+Polygon.R] = v[Polygon.R];
			vertexData[pos+Polygon.G] = v[Polygon.G];
			vertexData[pos+Polygon.B] = v[Polygon.B];
		}
    }

    public final VertexShader getVertexShader() {
        return vertexShader;
    }

    public final void setVertexShader(VertexShader s) {
		vertexShader = s;
    }

    public final boolean interpolateColor() {
        //return interpolateColor;
        return false;
    }

    public boolean interpolateAlpha() {
        return false;
    }

    public boolean isOutline() {
        return outline;
    }

    public void setOutline(boolean outline) {
        this.outline = outline;
    }

    public void setup(EffectiveAppearance eAppearance, String name) {
      outline = eAppearance.getAttribute(ShaderUtility.nameSpace(name, "outline"), outline);
      vertexShader=ShaderLookup.getVertexShaderAttr(eAppearance, name, "vertexShader");
      // since the shading is done only once per polygon at the moment
      // we do not need to interpolate the colors:
      //interpolateColor=!(vertexShader instanceof ConstantVertexShader);

      if (AttributeEntityUtility.hasAttributeEntity(Texture2D.class, ShaderUtility.nameSpace(name,"texture2d"), eAppearance))
          texture = new SimpleTexture((Texture2D) AttributeEntityUtility.createAttributeEntity(Texture2D.class, ShaderUtility.nameSpace(name,"texture2d"), eAppearance));
     
    }
    public Texture getTexture() {
        return texture;
    }


    public boolean hasTexture() {
        return texture != null;
    }
    public void startGeometry(Geometry geom)
    {
        if(vertexShader!=null) vertexShader.startGeometry(geom);
    }
    public boolean needsSorting() {
        return (getVertexShader().getTransparency() != 0.)||hasTexture()||interpolateAlpha(); 
    }
}
