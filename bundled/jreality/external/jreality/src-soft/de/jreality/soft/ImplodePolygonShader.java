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
import de.jreality.shader.EffectiveAppearance;
import de.jreality.shader.ShaderUtility;

/**
 * 
 * @version 1.0
 * @author <a href="mailto:hoffmann@math.tu-berlin.de">Tim Hoffmann</a>
 *
 */
public class ImplodePolygonShader implements PolygonShader {
    private VertexShader vertexShader;
    private boolean outline;
    double implodeFactor;
    private boolean interpolateColor;
    private Texture texture;

    public ImplodePolygonShader() {
		this(new DefaultVertexShader(),0.8, false);
    }
    
    public ImplodePolygonShader(VertexShader v, double implodeFactor, boolean outline) {
		super();
    	vertexShader = v;
    	this.implodeFactor = implodeFactor;
    	setOutline(outline);
    }

    public final void shadePolygon(Polygon p, double vertexData[], Environment environment) {
		double centerX = 0;
		double centerY = 0;
		double centerZ = 0;
		for(int i = 0; i< p.length;i++) {
			int pos = p.vertices[i];
			centerX += vertexData[pos+Polygon.SX];
			centerY += vertexData[pos+Polygon.SY];
			centerZ += vertexData[pos+Polygon.SZ];
		}

		double oml = (1-implodeFactor)/p.length;
		centerX *= oml;
		centerY *= oml;
		centerZ *= oml;
		
		for(int i = 0; i< p.length;i++) {
			int pos = p.vertices[i];
			vertexData[pos+Polygon.SX] = implodeFactor * vertexData[pos+Polygon.SX] + centerX;
			vertexData[pos+Polygon.SY] = implodeFactor * vertexData[pos+Polygon.SY] + centerY;
			vertexData[pos+Polygon.SZ] = implodeFactor * vertexData[pos+Polygon.SZ] + centerZ;
			vertexShader.shadeVertex(vertexData,p.vertices[i],environment);
		}
    }

    public final VertexShader getVertexShader() {
        return vertexShader;
    }

    public final void setVertexShader(VertexShader s) {
        if(vertexShader!=s) {
            vertexShader = s;
            interpolateColor=!(s instanceof ConstantVertexShader);
        }
    }

    public final boolean interpolateColor() {
        return interpolateColor;
    }

    public boolean interpolateAlpha() {
        return vertexShader.interpolateAlpha();
    }

    public boolean isOutline() {
        return outline;
    }

    public void setOutline(boolean outline) {
        this.outline = outline;
    }

    public void setup(EffectiveAppearance eAppearance, String name) {
      outline = eAppearance.getAttribute(ShaderUtility.nameSpace(name, "outline"), outline);
      implodeFactor = eAppearance.getAttribute(ShaderUtility.nameSpace(name, "implodeFactor"), implodeFactor);
      setVertexShader(ShaderLookup.getVertexShaderAttr(eAppearance, name, "vertexShader"));
//    String textureFile = (String) eAppearance.getAttribute(NameSpace.name(name, "texture"), "");
//    try {
//        texture = new SimpleTexture(textureFile);
//    } catch (MalformedURLException e) {
//        Logger.getLogger("de.jreality").log(Level.FINEST, "attempt to load textur {0} failed", textureFile);
//        texture =null;
//    }
      texture =(Texture)eAppearance.getAttribute(ShaderUtility.nameSpace(name, "texture"),null,Texture.class);
      
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
