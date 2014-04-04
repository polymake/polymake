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


/**
 * 
 * @version 1.0
 * @author timh
 *
 */
public class DefaultLineShader extends LineShader {
    private double lineWidth =0.01;
    private double tubeRadius =0.01;
    private boolean drawTubes = true;
    private boolean radiiWorldCoordinates = false;
    PolygonShader polygonShader =null;
    /**
     * 
     */
    public DefaultLineShader(de.jreality.shader.DefaultLineShader lis,de.jreality.shader.RenderingHintsShader rhs) {
        super();
        polygonShader = PolygonShader.createFrom(lis.getPolygonShader(),rhs);
        lineWidth = lis.getLineWidth();
        tubeRadius = lis.getTubeRadius();
        drawTubes = lis.getTubeDraw();
        radiiWorldCoordinates = lis.getRadiiWorldCoordinates();
    }

    /* (non-Javadoc)
     * @see de.jreality.soft.LineShader#getPolygonShader()
     */
    public PolygonShader getPolygonShader() {
        return polygonShader;
    }

    /* (non-Javadoc)
     * @see de.jreality.soft.LineShader#getLineWidth()
     */
    public double getLineWidth() {
        return lineWidth;
    }

//     public void startGeometry(Geometry geom)
//    {
//        if(polygonShader!=null) polygonShader.startGeometry(geom);
//    }

    @Override
    public double getTubeRadius() {
        return tubeRadius;
    }

    @Override
    public boolean isDrawTubes() {
        return drawTubes;
    }

    public boolean isRadiiWorldCoordinates() {
        return radiiWorldCoordinates;
    }
}
