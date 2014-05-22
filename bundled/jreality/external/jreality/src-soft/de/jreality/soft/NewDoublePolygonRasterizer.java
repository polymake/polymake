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

import java.util.Arrays;
/*
 * implementation notes: for speed reasons  we do not interpolate y coordinates 
 * and normals at all.
 * Moreover x is only interpolated as long as needed. 
 * All other quantities are interpolated when requested by the shading type.
 * 
 *
 */

/**
 * @version 1.0
 * @author <a href="mailto:hoffmann@math.tu-berlin.de">Tim Hoffmann</a>
 *  
 */
public class NewDoublePolygonRasterizer implements PolygonRasterizer {
    private static final double  COLOR_CH_SCALE = 1/255.;
    private static final int     COLOR_CH_MASK  = 255;
    private static final int     OPAQUE         = (COLOR_CH_MASK << 24);
    //private static final boolean[] interpolate = new boolean[Polygon.VERTEX_LENGTH];

    private static final boolean correctInterpolation = true;
    private final Colorizer colorizer;
    
    //dimensions of the image to render into:

    private int xmin = 0;
    private int xmax = 0;
    private int ymin = 0;
    private int ymax = 0;

    // store half the size of the screen:
    //minDim is the min of wh and hh;
    private double wh;
    private double hh;
    private double minDim;
    private double maxDim;
    private double[][] polygon =
        new double[Polygon.MAXPOLYVERTEX][Polygon.VERTEX_LENGTH];
    
    private final Quantity xxx = new Quantity();
    private final Quantity yyy = new Quantity();
    private final Quantity zzz = new Quantity();
    private final Quantity www = new Quantity();
    private final Quantity rrr = new Quantity();
    private final Quantity ggg = new Quantity();
    private final Quantity bbb = new Quantity();
    private final Quantity aaa = new Quantity();
    private final Quantity uuu = new Quantity();
    private final Quantity vvv = new Quantity();
    
    private final boolean interpolateY;
    private final boolean interpolateFullX;
    private boolean interpolateColor;
    private boolean interpolateW;
    private boolean interpolateA;
    private boolean interpolateUV;

    protected double transparency = 0;

    private Texture texture = null;

    private double zBuffer[];
    private int w;
    private int h;

    private int[] pixels;
    private int background;

    
    /**
     *  
     */
    public NewDoublePolygonRasterizer(int[] pixelBuf,boolean interpolateY, boolean interpolateFullX, Colorizer colorizer) {
        super();
        this.colorizer = colorizer;
        this.interpolateY = interpolateY;
        this.interpolateFullX = interpolateFullX;
        pixels=pixelBuf;
        //System.out.println(">NEW DOUBLE RASTERIZER<");
        //System.out.println("(correct interpolation = "+correctInterpolation+")");
    }
    public NewDoublePolygonRasterizer(int[] pixelBuf) {
        this(pixelBuf, false, false,null);
    }

    /*
     * (non-Javadoc)
     * 
     * @see de.jreality.soft.Renderer#renderPolygon(de.jreality.soft.Polygon,
     *      double[],int,int,int,int)
     */
    public final void renderPolygon(
        final Polygon p,
        final double[] vertexData,
        final boolean outline /*
                               * , final int xmin, final int xmax, final int
                               * ymin, final int ymax
                               */
    ) {
        transparency =
             p.getShader().getVertexShader().getTransparency() ;
        
        
        texture = p.getShader().getTexture();
        interpolateUV = texture != null;
        
        
        interpolateColor = p.getShader().interpolateColor();
        
        interpolateW =correctInterpolation&&(interpolateColor||interpolateUV);
        
        
        final int pLength = p.length;
        for (int i = 0; i < pLength; i++) {
            int pos = p.vertices[i];
            double[] pi = polygon[i];

            double w = 1 / vertexData[pos + Polygon.SW];
            double wxy = w * minDim;
            pi[Polygon.SX] =  (wh + vertexData[pos + Polygon.SX] * wxy);
            pi[Polygon.SY] = (hh - vertexData[pos + Polygon.SY] * wxy);
            pi[Polygon.SZ] =  (vertexData[pos + Polygon.SZ] * w );

            if (p.getShader() instanceof SkyboxPolygonShader)
                pi[Polygon.SZ] = 2 + 0*Double.MAX_VALUE;

            
            final double iw = (interpolateW? w: 1);
            
                pi[Polygon.SW] = iw;
                pi[Polygon.U] =
                    (vertexData[pos + Polygon.U] *  iw);
                pi[Polygon.V] =
                     (vertexData[pos + Polygon.V] *  iw);
                pi[Polygon.A] =
                     (vertexData[pos + Polygon.A] *  iw);
                
                if(interpolateColor) {
                    pi[Polygon.R] =
                         ((vertexData[pos + Polygon.R] >= 1
                            ? 1
                            : (vertexData[pos + Polygon.R] ))
                            * iw);
                    pi[Polygon.G] =
                         ((vertexData[pos + Polygon.G] >= 1
                            ? 1
                            : (vertexData[pos + Polygon.G] ))
                            * iw);
                    pi[Polygon.B] =
                         ((vertexData[pos + Polygon.B] >= 1
                            ? 1
                            : (vertexData[pos + Polygon.B] ))
                            * iw);
                }
        }
        
            if( !interpolateColor) {
                int pos = p.vertices[0];
                rrr.value = Math.min(1,vertexData[pos + Polygon.R]);
                ggg.value = Math.min(1,vertexData[pos + Polygon.G]);
                bbb.value = Math.min(1,vertexData[pos + Polygon.B]);
           }
            www.value = polygon[0][Polygon.SW];
            
            
            interpolateA = p.getShader().interpolateAlpha();
            if(interpolateA) aaa.value =1;
            
        if (outline) {
            transparency = 0;
            for (int j = 0; j < pLength - 1; j++) {
                line(
                        polygon[j][Polygon.SX],
                        polygon[j][Polygon.SY],
                        polygon[j][Polygon.SZ],
                        polygon[j + 1][Polygon.SX],
                        polygon[j + 1][Polygon.SY],
                        polygon[j + 1][Polygon.SZ],xmin,xmax,ymin,ymax);
            }
            line(
                    polygon[pLength - 1][Polygon.SX],
                    polygon[pLength - 1][Polygon.SY],
                    polygon[pLength - 1][Polygon.SZ],
                    polygon[0][Polygon.SX],
                    polygon[0][Polygon.SY],
                    polygon[0][Polygon.SZ],xmin,xmax,ymin,ymax);
        }
        
            /*
             * private int[][] tri = new int[3][];
             * tri[0] = t0; for(int i =1;i <pLength;i++) { tri[1]
             * =polygon[i-1]; tri[2] =polygon[i];
             * scanPolyI(tri,3,xmin,xmax-1,ymin,ymax-1); } tri[1] =polygon[0];
             * tri[2] =polygon[pLength-1];
             * scanPolyI(tri,3,xmin,xmax-1,ymin,ymax-1);
             */
       
        scanPolygon(polygon, pLength);

    }
    /*
     * a class for interpolatable quantities;
     * 
     * @version 1.0
     * @author timh
     *
     */
    private final class Quantity {
        double leftValue;
        double leftYIncrement;
        double rightValue;
        double rightYIncrement;
        double value;
        double xIncrement;
        
        Quantity() {
            super();
        }
        
        final  void makeXIncrement(final double dx, final double frac) {
            
                xIncrement = ((rightValue- leftValue)) * dx;
                value = leftValue + xIncrement * frac;
          
        }
        
        final void makeLeftYIncrement(final double start, final double end, final double dy, final double frac) {
            
                leftYIncrement = ((end - start)) * dy;
                leftValue = start + leftYIncrement * frac;
//                System.out.println("leftValue "+(leftValue)+" leftYIncr "+(leftYIncrement)+" frac "+(frac)+" \nstart "+(start)+" end "+end);
//                System.out.println("end - start "+(end - start));
//                System.out.println("(end - start)*dy "+((end - start)*dy));
//                System.out.println("1*dy = " +(dy));
                
        }
        final void makeRightYIncrement(final double start, final double end, final double dy, final double frac) {
                rightYIncrement = ((end - start)) * dy;
                rightValue = start + rightYIncrement * frac;
        }
        final void incrementY() {
                leftValue += leftYIncrement;
                rightValue += rightYIncrement;
        }

        final void incrementX() {
                value += xIncrement;  
        }
        
        public void dump() {
            System.out.println("dump:");
            System.out.println("value      :"+value);
            System.out.println("xIncr      :"+xIncrement);
            System.out.println("leftValue  :"+leftValue);
            System.out.println("leftIncr   :"+leftYIncrement);
            System.out.println("rightValue :"+rightValue);
            System.out.println("rightIncr  :"+rightYIncrement);
            
        }
    }
   

    private final void scanPolygon(
        final double[][] p,
        final int plength
        ) {
        int  y, ly, ry;
        int top = 0;
        
        //TODO remove this test?
        if (plength > Polygon.MAXPOLYVERTEX) {
            System.err.println(
                "scanPoly: polygon had to many vertices: " + plength);
            return;
        }
        //System.out.println("scan poly");

        double minY = Double.MAX_VALUE;
        for (int i = 0; i < plength; i++) {
            if (p[i][Polygon.SY] < minY) {
                minY = p[i][Polygon.SY];
                top = i;
            }
        }
        // now we have the topmost vertex.
        int li = top;
        int ri = top;
        int rem = plength;

        y =(int)Math.ceil(minY -.5);
        
        ly = ry = y - 1;

        while (rem > 0) {
            int i =li;
            while (ly <= y && rem > 0) {
                rem--;
                li = i;
                i = i - 1;
                if (i < 0)
                    i = plength - 1;
                ly = (int)Math.floor(p[i][Polygon.SY] +.5);
            }
            if(i!=li) {
                makeYIncrement(p[li], p[i], y,true);
                li = i;
            }
            i =ri;
            while (ry <= y && rem > 0) {
                rem--;
                ri = i;
                i = i + 1;
                if (i >= plength)
                    i = 0;
                ry = (int)Math.floor(p[i][Polygon.SY] + .5) ;
            }
            if(i!=ri) {
                makeYIncrement(p[ri], p[i], y,false);
                ri = i;
            }
            //System.out.println("    y "+y+" ly "+ly+" ry "+ry);

            while (y < ly && y < ry) {
                
                if (y >= ymin && y < ymax) {
                    scanline(y);
                }
                y++;
                xxx.incrementY();
                if(interpolateY)
                    yyy.incrementY();
                zzz.incrementY();
                if(interpolateW)
                    www.incrementY();
                if(interpolateColor) {
                    rrr.incrementY();
                    ggg.incrementY();
                    bbb.incrementY();
                }
                if(interpolateA) aaa.incrementY();
                if(interpolateUV) {
                    uuu.incrementY();
                    vvv.incrementY();
                }
            }
        }
    }

 

    
    private final void makeYIncrement(final double[] p1, final double[] p2, final int y, final boolean left) {
        
        final double p1y = p1[Polygon.SY];
        final double dy = 1/Math.max(1, p2[Polygon.SY] - p1y);
        //if (dy <1) 
        //    dy = 1;
        //dy =1/dy;
        final double frac = y+ .5 - p1y;
        if(left) {
            xxx.makeLeftYIncrement(p1[Polygon.SX],p2[Polygon.SX],dy,frac);
            if(interpolateY) 
                yyy.makeLeftYIncrement(p1[Polygon.SY],p2[Polygon.SY],dy,frac);
            zzz.makeLeftYIncrement(p1[Polygon.SZ],p2[Polygon.SZ],dy,frac);
            if(interpolateW) www.makeLeftYIncrement(p1[Polygon.SW],p2[Polygon.SW],dy,frac);
            if(interpolateColor) {
                rrr.makeLeftYIncrement(p1[Polygon.R],p2[Polygon.R],dy,frac);
                ggg.makeLeftYIncrement(p1[Polygon.G],p2[Polygon.G],dy,frac);
                bbb.makeLeftYIncrement(p1[Polygon.B],p2[Polygon.B],dy,frac);
            }
            if(interpolateA) aaa.makeLeftYIncrement(p1[Polygon.A],p2[Polygon.A],dy,frac);
            if(interpolateUV) {
                uuu.makeLeftYIncrement(p1[Polygon.U],p2[Polygon.U],dy,frac);
                vvv.makeLeftYIncrement(p1[Polygon.V],p2[Polygon.V],dy,frac);
            }
        } else {
            xxx.makeRightYIncrement(p1[Polygon.SX],p2[Polygon.SX],dy,frac);
            if(interpolateY)
                yyy.makeRightYIncrement(p1[Polygon.SY],p2[Polygon.SY],dy,frac);
            zzz.makeRightYIncrement(p1[Polygon.SZ],p2[Polygon.SZ],dy,frac);
            if(interpolateW) www.makeRightYIncrement(p1[Polygon.SW],p2[Polygon.SW],dy,frac);
            if(interpolateColor) {
                rrr.makeRightYIncrement(p1[Polygon.R],p2[Polygon.R],dy,frac);
                ggg.makeRightYIncrement(p1[Polygon.G],p2[Polygon.G],dy,frac);
                bbb.makeRightYIncrement(p1[Polygon.B],p2[Polygon.B],dy,frac);
            }
            if(interpolateA) aaa.makeRightYIncrement(p1[Polygon.A],p2[Polygon.A],dy,frac);
            if(interpolateUV) {
                uuu.makeRightYIncrement(p1[Polygon.U],p2[Polygon.U],dy,frac);
                vvv.makeRightYIncrement(p1[Polygon.V],p2[Polygon.V],dy,frac);
            }
        }
    }

    private final void scanline(final int y) {
        final double l = xxx.leftValue, r = xxx.rightValue;
        final boolean lr = l<r;
        int lx, rx;
        if(lr) {
            lx = (int)Math.ceil(l -.5);
            rx = (int) Math.floor(r -.5);
            if(lx < xmin) lx = xmin;
            if(rx >= xmax) rx = xmax-1;
            if(lx > rx) return;
        }else {
            lx = (int)Math.floor(l -.5);
            rx = (int) Math.ceil(r -.5);
            if(rx < xmin) rx = xmin;
            if(lx >= xmax) lx = xmax-1;
            if(rx > lx) return;
        }
        
        makeXincrement(lx);
        final int inc=(lr)? 1: -1;
        final int posOff=y*w;
        colorize(lx+posOff);
        for(int x = lx+inc+posOff; lr? (x <= rx+posOff): (x >= rx+posOff); x+=inc) {
            //colorize(x+posOff);
            if(interpolateFullX) xxx.incrementX();
            if(interpolateY) yyy.incrementX();
            zzz.incrementX();
            
            if(interpolateW) www.incrementX();
            if(interpolateColor){
                rrr.incrementX();
                ggg.incrementX();
                bbb.incrementX();
            }
            if(interpolateA) aaa.incrementX();
            if(interpolateUV) {
                uuu.incrementX();
                vvv.incrementX();
            }
            colorize(x/*+posOff*/);
//            for(int k = 2; k<INTERPOLATION_LENGTH; k++) 
//            quantities[k].incrementX();
        }
    }
    
    private final void makeXincrement(final int leftBound) {
        final double dx = 1/Math.max(1, Math.abs(xxx.rightValue - xxx.leftValue));
        
        final double frac = Math.abs( (leftBound  + .5 - xxx.leftValue));
        if(interpolateFullX) xxx.makeXIncrement(dx,frac);
        if(interpolateY) yyy.makeXIncrement(dx,frac);
        zzz.makeXIncrement(dx,frac);
        if(interpolateW) www.makeXIncrement(dx,frac);
        if(interpolateColor) {
            rrr.makeXIncrement(dx,frac);
            ggg.makeXIncrement(dx,frac);
            bbb.makeXIncrement(dx,frac);
        }
        if(interpolateA) aaa.makeXIncrement(dx,frac);
        if(interpolateUV) {
            uuu.makeXIncrement(dx,frac);
            vvv.makeXIncrement(dx,frac);
        }
    }
    
    private final int[] color = new int[4];
    
    public static abstract class Colorizer {
        public abstract void colorize(final int[] pixels, final double[] zBuff, final int pos, final double[] data,boolean interpolateUV);
    }
    final double [] data = new double[Polygon.VERTEX_LENGTH];
    private final void colorize(final int pos) {
        
        final double z = zzz.value;
        if (z >= zBuffer[pos]) return;
        
        final double ww =www.value;

        final double factor = interpolateColor ?(255./ww):255.;
        double r = /*(int)*/(rrr.value*factor);
        double g = /*(int)*/(ggg.value*factor);
        double b = /*(int)*/(bbb.value*factor);
        
        double omt = (1 - transparency);
        if(interpolateA) omt = (omt*aaa.value) / ww;

//        if(r<0||r>255) System.out.println("R: "+r+" W "+ww);
//        if(g<0||g>255) System.out.println("G: "+r);
//        if(b<0||b>255) System.out.println("B: "+r);
//        if(r<0||r>255)  www.dump();

        if (interpolateUV) {
            final int[] color=this.color;
            final double WW = ((double)ww);
            color[0] = (int)r;
            color[1] = (int)g;
            color[2] = (int)b;
            color[3] = (int)omt;
            texture.getColor(
                    uuu.value /WW, vvv.value /WW,pos%w,pos/w, color);
//            omt *= color[3]*COLOR_CH_SCALE;
//            r   *= color[0]*COLOR_CH_SCALE;
//            g   *= color[1]*COLOR_CH_SCALE;
//            b   *= color[2]*COLOR_CH_SCALE;   
            omt = color[3]*COLOR_CH_SCALE;
            r   = color[0]*COLOR_CH_SCALE;
            g   = color[1]*COLOR_CH_SCALE;
            b   = color[2]*COLOR_CH_SCALE;     
        }
       

        if(omt<1.D) {
            final int sample = pixels[pos];

            double t = (1.-omt);
            
            int sb =  sample      & COLOR_CH_MASK;
            int sg = (sample>>8)  & COLOR_CH_MASK;
            int sr = (sample>>16) & COLOR_CH_MASK;
            
            r = r*omt + t*sr;
            g = g*omt + t*sg;
            b = b*omt + t*sb;
        }
//        System.out.println("r "+r);
//        System.out.println("g "+g);
//        System.out.println("b "+b);
        if(colorizer != null) {
            data[Polygon.SX] = xxx.value/ww;
            data[Polygon.SY] = yyy.value/ww;
            data[Polygon.SZ] = z;
            data[Polygon.R] = r;
            data[Polygon.G] = g;
            data[Polygon.B] = b;
            data[Polygon.A] = aaa.value/ww;
            data[Polygon.U] = uuu.value/ww;
            data[Polygon.V] = vvv.value/ww;
            colorizer.colorize(pixels,zBuffer, pos, data,interpolateUV);
        } else {
        pixels[pos]  = OPAQUE |  ((int)r <<16) |  ((int)g <<8) | (int)b;
        zBuffer[pos]= z;
        }
    }
    



    //
    //
    // line with z
    //
    //
    private static final double ZEPS = -.0001;
    private final void line(
            final double x1,
            final double y1,
            final double z1,
            final double x2,
            final double y2,
            final double z2, final int xmin, final int xmax, final int ymin, final int ymax) {
        double dirX = x2-x1;
        double dirY = y2-y1;
        double dirZ = z2-z1;
        double l = ((double)dirX)*dirX + ((double)dirY)*dirY;
        l = Math.sqrt(l);
        if(l <1) return;
        dirX /= l;
        dirY /= l;
        dirZ/= l;

//          scanPolyNoColor(new double[][] {{x1-dirY,y1+dirX,z1+ZEPS,0,0,0,0,0,1},{x1+dirY,y1-dirX,z1-ZEPS,0,0,0,0,0,1},
//              {x2+dirY,y2-dirX,z2-ZEPS,0,0,0,0,0,1},{x2-dirY,y2+dirX,z2+ZEPS,0,0,0,0,0,1}},4,xmin,xmax-1,ymin,ymax-1);
        double zz1 =z1-dirZ+ZEPS;
        double zz2 = z2+dirZ+ZEPS;
        
        //TODO: find a replacement for the following two lines:
        //interpolateColor   = false;
        //interpolateTexture = false;
        
        scanPolygon(new double[][] {
                {x1-dirX-dirY,y1-dirY+dirX,zz1,0,0,0,0,0,1},
                {x1-dirX+dirY,y1-dirY-dirX,zz1,0,0,0,0,0,1},
                {x2+dirX+dirY,y2+dirY-dirX,zz2,0,0,0,0,0,1},
                {x2+dirX-dirY,y2+dirY+dirX,zz2,0,0,0,0,0,1}},4);

    }

    public final void setWindow(int xmin, int xmax, int ymin, int ymax) {
        this.xmin = xmin;
        this.xmax = xmax;
        this.ymin = ymin;
        this.ymax = ymax;
        //        w = xmax - xmin;
        //        h = ymax -ymin;
        int nw=xmax-xmin, nh=ymax-ymin;
        if(nw!=w||nh!=h) {
            w=nw;
            h=nh;
            final int numPx=w*h;
            zBuffer=new double[numPx];
        }

    }
    public final void setSize(double width, double height) {
        wh = ( width)  / 2.;
        hh = ( height)  / 2.;
        minDim = Math.min(wh, hh);
        maxDim = Math.max(wh, hh);
       
    }

    public void setBackground(int argb) {
        background=argb;
    }
    
    public final void clear() {
        Arrays.fill(zBuffer, Integer.MAX_VALUE);
        Arrays.fill(pixels, background);
        
    }

    public final void start() {
    }

    public final  void stop() {
    }
    public boolean isInterpolateUV() {
        return interpolateUV;
    }

}
