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

import de.jreality.math.Rn;
import de.jreality.scene.Geometry;
import de.jreality.shader.EffectiveAppearance;

/**
 * 
 * @version 1.0
 * @author <a href="mailto:hoffmann@math.tu-berlin.de">Tim Hoffmann</a>
 *
 */
public class TwoSidePolygonShader implements PolygonShader {
    private PolygonShader front;
    private PolygonShader back;
    
    public TwoSidePolygonShader() {
        super();
        front = new DefaultPolygonShader();
        back = new DefaultPolygonShader();
    }
    
	 
	public TwoSidePolygonShader(PolygonShader f, PolygonShader b) {
		 super();
		 front =f;
         back = b;
	 }

    public final void shadePolygon(Polygon p, double vd[], Environment environment) {
        int pos = p.vertices[0];
        double px = vd[pos+Polygon.SX];
        double py = vd[pos+Polygon.SY];
        double pz = vd[pos+Polygon.SZ];
        double[] d1 = new double[3];
        pos =p.vertices[1];
        d1[0] =vd[pos+Polygon.SX] -px;
        d1[1] =vd[pos+Polygon.SY] -py;
        d1[2] =vd[pos+Polygon.SZ] -pz;
        double[] d2 = new double[3];
        pos =p.vertices[p.length-1];
        d2[0] =vd[pos+Polygon.SX] -px;
        d2[1] =vd[pos+Polygon.SY] -py;
        d2[2] =vd[pos+Polygon.SZ] -pz;
        
        d1 = Rn.crossProduct(null,d1,d2);
        
		boolean faceforward = (px * d1[0] + py * d1[1] + pz * d1[2]) <= 0 ;
        if(faceforward){
            front.shadePolygon(p,vd,environment);
            p.setShader(front);
        } else {
            back.shadePolygon(p,vd,environment);
            p.setShader(back);
        }
    }

    public final VertexShader getVertexShader() {
        return null;
    }

    public final void setVertexShader(VertexShader s) {
     
    }

    public final boolean interpolateColor() {
        return false;
    }

    public boolean interpolateAlpha() {
        return false;
    }

    public boolean isOutline() {
        return false;
    }

    public void setOutline(boolean outline) {
    }

  public void setup(EffectiveAppearance eAppearance, String shaderName) {
      front=ShaderLookup.getPolygonShaderAttr(eAppearance, shaderName, "front");
      back=ShaderLookup.getPolygonShaderAttr(eAppearance, shaderName, "back");
  }


    public Texture getTexture() {
        return null;
    }
    
    
    public boolean hasTexture() {
        return false;
    }

    public void startGeometry(Geometry geom)
    {
        front.startGeometry(geom);
        back.startGeometry(geom);
    }
    public boolean needsSorting() {
        return false; 
    }
}
