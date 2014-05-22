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


import de.jreality.backends.texture.EnvironmentTexture;
import de.jreality.backends.texture.SimpleTexture;
import de.jreality.backends.texture.Texture;
import de.jreality.scene.Geometry;
import de.jreality.shader.CubeMap;
import de.jreality.shader.Texture2D;
import de.jreality.softviewer.Environment;
import de.jreality.softviewer.Polygon;
import de.jreality.softviewer.VecMat;

/**
 * 
 * @version 1.0
 * @author <a href="mailto:hoffmann@math.tu-berlin.de">Tim Hoffmann</a>
 *
 */
public class ConstantPolygonShader extends PolygonShader {
    private boolean interpolateColor=false;
    protected VertexShader vertexShader;
    protected boolean outline = false;
    private final boolean needsNormals;
    
//    final private double[] d1;
//    final private double[] d2;
//    final private double[] d3;
    //final private double[] v;
    
    final private de.jreality.shader.DefaultPolygonShader ps;
    public  ConstantPolygonShader() {
        this(new ConstantVertexShader(0,0,0));
    }
    private ConstantPolygonShader(VertexShader v) {
        super();
        vertexShader= v;
//        d1 = null;
//        d2 = null;
//        d3 = null;
        //v = null;
        ps = null;
        needsNormals = false;
    }
    
    public ConstantPolygonShader(de.jreality.shader.DefaultPolygonShader ps) {
        super();
		vertexShader = new ConstantVertexShader(ps.getDiffuseColor(),ps.getTransparency());
        this.ps = ps;
        Texture2D tex = ps.getTexture2d();
        if (tex != null && tex.getImage() != null) {
            texture = new SimpleTexture(tex);
            //texture = MipMapedTexture.create(tex);
        }
        CubeMap cm = ps.getReflectionMap();
        if(cm != null) {
            
            texture = new EnvironmentTexture(cm,texture);
            needsNormals = true;
        } else {
            needsNormals = false;
        }

    }
    
    
    public void shadePolygon(Polygon p, Environment environment, boolean vertexColors) {
        p.setTransparency(vertexShader.getTransparency());
        p.setTexture(texture);
        p.setInterpolateAlpha(interpolateAlpha());
        p.setInterpolateColor(interpolateColor());
        p.setSkybox(false);
        int n = p.getLength();
        double[] v = p.getCenter();
        vertexShader.shadeVertex(v,environment,vertexColors);
		if(needsNormals) {
            for(int i = 0; i< n;i++) {
                double vertexData[] = p.getPoint(i);
                //vertexData[Polygon.R] = v[Polygon.R];
                //vertexData[Polygon.G] = v[Polygon.G];
                //vertexData[Polygon.B] = v[Polygon.B];
                vertexShader.shadeVertex(vertexData,environment,vertexColors);
                //if there is a reflection map we prepare the normals:
                
                    double ff =( vertexData[Polygon.WX]*vertexData[Polygon.NX] + vertexData[Polygon.WY]*vertexData[Polygon.NY] + vertexData[Polygon.WZ]*vertexData[Polygon.NZ])/
                    (vertexData[Polygon.NX]*vertexData[Polygon.NX] + vertexData[Polygon.NY]*vertexData[Polygon.NY] + vertexData[Polygon.NZ]*vertexData[Polygon.NZ]);
                    //v[Polygon.NX] = -2*ff * v[Polygon.NX] + v[Polygon.WX];
                    //v[Polygon.NY] = -2*ff * v[Polygon.NY] + v[Polygon.WY];
                    //v[Polygon.NZ] = -2*ff * v[Polygon.NZ] + v[Polygon.WZ];
                    double[] m = environment.getCameraWorld();
                    VecMat.transformUnNormalized(m, -2*ff * vertexData[Polygon.NX] + vertexData[Polygon.WX],
                            -2*ff * vertexData[Polygon.NY] + vertexData[Polygon.WY],
                            -2*ff * vertexData[Polygon.NZ] + vertexData[Polygon.WZ],
                            vertexData, Polygon.NX);
                
            }
        } else {
            for(int i = 0; i< n;i++) {
                double[] vertexData = p.getPoint(i);
               // vertexData[Polygon.R] = v[Polygon.R];
                //vertexData[Polygon.G] = v[Polygon.G];
                //vertexData[Polygon.B] = v[Polygon.B];
                vertexShader.shadeVertex(vertexData,environment,vertexColors);
            }
        }
    }

    public final VertexShader getVertexShader() {
        return vertexShader;
    }

    public final boolean interpolateColor() {
        return hasTexture()|| vertexShader.isVertexColors();
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
        return (vertexShader.getTransparency() != 0.)||
        (hasTexture() &&texture.isTransparent())||
        interpolateAlpha(); 
    }

    @Override
    public void setColor(double r, double g, double b) {
        vertexShader.setColor(r,g,b);
        
    }

    @Override
    public double getBlue() {
        return vertexShader.getBlue();
    }

    @Override
    public double getGreen() {
        return vertexShader.getGreen();
    }

    @Override
    public double getRed() {
        return vertexShader.getRed();
    }


}
