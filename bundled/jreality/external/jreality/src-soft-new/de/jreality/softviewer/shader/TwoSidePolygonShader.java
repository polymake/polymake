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


package de.jreality.softviewer.shader;

import de.jreality.backends.texture.Texture;
import de.jreality.math.Rn;
import de.jreality.softviewer.Environment;
import de.jreality.softviewer.Polygon;

/**
 * 
 * @version 1.0
 * @author <a href="mailto:hoffmann@math.tu-berlin.de">Tim Hoffmann</a>
 *
 */
public class TwoSidePolygonShader extends PolygonShader {
    private PolygonShader front;
    private PolygonShader back;
    
    public TwoSidePolygonShader(de.jreality.shader.TwoSidePolygonShader ps, de.jreality.shader.RenderingHintsShader rhs) {
        super();
        front = PolygonShader.createFrom(ps.getFront(), rhs);
        back = PolygonShader.createFrom(ps.getBack(), rhs);
    }
    
	 
	public TwoSidePolygonShader(PolygonShader f, PolygonShader b) {
		 super();
		 front =f;
         back = b;
	 }

    public final void shadePolygon(Polygon p, Environment environment,boolean vertexColors) {
        double[] vd = p.getPoint(0);
        double px = vd[Polygon.SX];
        double py = vd[Polygon.SY];
        double pz = vd[Polygon.SZ];
        double[] d1 = new double[3];
        vd = p.getPoint(1);
        d1[0] =vd[Polygon.SX] -px;
        d1[1] =vd[Polygon.SY] -py;
        d1[2] =vd[Polygon.SZ] -pz;
        double[] d2 = new double[3];
        vd = p.getPoint(p.getLength()-1);
        d2[0] =vd[Polygon.SX] -px;
        d2[1] =vd[Polygon.SY] -py;
        d2[2] =vd[Polygon.SZ] -pz;
        
        d1 = Rn.crossProduct(null,d1,d2);
        
		boolean faceforward = (px * d1[0] + py * d1[1] + pz * d1[2]) >= 0 ;
        if(faceforward){
            front.shadePolygon(p,environment,vertexColors);
        } else {
            back.shadePolygon(p,environment,vertexColors);
        }
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

    public Texture getTexture() {
        return null;
    }
    
    
    public boolean hasTexture() {
        return false;
    }

//    public void startGeometry(Geometry geom)
//    {
//        front.startGeometry(geom);
//        back.startGeometry(geom);
//    }
    public boolean needsSorting() {
        return false; 
    }


    @Override
    public double getBlue() {
        return front.getBlue();
    }


    @Override
    public double getGreen() {
        return front.getGreen();
    }


    @Override
    public double getRed() {
        return front.getRed();
    }


    @Override
    public void setColor(double r, double g, double b) {
        front.setColor(r, g, b);
    }
}
