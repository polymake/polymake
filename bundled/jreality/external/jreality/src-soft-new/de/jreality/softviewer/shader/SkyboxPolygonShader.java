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

import de.jreality.softviewer.Environment;
import de.jreality.softviewer.Polygon;

/**
 * 
 * @version 1.0
 * @author timh
 *
 */
public class SkyboxPolygonShader extends DefaultPolygonShader {

    @Override
    public boolean interpolateAlpha() {
        return false;
    }
    @Override
    public boolean needsSorting() {
        return false;
    }
    public SkyboxPolygonShader() {
        super(new ConstantVertexShader(1,1,1));
    }
    public SkyboxPolygonShader(de.jreality.shader.DefaultPolygonShader ps) {
        super(new ConstantVertexShader(1,1,1));
    }
    @Override
    public void shadePolygon(Polygon p, Environment environment, boolean vertexColors) {
        double[] matrix =environment.getMatrix();
        double x = -matrix[0+3];
        double y = -matrix[4+3];
        double z = -matrix[8+3];
        for(int i = 0; i< p.getLength();i++) {
            double[] vertexData = p.getPoint(i);
            double w = vertexData[Polygon.WW];
            vertexData[Polygon.WX] +=x*w;
            vertexData[Polygon.WY] +=y*w;
            vertexData[Polygon.WZ] +=z*w;
            //vertexShader.shadeVertex(vertexData,p.vertices[i],environment);
        }
        super.shadePolygon(p, environment, vertexColors);
        p.setSkybox(true);
    }
}
