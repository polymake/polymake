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

import de.jreality.scene.Geometry;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.PointSet;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.DataList;
import de.jreality.softviewer.Environment;


/**
 * This is what the PolygonPipeline uses to shade a vertex.
 * @version 1.0
 * @author <a href="mailto:hoffmann@math.tu-berlin.de">Tim Hoffmann</a>
 *
 */
public abstract class VertexShader {
	
	protected boolean vertexColors;
    protected boolean interpolateAlpha;

    public abstract void shadeVertex(final double[] vertex, final Environment environment, boolean vertexColors);
	public abstract double getTransparency();
    public boolean interpolateAlpha() {
        return interpolateAlpha;
    }
    
    public abstract void setColor(double r, double g, double b);
    public abstract double getRed();
    public abstract double getGreen();
    public abstract double getBlue();
    public void startGeometry(Geometry geom)
    {
        DataList colors=null;
        
        vertexColors=
                (geom instanceof PointSet)
          &&((colors=((PointSet)geom).getVertexAttributes(Attribute.COLORS))!=null
                  );
        //interpolateAlpha=vertexColors&&colors.getStorageModel().getDimensions()[1]!=3;
        interpolateAlpha=vertexColors&&colors.item(0).size()!=3;
        
        if((geom instanceof IndexedFaceSet) && (colors =  ((IndexedFaceSet)geom).getFaceAttributes(Attribute.COLORS))!=null) {
            vertexColors |= true;
            interpolateAlpha |= colors.item(0).size()!=3;
        }
        //System.out.println(vertexColors+": colors: "+colors+" interpolate alpha: "+interpolateAlpha);
    }
    public boolean isVertexColors() {
        return vertexColors;
    }
}
