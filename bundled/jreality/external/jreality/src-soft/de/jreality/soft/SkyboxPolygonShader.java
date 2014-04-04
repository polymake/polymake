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
 * @author timh
 *
 */
public class SkyboxPolygonShader implements PolygonShader {
    private boolean interpolateColor=true;
    private VertexShader vertexShader;
    private boolean outline = false;
    private Texture texture;

    public SkyboxPolygonShader() {
        super();
        vertexShader = new DefaultVertexShader();
    }
    
    public SkyboxPolygonShader(VertexShader v) {
        super();
        vertexShader = v;
    }
    
    public SkyboxPolygonShader(VertexShader v,boolean outline) {
        super();
        vertexShader = v;
        this.outline = outline;
    }

    public final void shadePolygon(Polygon p, double vertexData[], Environment environment) {
        double[] matrix =environment.getMatrix();
        double x = -matrix[0+3];
        double y = -matrix[4+3];
        double z = -matrix[8+3];
        for(int i = 0; i< p.length;i++) {
            int pos = p.vertices[i];
            vertexData[pos+Polygon.SX] = vertexData[pos+Polygon.SX] + x;
            vertexData[pos+Polygon.SY] = vertexData[pos+Polygon.SY] + y;
            vertexData[pos+Polygon.SZ] = vertexData[pos+Polygon.SZ] + z;
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
        setVertexShader(ShaderLookup.getVertexShaderAttr(eAppearance, name, "vertexShader"));

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
