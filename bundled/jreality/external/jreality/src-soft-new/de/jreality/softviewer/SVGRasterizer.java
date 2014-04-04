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


package de.jreality.softviewer;

import java.awt.Color;
import java.io.PrintWriter;

/**
 * This is a SVG writer for the software renderer. At the moment it needs
 * the PolygonPipline to be configured to sort <em>all</em> Polygons (and
 * not only the transparent ones) since it uses a simple painter's 
 * algorithm. No polygon intersections are done. It is
 * first come first paint at the moment.
 * @version 1.0
 * @author <a href="mailto:hoffmann@math.tu-berlin.de">Tim Hoffmann</a>
 *
 */
public class SVGRasterizer extends TriangleRasterizer {
    public static final String NONE = "none";
    private boolean useGradients = false;
    private int background;
    private int xmin;
    private int xmax;
    private int ymin;
    private int ymax;
    

    private double[][] triangle = new double[3][Polygon.VERTEX_LENGTH];
    protected double transparency = 0;
    protected double oneMinusTransparency = 1;
    
    private PrintWriter writer;
    private int count;
    private double wh;
    private double hh;
    private double mh;
    private boolean transparencyEnabled;
    /**
     * 
     */
    public SVGRasterizer( PrintWriter w) {
        super();
       writer =w;

    }

    /* (non-Javadoc)
     * @see de.jreality.soft.PolygonRasterizer#renderPolygon(de.jreality.soft.Polygon, double[], boolean)
     */
    public void renderTriangle(Triangle t,boolean outline) {
        transparency = t.getTransparency();
        if(!transparencyEnabled)
            transparency = (transparency < 1 ? 0:1);
        oneMinusTransparency = 1 - transparency;

      

        double[] t0 = new double[Polygon.VERTEX_LENGTH];
        
       
        for (int i = 0; i < 3; i++) {
            double[] pi= triangle[i];
            double[] vertexData = t.getPoint(i);
            //System.out.println("render Poly"+i+" "+vertexData[pos+Polygon.SX]+" pos"+pos);
            double w = 1/vertexData[Polygon.SW];
            double wxy =w*mh;
            pi[Polygon.SX] =(wh + vertexData[Polygon.SX] * wxy);
            pi[Polygon.SY] =(hh - vertexData[Polygon.SY] * wxy);
            pi[Polygon.SZ] =(vertexData[Polygon.SZ] * w) ;


            pi[Polygon.R] = ((vertexData[Polygon.R] > 1 ? 255 : (255*vertexData[Polygon.R] )));
            pi[Polygon.G] = ((vertexData[Polygon.G] > 1 ? 255 : (255*vertexData[Polygon.G] )));
            pi[Polygon.B] = ((vertexData[Polygon.B] > 1 ? 255 : (255*vertexData[Polygon.B])));
            

            t0[Polygon.SX] += pi[Polygon.SX];
            t0[Polygon.SY] += pi[Polygon.SY];
            t0[Polygon.SZ] += pi[Polygon.SZ];
            t0[Polygon.R] += pi[Polygon.R];
            t0[Polygon.G] += pi[Polygon.G];
            t0[Polygon.B] += pi[Polygon.B];
        }

        t0[Polygon.SX] /= 3;
        t0[Polygon.SY] /= 3;
        t0[Polygon.R]  /= 3;
        t0[Polygon.G]  /= 3;
        t0[Polygon.B]  /= 3;
        
        
        String col;
        if(useGradients && t.isInterpolateColor()) {
            double[][] pol = new double[3][];
            pol[2] =t0; 
            String t0Col = colorString((int)t0[Polygon.R],(int)t0[Polygon.G],(int)t0[Polygon.B]);
            
            for(int n=0;n<3-1;n++) {
                pol[0] =triangle[n];
                pol[1] =triangle[n+1];
                writeGradients(pol);
                col ="url(#"+count+"a)";
                col = t0Col;
                writePolygon(pol, 3, col, NONE);
                col ="url(#"+count+"b)";
                writePolygon(pol, 3, col, NONE);
                col ="url(#"+count+"c)";
                writePolygon(pol, 3, col, NONE);
                
                
                count++;
            }
            pol[0] =triangle[3-1];
            pol[1] =triangle[0];
            writeGradients(pol);
            col ="url(#"+count+"a)";
            col =t0Col;
            writePolygon(pol, 3, col, NONE);
            col ="url(#"+count+"b)";
            writePolygon(pol, 3, col, NONE);
            col ="url(#"+count+"c)";
            writePolygon(pol, 3, col, NONE);
            count++;
//            if(t.isOutline())
//                writePolygon(triangle, 3, "none", "black");
            
        }
        else {
            //col = colorString((int)polygon[0][Polygon.R],(int)polygon[0][Polygon.G],(int)polygon[0][Polygon.B]);
            col = colorString((int)t0[Polygon.R],(int)t0[Polygon.G],(int)t0[Polygon.B]);
            
            //TODO decide how to handle the stroke:NONE or col
            //it seems that NONE is good for transparent but col is better for opaque
            writePolygon(triangle, 3, col, /*p.getShader().isOutline()?"black":NONE*/ NONE);
            //writePolygon(triangle, 3, col, /*p.getShader().isOutline()?"black":NONE*/ col);
        }
        
        
        
        count++;
    }

    
    private void writeGradients(double[][] p) {
        /*
        writer.print("<linearGradient gradientUnits=\"userSpaceOnUse\" ");
        writer.print("id=\""+count+"a\" ");
        writer.print("x1=\""+p[0][Polygon.SX]+"\" ");
        writer.print("y1=\""+p[0][Polygon.SY]+"\" ");
        writer.print("x2=\""+p[1][Polygon.SX]+"\" ");
        writer.print("y2=\""+p[1][Polygon.SY]+"\" ");
        writer.println(">");
        
        writer.print("<stop offset=\"0\" stop-color=\""+
                colorString((int)p[0][Polygon.R],(int)p[0][Polygon.G],(int)p[0][Polygon.B])+"\" ");
        writer.print("/>");
        writer.print("<stop offset=\"1\" stop-color=\""+
                colorString((int)p[1][Polygon.R],(int)p[1][Polygon.G],(int)p[1][Polygon.B])+"\" ");
        writer.print("/>");
        
        writer.println("</linearGradient>");
        */
        //writeGradient(p[0],p[1],p[2],""+count+"a");
        writeGradient(p[1],p[2],p[0],""+count+"b");
        writeGradient(p[2],p[0],p[1],""+count+"c");
    }

    private void writeGradient(double[] p0, double [] p1, double[] p2, String name) {
        double[] proj =project(p0,p1,p2);
        
        writer.print("<linearGradient gradientUnits=\"userSpaceOnUse\" ");
        writer.print("id=\""+name+"\" ");
        writer.print("x1=\""+proj[0]+"\" ");
        writer.print("y1=\""+proj[1]+"\" ");
        writer.print("x2=\""+p2[Polygon.SX]+"\" ");
        writer.print("y2=\""+p2[Polygon.SY]+"\" ");
        writer.println(">");
        
        String col = 
            colorString((int)p2[Polygon.R],(int)p2[Polygon.G],(int)p2[Polygon.B]);
        String col2 = 
            colorString((int)(oneMinusTransparency*p2[Polygon.R]),(int)(oneMinusTransparency*p2[Polygon.G]),(int)(oneMinusTransparency*p2[Polygon.B]));
        
        writer.print("<stop offset=\"0\" stop-color=\""+col+"\" stop-opacity=\"0\" ");
        writer.print("/>");
        writer.print("<stop offset=\".99\" stop-color=\""+col+"\" stop-opacity=\""+1+"\" ");
        writer.print("/>");
        
//        writer.print("<stop offset=\"0\" stop-color=\""+"currentColor"+"\" stop-opacity=\"1\" ");
//        writer.print("/>");
//        writer.print("<stop offset=\"1\" stop-color=\""+col+"\" stop-opacity=\""+1+"\" ");
//        writer.print("/>");
        
        writer.println("</linearGradient>");
    }

    private void writePolygon(double[][] polygon, int pLength, String color, String outlineColor) {
        writer.print("<polygon ");
        writer.print("stroke=\""+outlineColor+"\" ");
        if(!outlineColor.equals("none"))
            writer.print("stroke-opacity=\""+oneMinusTransparency+"\" ");
        
        writer.print("fill=\""+color+"\" ");
        
        writer.print("fill-opacity=\""+oneMinusTransparency+"\" ");
        
        writer.print("points=\"");
        for(int i =0;i<pLength;i++) {
            writer.print(" "+polygon[i][Polygon.SX]);
            writer.print(" "+polygon[i][Polygon.SY]);
        }
        writer.print("\"");
        writer.println(" />");
    }

    /**
     * projects p2 onto the line through p0 p1.
     * @param polygon2
     * @return
     */
    private double[] project(double[] p0, double [] p1, double[] p2) {
        double[] r =new double[3];
        double d0 =p1[Polygon.SX]-p0[Polygon.SX];
        double d1 =p1[Polygon.SY]-p0[Polygon.SY];
        
        double e0 =p2[Polygon.SX]-p0[Polygon.SX];
        double e1 =p2[Polygon.SY]-p0[Polygon.SY];
        
        double normS =(d0*d0 +d1*d1);
        double s =d0*e0 + d1*e1;
        double l;
        if(normS != 0) {
            l = s/normS;
        d0 *= l;
        d1 *= l;
        }else {
            l = d0 = d1 = 0;
        }
        r[0] = p0[Polygon.SX]+d0;
        r[1] = p0[Polygon.SY]+d1;
        r[2] = l/Math.sqrt(normS);
        return r;
    }

    /**
     * @param i
     * @param j
     * @param k
     * @return
     */
    private String colorString(int r, int g, int b) {
        String sr =Integer.toHexString(r);
        sr =sr.length()==1?"0"+sr:sr;
        String sg =Integer.toHexString(g);
        sg =sg.length()==1?"0"+sg:sg;
        String sb =Integer.toHexString(b);
        sb =sb.length()==1?"0"+sb:sb;
        return "#"+sr+sg+sb;
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
    public void clear(boolean clearBackground) {
    }
    /**
     * This should be called before any renderPolygon.
     * It writes the header.
     */
    public void start() {
        count =0;
        writer.println(
                "<svg color-interpolation=\"sRGB\" color-rendering=\"auto\" fill=\"none\""+
                "    fill-opacity=\"1\" font-family=\"\'Arial\'\" font-size=\"12\" font-style=\"normal\""+
                "    font-weight=\"normal\"  "+
                " viewBox=\""+xmin+" "+ymin+" "+(xmax-xmin)+" "+(ymax-ymin)+" \" "+
                " image-rendering=\"auto\" "+
                "    shape-rendering=\"auto\" stroke=\"black\" stroke-dasharray=\"none\" "+
                "    stroke-dashoffset=\"0\" stroke-linecap=\"square\" stroke-linejoin=\"bevel\""+
                "    stroke-miterlimit=\"10\" stroke-opacity=\"1\" stroke-width=\"1\" "+
                "    text-rendering=\"auto\" >"        
        );
    }
    
    /**
     * This should be called after the last renderPolygon call.
     */
    public void stop() {
        writer.println("</svg>");
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

    @Override
    public int getBackground() {
        return background;
    }

    @Override
    public void setBackgroundColors(Color[] c) {
        // TODO make a background color gradinet
        
    }

    @Override
    public void setTransparencyEnabled(boolean transparencyEnabled) {
        this.transparencyEnabled = transparencyEnabled;
        
    }

    @Override
    public double getMinDim() {
        return 2*mh;
    }
}
