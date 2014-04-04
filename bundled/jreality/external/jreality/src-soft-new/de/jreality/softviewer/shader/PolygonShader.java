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
import de.jreality.scene.Geometry;
import de.jreality.softviewer.Environment;
import de.jreality.softviewer.Polygon;

/**
 * This is what the PolygonPipeline uses to shade a polygon.
 * @version 1.0
 * @author <a href="mailto:hoffmann@math.tu-berlin.de">Tim Hoffmann</a>
 *
 */
public abstract class PolygonShader {
	protected Texture texture;

    public abstract  void shadePolygon(final Polygon p, final Environment environment,boolean vertexColors);
	public abstract boolean interpolateColor();
	public abstract boolean isOutline();
    public abstract boolean hasTexture();
    public abstract boolean interpolateAlpha();
    public abstract boolean needsSorting();
    public abstract void setColor(double r, double g, double b);
    public abstract double getRed();
    public abstract double getGreen();
    public abstract double getBlue();
    public void startGeometry(Geometry geom) {
        
    }
    
    public static PolygonShader createFrom(de.jreality.shader.PolygonShader ps,de.jreality.shader.RenderingHintsShader rs) {
        if(ps instanceof de.jreality.shader.HatchPolygonShader)
            return new HatchPolygonShader((de.jreality.shader.DefaultPolygonShader) ps);
        if(ps instanceof de.jreality.shader.InvertPolygonShader)
            return new InvertPolygonShader((de.jreality.shader.DefaultPolygonShader) ps);
        if(ps instanceof de.jreality.shader.EtchPolygonShader)
            return new EtchPolygonShader((de.jreality.shader.DefaultPolygonShader) ps);

        if(ps instanceof de.jreality.shader.DefaultPolygonShader)
            return rs.getLightingEnabled() ? 
                    new DefaultPolygonShader((de.jreality.shader.DefaultPolygonShader) ps) :
                    new ConstantPolygonShader((de.jreality.shader.DefaultPolygonShader) ps);
        if(ps instanceof de.jreality.shader.TwoSidePolygonShader)
            return new TwoSidePolygonShader((de.jreality.shader.TwoSidePolygonShader) ps, rs);
        
return null;
    }
    public void setTexture(Texture texture) {
        this.texture = texture;
    }
    public Texture getTexture() {
        return texture;
    }
}
