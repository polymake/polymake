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

import java.io.PrintWriter;

/**
 * This is a PS writer for the software renderer. At the moment it needs
 * the PolygonPipline to be configured to sort <em>all</em> Polygons (and
 * not only the transparent ones) since it uses a simple painter's 
 * algorithm. No polygon intersections are done. It is
 * first come first paint at the moment.
 * @version 1.0
 * @author <a href="mailto:hoffmann@math.tu-berlin.de">Tim Hoffmann</a>
 *
 */
public class PSRasterizer implements PolygonRasterizer {
    public static final String NONE = "none";
    private boolean useGradients = false;
    private int background;
    private int xmin;
    private int xmax;
    private int ymin;
    private int ymax;
    
    private double wh;
    private double hh;
    private double mh;
    
    private int pLength = 0;    
    private double[][] polygon = new double[Polygon.MAXPOLYVERTEX][Polygon.VERTEX_LENGTH];
    protected double transparency = 0;
    protected double oneMinusTransparency = 1;
    
    private PrintWriter writer;
    private int count;
    /**
     * 
     */
    public PSRasterizer( PrintWriter w) {
        super();
       writer =w;

    }

    /* (non-Javadoc)
     * @see de.jreality.soft.PolygonRasterizer#renderPolygon(de.jreality.soft.Polygon, double[], boolean)
     */
    public void renderPolygon(
        Polygon p,
        double[] vertexData,
        boolean outline) {
        transparency = (p.getShader().getVertexShader().getTransparency());
        oneMinusTransparency = 1 - transparency;

        pLength = p.length;
        
        if(pLength==0) {
            System.err.println("Warning polygon of length 0");
            return;
        }
        double[] t0 = new double[Polygon.VERTEX_LENGTH];
        
       
        for (int i = 0; i < pLength; i++) {
            int pos = p.vertices[i];
            double[] pi= polygon[i];
            
            double w = 1/vertexData[pos+Polygon.SW];
            double wxy =w*mh;
            pi[Polygon.SX] =(wh + vertexData[pos+Polygon.SX] * wxy);
            pi[Polygon.SY] =(hh - vertexData[pos+Polygon.SY] * wxy);
            pi[Polygon.SZ] =(vertexData[pos+Polygon.SZ] * w);


            pi[Polygon.R] = ((vertexData[pos+Polygon.R] > 1 ? 255 : (255*vertexData[pos+Polygon.R] )));
            pi[Polygon.G] = ((vertexData[pos+Polygon.G] > 1 ? 255 : (255*vertexData[pos+Polygon.G] )));
            pi[Polygon.B] = ((vertexData[pos+Polygon.B] > 1 ? 255 : (255*vertexData[pos+Polygon.B])));
            

            t0[Polygon.SX] += pi[Polygon.SX];
            t0[Polygon.SY] += pi[Polygon.SY];
            t0[Polygon.SZ] += pi[Polygon.SZ];
            t0[Polygon.R] += pi[Polygon.R];
            t0[Polygon.G] += pi[Polygon.G];
            t0[Polygon.B] += pi[Polygon.B];
        }

        t0[Polygon.SX] /= pLength;
        t0[Polygon.SY] /= pLength;
        t0[Polygon.R]  /= pLength;
        t0[Polygon.G]  /= pLength;
        t0[Polygon.B]  /= pLength;
        
        
        
        if(true) {
            double[][] pol = new double[3][];
            pol[2] =t0; 
            
            for(int n=0;n<pLength-1;n++) {
                pol[0] =polygon[n];
                pol[1] =polygon[n+1];
                
                writePolygon(pol);
                
                
                count++;
            }
            pol[0] =polygon[pLength-1];
            pol[1] =polygon[0];
            writePolygon(pol);
            count++;
            if(p.getShader().isOutline())
                linePolygon(polygon);
            
        }
        else {
            //           writePolygon(polygon, pLength, col, p.getShader().isOutline()?"black":NONE);
        }
        
        
        count++;
    }

    

    private void writePolygon(double[][] polygon) {
        writer.println("<<\n/ColorSpace [/DeviceRGB]\n/ShadingType 4\n/DataSource [");
        for(int i =0;i<3;i++) {
            writer.print(" 0");
            writer.print(" "+polygon[i][Polygon.SX]);
            writer.print(" "+((ymax-ymin)-polygon[i][Polygon.SY]));
            writer.print(" "+polygon[i][Polygon.R]/255.);
            writer.print(" "+polygon[i][Polygon.G]/255.);
            writer.println(" "+polygon[i][Polygon.B]/255.);
        }
        writer.print("]\n>> shfill");
        
    }

    private void linePolygon(double[][] p) {
        
        writer.println("\n"+p[0][Polygon.SX]+" "+((ymax-ymin)-p[0][Polygon.SY])+" moveto");
        for(int i =1;i<pLength;i++) {
            writer.println(p[i][Polygon.SX]+" "+((ymax-ymin)-p[i][Polygon.SY])+" lineto");
        }
        writer.println("\n"+p[0][Polygon.SX]+" "+((ymax-ymin)-p[0][Polygon.SY])+" lineto");
        writer.println("stroke");
    }

    /* (non-Javadoc)
     * @see de.jreality.soft.PolygonRasterizer#setBackground(int)
     */
    public void setBackground(int argb) {
        background =argb;

    }

    /* (non-Javadoc)
     * @see de.jreality.soft.PolygonRasterizer#clear()
     */
    public void clear() {
    }
    /**
     * This should be called before any renderPolygon.
     * It writes the header.
     */
    public void start() {
        count =0;
        writer.println("%!PS-Adobe-3.0 EPSF-3.0\n%%Creator: jReality");
        writer.println("%%LanguageLevel: 3");
        writer.println("%%BoundingBox: "+xmin+" "+ymin+" "+(xmax-xmin)+" "+(ymax-ymin)+"\n%%EndComments");
        writer.println("gsave\n");
    }
    
    /**
     * This should be called after the last renderPolygon call.
     */
    public void stop() {
        writer.println(" \ngrestore");
    }

    /* (non-Javadoc)
     * @see de.jreality.soft.PolygonRasterizer#setWindow(int, int, int, int)
     */
    public void setWindow(int xmin, int xmax, int ymin, int ymax) {
        this.xmin =xmin;
        this.xmax =xmax;
        this.ymin =ymin;
        this.ymax =ymax;
    }
    public void setSize(double width, double height) {
        wh =(width)/2;
        hh =(height)/2;
        mh =Math.min(wh,hh);
        
    }
}
