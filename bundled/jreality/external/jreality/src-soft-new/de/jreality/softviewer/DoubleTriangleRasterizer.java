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
import java.util.Arrays;

import de.jreality.backends.texture.Texture;
import de.jreality.softviewer.shader.OutlineImager;
import de.jreality.util.Secure;
import de.jreality.util.SystemProperties;

/*
 * implementation notes: for speed reasons we do not interpolate y coordinates
 * and normals at all. Moreover x is only interpolated as long as needed. All
 * other quantities are interpolated when requested by the shading type.
 */

/**
 * @version 1.0
 * @author <a href="mailto:hoffmann@math.tu-berlin.de">Tim Hoffmann</a>
 */
public class DoubleTriangleRasterizer extends TriangleRasterizer {
    private static final double COLOR_CH_SCALE = 1 / 255.;

    private static final int COLOR_CH_MASK = 255;

    private static final int OPAQUE = (COLOR_CH_MASK << 24);

    // private static final boolean[] interpolate = new
    // boolean[Polygon.VERTEX_LENGTH];

    private static final boolean correctInterpolation = true;

    private  boolean interpolateNormals = false;
    
    //private final Colorizer colorizer;

    // dimensions of the image to render into:

    private int xmin = 0;

    private int xmax = 0;

    private int ymin = 0;

    private int ymax = 0;

    // store half the size of the screen:
    // minDim is the min of wh and hh;
    private double wh;

    private double hh;

    private double minDim;

    private double maxDim;

    private double[][] polygon = new double[3][Polygon.VERTEX_LENGTH];

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

    private final Quantity nnx = new Quantity();

    private final Quantity nny = new Quantity();

    private final Quantity nnz = new Quantity();

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

private Imager imager = null;
    
    {
        try {
            String img = Secure.getProperty(SystemProperties.SOFT_IMAGER);
            //if (img != null && img.equals("hatch")) imager = new HatchImager();
            if (img != null && img.equals("outline")) imager = new OutlineImager();
        } catch (SecurityException se) {
            //webstart
        }
    }
    
    /**
     *  
     */
    public DoubleTriangleRasterizer(int[] pixelBuf, boolean interpolateY,
            boolean interpolateFullX, Colorizer colorizer) {
        super();
        //this.colorizer = colorizer;
        this.interpolateY = interpolateY;
        this.interpolateFullX = interpolateFullX;
        pixels = pixelBuf;
        // System.out.println(">NEW DOUBLE RASTERIZER<");
        // System.out.println("(correct interpolation =
        // "+correctInterpolation+")");
    }

    public DoubleTriangleRasterizer(int[] pixelBuf) {
        this(pixelBuf, false, false, null);
    }

    @Override
    public final void renderTriangle(final Triangle t, final boolean outline) {
        transparency = t.getTransparency();

        texture = t.getTexture();
        interpolateUV = texture != null;

        interpolateColor = t.isInterpolateColor();
        interpolateNormals = texture!= null && texture.needsNormals();
        
        
        interpolateW = correctInterpolation
                && (interpolateColor || interpolateUV);

        for (int i = 0; i < 3; i++) {
            double vertexData[] = t.getPoint(i);
            double[] pi = polygon[i];
            double w = 1 / vertexData[Polygon.SW];
            double wxy = w * minDim;
            pi[Polygon.SX] = (wh + vertexData[Polygon.SX] * wxy);
            pi[Polygon.SY] = (hh - vertexData[Polygon.SY] * wxy);
            pi[Polygon.SZ] = (vertexData[Polygon.SZ] * w);

            if (t.isSkybox())
                pi[Polygon.SZ] = 2 + 0 * Double.MAX_VALUE;

            final double iw = (interpolateW ? w : 1);
            //final double iw = (interpolateW ?  1/pi[Polygon.SZ] : 1);
            
            pi[Polygon.SW] = iw;
            pi[Polygon.U] = (vertexData[Polygon.U] * iw);
            pi[Polygon.V] = (vertexData[Polygon.V] * iw);
            pi[Polygon.A] = (vertexData[Polygon.A] * iw);

            if (interpolateColor) {
                pi[Polygon.R] = ((vertexData[Polygon.R] >= 1 ? 1
                        : (vertexData[Polygon.R])) * iw);
                pi[Polygon.G] = ((vertexData[Polygon.G] >= 1 ? 1
                        : (vertexData[Polygon.G])) * iw);
                pi[Polygon.B] = ((vertexData[Polygon.B] >= 1 ? 1
                        : (vertexData[Polygon.B])) * iw);
            }
            
            if(interpolateNormals) {
                pi[Polygon.NX] = (vertexData[Polygon.NX] * iw);
                pi[Polygon.NY] = (vertexData[Polygon.NY] * iw);
                pi[Polygon.NZ] = (vertexData[Polygon.NZ] * iw);
            }
        } 
        if (!interpolateColor) {
            double[] vertexData = t.getCenter();
            rrr.value = Math.min(1, vertexData[Polygon.R]);
            ggg.value = Math.min(1, vertexData[Polygon.G]);
            bbb.value = Math.min(1, vertexData[Polygon.B]);
            aaa.value = Math.min(1, vertexData[Polygon.A]);
        }
        www.value = polygon[0][Polygon.SW];

        interpolateA = t.isInterpolateAlpha();
        if (!interpolateA)
            aaa.value = 1;

       if (outline) {
            transparency = 0;
            for (int j = 0; j < 3 - 1; j++) {
                line(polygon[j][Polygon.SX], polygon[j][Polygon.SY],
                        polygon[j][Polygon.SZ], polygon[j + 1][Polygon.SX],
                        polygon[j + 1][Polygon.SY], polygon[j + 1][Polygon.SZ],
                        xmin, xmax, ymin, ymax);
            }
            line(polygon[3 - 1][Polygon.SX], polygon[3 - 1][Polygon.SY],
                    polygon[3 - 1][Polygon.SZ], polygon[0][Polygon.SX],
                    polygon[0][Polygon.SY], polygon[0][Polygon.SZ], xmin, xmax,
                    ymin, ymax);
        }

        /*
         * private int[][] tri = new int[3][]; tri[0] = t0; for(int i =1;i
         * <pLength;i++) { tri[1] =polygon[i-1]; tri[2] =polygon[i];
         * scanPolyI(tri,3,xmin,xmax-1,ymin,ymax-1); } tri[1] =polygon[0];
         * tri[2] =polygon[pLength-1]; scanPolyI(tri,3,xmin,xmax-1,ymin,ymax-1);
         */

        scanPolygon(polygon);

    }

    /*
     * a class for interpolatable quantities;
     * 
     * @version 1.0 @author timh
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

        final void makeXIncrement(final double dx, final double frac) {

            xIncrement = ((rightValue - leftValue)) * dx;
            value = leftValue + xIncrement * frac;

        }

        final void makeLeftYIncrement(final double start, final double end,
                final double dy, final double frac) {

            leftYIncrement = ((end - start)) * dy;
            leftValue = start + leftYIncrement * frac;
            // System.out.println("leftValue "+(leftValue)+" leftYIncr
            // "+(leftYIncrement)+" frac "+(frac)+" \nstart "+(start)+" end
            // "+end);
            // System.out.println("end - start "+(end - start));
            // System.out.println("(end - start)*dy "+((end - start)*dy));
            // System.out.println("1*dy = " +(dy));

        }

        final void makeRightYIncrement(final double start, final double end,
                final double dy, final double frac) {
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
            System.out.println("value      :" + value);
            System.out.println("xIncr      :" + xIncrement);
            System.out.println("leftValue  :" + leftValue);
            System.out.println("leftIncr   :" + leftYIncrement);
            System.out.println("rightValue :" + rightValue);
            System.out.println("rightIncr  :" + rightYIncrement);

        }
    }

    private final void scanPolygon(final double[][] p) {
        int y, ly, ry;
        int top = 0;

        // System.out.println("scan poly");

        double minY = Double.MAX_VALUE;
        for (int i = 0; i < 3; i++) {
            if (p[i][Polygon.SY] < minY) {
                minY = p[i][Polygon.SY];
                top = i;
            }
        }
        // now we have the topmost vertex.
        int li = top;
        int ri = top;
        int rem = 3;

        y = (int) Math.ceil(minY - .5);

        ly = ry = y - 1;

        while (rem > 0) {
            int i = li;
            while (ly <= y && rem > 0) {
                rem--;
                li = i;
                i = i - 1;
                if (i < 0)
                    i = 3 - 1;
                ly = (int) Math.floor(p[i][Polygon.SY] + .5);
            }
            if (i != li) {
                makeYIncrement(p[li], p[i], y, true);
                li = i;
            }
            i = ri;
            while (ry <= y && rem > 0) {
                rem--;
                ri = i;
                i = i + 1;
                if (i >= 3)
                    i = 0;
                ry = (int) Math.floor(p[i][Polygon.SY] + .5);
            }
            if (i != ri) {
                makeYIncrement(p[ri], p[i], y, false);
                ri = i;
            }
            // System.out.println(" y "+y+" ly "+ly+" ry "+ry);

            while (y < ly && y < ry) {

                if (y >= ymin && y < ymax) {
                    scanline(y);
                }
                y++;
                xxx.incrementY();
                if (interpolateY)
                    yyy.incrementY();
                zzz.incrementY();
                if (interpolateW)
                    www.incrementY();
                if (interpolateColor) {
                    rrr.incrementY();
                    ggg.incrementY();
                    bbb.incrementY();
                }
                if (interpolateA)
                    aaa.incrementY();
                if (interpolateUV) {
                    uuu.incrementY();
                    vvv.incrementY();
                }
                if(interpolateNormals) {
                    nnx.incrementY();
                    nny.incrementY();
                    nnz.incrementY();
                }
            }
        }
    }

    private final void makeYIncrement(final double[] p1, final double[] p2,
            final int y, final boolean left) {

        final double p1y = p1[Polygon.SY];
        final double dy = 1 / Math.max(1, p2[Polygon.SY] - p1y);
        // if (dy <1)
        // dy = 1;
        // dy =1/dy;
        final double frac = y + .5 - p1y;
        if (left) {
            xxx.makeLeftYIncrement(p1[Polygon.SX], p2[Polygon.SX], dy, frac);
            if (interpolateY)
                yyy
                        .makeLeftYIncrement(p1[Polygon.SY], p2[Polygon.SY], dy,
                                frac);
            zzz.makeLeftYIncrement(p1[Polygon.SZ], p2[Polygon.SZ], dy, frac);
            if (interpolateW)
                www
                        .makeLeftYIncrement(p1[Polygon.SW], p2[Polygon.SW], dy,
                                frac);
            if (interpolateColor) {
                rrr.makeLeftYIncrement(p1[Polygon.R], p2[Polygon.R], dy, frac);
                ggg.makeLeftYIncrement(p1[Polygon.G], p2[Polygon.G], dy, frac);
                bbb.makeLeftYIncrement(p1[Polygon.B], p2[Polygon.B], dy, frac);
            }
            if (interpolateA)
                aaa.makeLeftYIncrement(p1[Polygon.A], p2[Polygon.A], dy, frac);
            if (interpolateUV) {
                uuu.makeLeftYIncrement(p1[Polygon.U], p2[Polygon.U], dy, frac);
                vvv.makeLeftYIncrement(p1[Polygon.V], p2[Polygon.V], dy, frac);
            }
            if(interpolateNormals) {
                nnx.makeLeftYIncrement(p1[Polygon.NX], p2[Polygon.NX], dy, frac);
                nny.makeLeftYIncrement(p1[Polygon.NY], p2[Polygon.NY], dy, frac);
                nnz.makeLeftYIncrement(p1[Polygon.NZ], p2[Polygon.NZ], dy, frac);
            }
        } else {
            xxx.makeRightYIncrement(p1[Polygon.SX], p2[Polygon.SX], dy, frac);
            if (interpolateY)
                yyy.makeRightYIncrement(p1[Polygon.SY], p2[Polygon.SY], dy,
                        frac);
            zzz.makeRightYIncrement(p1[Polygon.SZ], p2[Polygon.SZ], dy, frac);
            if (interpolateW)
                www.makeRightYIncrement(p1[Polygon.SW], p2[Polygon.SW], dy,
                        frac);
            if (interpolateColor) {
                rrr.makeRightYIncrement(p1[Polygon.R], p2[Polygon.R], dy, frac);
                ggg.makeRightYIncrement(p1[Polygon.G], p2[Polygon.G], dy, frac);
                bbb.makeRightYIncrement(p1[Polygon.B], p2[Polygon.B], dy, frac);
            }
            if (interpolateA)
                aaa.makeRightYIncrement(p1[Polygon.A], p2[Polygon.A], dy, frac);
            if (interpolateUV) {
                uuu.makeRightYIncrement(p1[Polygon.U], p2[Polygon.U], dy, frac);
                vvv.makeRightYIncrement(p1[Polygon.V], p2[Polygon.V], dy, frac);
            }
            if(interpolateNormals) {
                nnx.makeRightYIncrement(p1[Polygon.NX], p2[Polygon.NX], dy, frac);
                nny.makeRightYIncrement(p1[Polygon.NY], p2[Polygon.NY], dy, frac);
                nnz.makeRightYIncrement(p1[Polygon.NZ], p2[Polygon.NZ], dy, frac);
            }
        }
    }

    private final void scanline(final int y) {
        final double l = xxx.leftValue, r = xxx.rightValue;
        final boolean lr = l < r;
        int lx, rx;
        if (lr) {
            lx = (int) Math.ceil(l - .5);
            rx = (int) Math.floor(r - .5);
            if (lx < xmin)
                lx = xmin;
            if (rx >= xmax)
                rx = xmax - 1;
            if (lx > rx)
                return;
        } else {
            lx = (int) Math.floor(l - .5);
            rx = (int) Math.ceil(r - .5);
            if (rx < xmin)
                rx = xmin;
            if (lx >= xmax)
                lx = xmax - 1;
            if (rx > lx)
                return;
        }

        makeXincrement(lx);
        final int inc = (lr) ? 1 : -1;
        final int posOff = y * w;

        colorize(lx + posOff);
        // blur(lx+posOff,-inc);
        for (int x = lx + inc + posOff; lr ? (x <= rx + posOff) : (x >= rx
                + posOff); x += inc) {
            // colorize(x+posOff);
            if (interpolateFullX)
                xxx.incrementX();
            if (interpolateY)
                yyy.incrementX();
            zzz.incrementX();

            if (interpolateW)
                www.incrementX();
            if (interpolateColor) {
                rrr.incrementX();
                ggg.incrementX();
                bbb.incrementX();
            }
            if (interpolateA)
                aaa.incrementX();
            if (interpolateUV) {
                uuu.incrementX();
                vvv.incrementX();
            }
            if(interpolateNormals) {
                nnx.incrementX();
                nny.incrementX();
                nnz.incrementX();
            }
            colorize(x/* +posOff */);
            // blur(x,+inc);
            // for(int k = 2; k<INTERPOLATION_LENGTH; k++)
            // quantities[k].incrementX();
        }
    }

    private final void blur(int i, int j) {
        int k = i + j;
        if (k < 0 | k >= zBuffer.length)
            return;
        if (zBuffer[k] - zBuffer[i] > 0.00) {
            final int sample = pixels[i];
            final int sample2 = pixels[k];

            int sb = sample & COLOR_CH_MASK;
            int sg = (sample >> 8) & COLOR_CH_MASK;
            int sr = (sample >> 16) & COLOR_CH_MASK;
            int sa = (sample >> 24) & COLOR_CH_MASK;
            int sb2 = sample2 & COLOR_CH_MASK;
            int sg2 = (sample2 >> 8) & COLOR_CH_MASK;
            int sr2 = (sample2 >> 16) & COLOR_CH_MASK;
            int sa2 = (sample2 >> 24) & COLOR_CH_MASK;

            int a = (sa2 + sa) / 2;
            int r = (sr2 + sr) / 2;
            int g = (sg2 + sg) / 2;
            int b = (sb2 + sb) / 2;
            pixels[k] = ((int) a << 24) | ((int) r << 16) | ((int) g << 8)
                    | (int) b;
        }

    }

    private final void makeXincrement(final int leftBound) {
        final double dx = 1 / Math.max(1, Math.abs(xxx.rightValue
                - xxx.leftValue));

        final double frac = Math.abs((leftBound + .5 - xxx.leftValue));
        if (interpolateFullX)
            xxx.makeXIncrement(dx, frac);
        if (interpolateY)
            yyy.makeXIncrement(dx, frac);
        zzz.makeXIncrement(dx, frac);
        if (interpolateW)
            www.makeXIncrement(dx, frac);
        if (interpolateColor) {
            rrr.makeXIncrement(dx, frac);
            ggg.makeXIncrement(dx, frac);
            bbb.makeXIncrement(dx, frac);
        }
        if (interpolateA)
            aaa.makeXIncrement(dx, frac);
        if (interpolateUV) {
            uuu.makeXIncrement(dx, frac);
            vvv.makeXIncrement(dx, frac);
        }
        if(interpolateNormals) {
            nnx.makeXIncrement(dx,frac);
            nny.makeXIncrement(dx,frac);
            nnz.makeXIncrement(dx,frac);
        }
    }

    private final double[] color = new double[4];

    public static abstract class Colorizer {
        public abstract void colorize(final int[] pixels, final double[] zBuff,
                final int pos, final double[] data, boolean interpolateUV);
    }

    final double[] data = new double[Polygon.VERTEX_LENGTH];
   
    private int[] backgroundArray;

    private int rightlower;

    private int rightupper;

    private int leftupper;

    private int leftlower;

    private boolean transparencyEnabled;

    private final void colorize(final int pos) {

        final double z = zzz.value;
        if (z >= zBuffer[pos])
            return;

        final double ww = www.value;

        final double factor = interpolateColor ? (255. / ww) : 255.;
        double r = /* (int) */(rrr.value * factor);
        double g = /* (int) */(ggg.value * factor);
        double b = /* (int) */(bbb.value * factor);

        double omt = (1 - transparency);
        if (interpolateA)
            omt = (omt * aaa.value) / ww;
       
        if (interpolateUV) {
            //final double[] color = this.color;
            final double WW = 1/ww;
            color[0] = r;
            color[1] = g;
            color[2] = b;
            color[3] = (omt*255);
            if(interpolateNormals)
                texture.getColor(uuu.value * WW, vvv.value * WW,
                        nnx.value * WW, nny.value * WW, nnz.value * WW,
                        pos % w, pos / w,
                        color);
            else 
                texture.getColor(uuu.value * WW, vvv.value * WW,
                        0,0,0,
                        pos % w, pos / w,
                        color);
            // omt *= color[3]*COLOR_CH_SCALE;
            // r *= color[0]*COLOR_CH_SCALE;
            // g *= color[1]*COLOR_CH_SCALE;
            // b *= color[2]*COLOR_CH_SCALE;
            omt = color[3]/255. ;
            r = color[0];
            g = color[1];
            b = color[2];
        }
        if(omt <=0)
            return;
       //if ( omt < 1.D) {
            if ( transparencyEnabled && omt < 1.D) {


            final int sample = pixels[pos];

            final double t = (1. - omt);

            final int sb = sample & COLOR_CH_MASK;
            final int sg = (sample >> 8) & COLOR_CH_MASK;
            final int sr = (sample >> 16) & COLOR_CH_MASK;

            r = r * omt + t * sr;
            g = g * omt + t * sg;
            b = b * omt + t * sb;
        }
        // System.out.println("r "+r);
        // System.out.println("g "+g);
        // System.out.println("b "+b);
//        if (colorizer != null) {
//            data[Polygon.SX] = xxx.value / ww;
//            data[Polygon.SY] = yyy.value / ww;
//            data[Polygon.SZ] = z;
//            data[Polygon.R] = r;
//            data[Polygon.G] = g;
//            data[Polygon.B] = b;
//            data[Polygon.A] = aaa.value / ww;
//            data[Polygon.U] = uuu.value / ww;
//            data[Polygon.V] = vvv.value / ww;
//            colorizer.colorize(pixels, zBuffer, pos, data, interpolateUV);
//        } else {
            pixels[pos] = OPAQUE | ((int) r << 16) | ((int) g << 8) | (int) b;
            zBuffer[pos] = z;
//        }
        /*
         blur(pos,1);
         blur(pos,-1);
         blur(pos,+w);
         blur(pos,-w);
         */
    }

    //
    //
    // line with z
    //
    //
    private static final double ZEPS = -.0001;

    private final void line(final double x1, final double y1, final double z1,
            final double x2, final double y2, final double z2, final int xmin,
            final int xmax, final int ymin, final int ymax) {
        double dirX = x2 - x1;
        double dirY = y2 - y1;
        double dirZ = z2 - z1;
        double l = ((double) dirX) * dirX + ((double) dirY) * dirY;
        l = Math.sqrt(l)/4;
        if (l < 1)
            return;
        dirX /= l;
        dirY /= l;
        dirZ /= l;

        double zz1 = z1 - dirZ + ZEPS;
        double zz2 = z2 + dirZ + ZEPS;

        scanPolygon(new double[][] {
                { x1 - dirX - dirY, y1 - dirY + dirX, zz1, 0, 0, 0, 0, 0, 1 },
                { x1 - dirX + dirY, y1 - dirY - dirX, zz1, 0, 0, 0, 0, 0, 1 },
                { x2 + dirX + dirY, y2 + dirY - dirX, zz2, 0, 0, 0, 0, 0, 1 } });

        scanPolygon(new double[][] {
                { x1 - dirX - dirY, y1 - dirY + dirX, zz1, 0, 0, 0, 0, 0, 1 },
                { x2 + dirX + dirY, y2 + dirY - dirX, zz2, 0, 0, 0, 0, 0, 1 },
                { x2 + dirX - dirY, y2 + dirY + dirX, zz2, 0, 0, 0, 0, 0, 1 } });

    }

    public final void setWindow(int xmin, int xmax, int ymin, int ymax) {
        this.xmin = xmin;
        this.xmax = xmax;
        this.ymin = ymin;
        this.ymax = ymax;
        // w = xmax - xmin;
        // h = ymax -ymin;
        int nw = xmax - xmin, nh = ymax - ymin;
        if (nw != w || nh != h) {
            w = nw;
            h = nh;
            final int numPx = w * h;
            zBuffer = new double[numPx];
            if(backgroundArray != null)
                makeBackgroundArray();
        }

    }

    public final void setSize(double width, double height) {
        wh = (width) / 2.;
        hh = (height) / 2.;
        minDim = Math.min(wh, hh);
        maxDim = Math.max(wh, hh);

    }

    @Override
    public void setBackground(int argb) {
        background = argb;
    }

    @Override
    public int getBackground() {
        return background;
    }

    public final void clear(boolean clearBackground) {
        Arrays.fill(zBuffer, Double.MAX_VALUE);
        if(clearBackground)
            if(backgroundArray!= null)
                System.arraycopy(backgroundArray, 0, pixels, 0, pixels.length);
            else
                Arrays.fill(pixels, background);

    }

    public final void start() {
    }

    public final void stop() {
        if(imager != null)
            imager.process(pixels,zBuffer,w,h);
    }

    public boolean isInterpolateUV() {
        return interpolateUV;
    }

    @Override
    public void setBackgroundColors(Color[] colors) {
        if(colors == null) {
            backgroundArray = null;
            return;
        }
        
        int lu = colors[0].getRGB();
        int ru = colors[1].getRGB();
        int ld = colors[2].getRGB();
        int rd = colors[3].getRGB();;
        if ( lu != leftupper || ru != rightupper || ld != leftlower || rd != rightlower) {
            leftupper = lu;
            rightupper = ru;
            leftlower = ld;
            rightlower = rd;
            makeBackgroundArray();
        }
    }

    private void makeBackgroundArray() {
        backgroundArray = new int[w*h];
        for(int i = 0; i<w; i++) {
            double l = i/(double)w;
            for(int j = 0;j<h;j++) {
                double m = j/(double) h;
                double r =(1-m)* ( (l) *(255&(leftupper>>16)) + (1-l)*(255&(rightupper>>16))) +
                (m)* ( (l) *(255&(leftlower>>16)) + (1-l)*(255&(rightlower>>16)));
                
                double g =(1-m)* ( (l) *(255&(leftupper>>8)) + (1-l)*(255&(rightupper>>8))) +
                (m)* ( (l) *(255&(leftlower>>8)) + (1-l)*(255&(rightlower>>8)));
                
                double b =(1-m)* ( (l) *(255&(leftupper>>0)) + (1-l)*(255&(rightupper>>0))) +
                (m)* ( (l) *(255&(leftlower>>0)) + (1-l)*(255&(rightlower>>0)));
                backgroundArray[i+ w*j] = 0xff000000 + (((int)r)<< 16)  + (((int)g)<<8) + ((int)b);
            }
        }
    }

    public void setTransparencyEnabled(boolean transparencyEnabled) {
        this.transparencyEnabled = transparencyEnabled;
    }

    @Override
    public double getMinDim() {
        return 2*minDim;
    }

}
