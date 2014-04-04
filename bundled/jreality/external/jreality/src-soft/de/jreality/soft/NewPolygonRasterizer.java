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

import de.jreality.util.Secure;
import de.jreality.util.SystemProperties;

/**
 * @version 1.0
 * @author <a href="mailto:hoffmann@math.tu-berlin.de">Tim Hoffmann</a>
 *  
 */
public class NewPolygonRasterizer implements PolygonRasterizer {
    //For fixpoint math:
    public static final int FIXP = 16;
    protected final static int FIXPS = 1 << FIXP;
    protected final static long LONG_DFIXPS = 1L << (2*FIXP);
    protected final static double DOUBLE_DFIXPS = 1L << (2*FIXP);
    //protected static final int FIXMASK = ~(0xffffffff << FIXP);
    protected final static int HIGH_BITS = (0xffffffff << FIXP);
    protected final static int MAX_RANGE_FACTOR = (Integer.MAX_VALUE>>(FIXP+1));
    
    public static final int  COLOR_CH_SCALE = fpInverse(254<<FIXP);
    public static final int     COLOR_CH_MASK  = 255;
    private static final int     OPAQUE         = (COLOR_CH_MASK << 24);
    protected static final int R_MASK = (0xff0000 );
    protected static final int G_MASK = (0xff00 );
    protected static final int B_MASK = (0xff );
   
    private static final boolean correctInterpolation = false;

    
    private Imager imager = null;
    
    {
    	try {
    		String img = Secure.getProperty(SystemProperties.SOFT_IMAGER);
            if (img != null && img.equals("hatch")) imager = new HatchImager();
            if (img != null && img.equals("toon")) imager = new ToonImager();
    	} catch (SecurityException se) {
 			//webstart
		}
    }
    
    //dimensions of the image to render into:

    private int xmin = 0;
    private int xmax = 0;
    private int ymin = 0;
    private int ymax = 0;

    // store half the size of the screen:
    //minDim is the min of wh and hh;
    private int wh;
    private int hh;
    private int minDim;
    private int maxDim;
    private int[][] polygon =
        new int[Polygon.MAXPOLYVERTEX][Polygon.VERTEX_LENGTH];
    
    //private final Quantity[] quantities = new Quantity[Polygon.VERTEX_LENGTH];
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
    
    private boolean interpolateColor;
    private boolean interpolateW;
    private boolean interpolateA;
    private boolean interpolateUV;
    protected int transparency = 0;
    protected double dTransparency = 0;
    private Texture texture = null;

    private int zBuffer[];
    private int w;
    private int h;

    private int[] pixels;
    private int background;

    
    /**
     *  
     */
    public NewPolygonRasterizer(int[] pixelBuf) {
        super();
        pixels=pixelBuf;
        //System.out.println(">NEW INTEGER RASTERIZER<");
        //System.out.println("(correct interpolation = "+correctInterpolation+")");
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
        dTransparency = p.getShader().getVertexShader().getTransparency();
        transparency = (int)(FIXPS*
             dTransparency) ;
        
        
        texture = p.getShader().getTexture();
        interpolateUV = texture != null;
        
        
        interpolateColor = p.getShader().interpolateColor();
        
        interpolateW =correctInterpolation&&(interpolateColor||interpolateUV);
        
        
        final int pLength = p.length;
        for (int i = 0; i < pLength; i++) {
            int pos = p.vertices[i];
            int[] pi = polygon[i];

            double w = FIXPS / vertexData[pos + Polygon.SW];
            double wxy = w * minDim;
            pi[Polygon.SX] =  (int)(wh + vertexData[pos + Polygon.SX] * wxy);
            pi[Polygon.SY] =  (int)(hh - vertexData[pos + Polygon.SY] * wxy);
            pi[Polygon.SZ] =  (int)(vertexData[pos + Polygon.SZ] * w *MAX_RANGE_FACTOR);
            
            //note normScale contains FIXPS
//            double wxy = w * normScale;
//            pi[Polygon.SX] = (int) (vertexData[pos + Polygon.SX] * wxy);
//            pi[Polygon.SY] = (int) (vertexData[pos + Polygon.SY] * wxy);
//            pi[Polygon.SZ] = (int) (vertexData[pos + Polygon.SZ] * w * FIXPS);
//            
            if (p.getShader() instanceof SkyboxPolygonShader)
                pi[Polygon.SZ] = Integer.MAX_VALUE-1;
            
            final double iw = (interpolateW? w: FIXPS)*MAX_RANGE_FACTOR;
                pi[Polygon.SW] = (int)iw;
//                System.out.println(" w is set" + w);
                //We scale u and v by MAX_RANGE_FACTOR to have maximal bit
                // resolution for them oterwise textures will look odd sometimes....
                pi[Polygon.U] =
                    (int)(vertexData[pos + Polygon.U] *  iw);
                pi[Polygon.V] =
                     (int)(vertexData[pos + Polygon.V] *  iw);
                pi[Polygon.A] =
                     (int)(vertexData[pos + Polygon.A] *  iw);
                
                if(interpolateColor) {
                    pi[Polygon.R] =
                         (int)((vertexData[pos + Polygon.R] >= 1
                            ? 1
                            : (vertexData[pos + Polygon.R] ))
                            * iw);
                    pi[Polygon.G] =
                         (int)((vertexData[pos + Polygon.G] >= 1
                            ? 1
                            : (vertexData[pos + Polygon.G] ))
                            * iw);
                    pi[Polygon.B] =
                         (int)((vertexData[pos + Polygon.B] >= 1
                            ? 1
                            : (vertexData[pos + Polygon.B] ))
                            * iw);
                }
                        
        }
        
            if( !interpolateColor) {
                
                int pos = p.vertices[0];
                rrr.value = (int) Math.min(FIXPS*MAX_RANGE_FACTOR,vertexData[pos + Polygon.R]*FIXPS*MAX_RANGE_FACTOR);
                ggg.value = (int) Math.min(FIXPS*MAX_RANGE_FACTOR,vertexData[pos + Polygon.G]*FIXPS*MAX_RANGE_FACTOR);
                bbb.value = (int) Math.min(FIXPS*MAX_RANGE_FACTOR,vertexData[pos + Polygon.B]*FIXPS*MAX_RANGE_FACTOR);
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

        //interpolateColor = p.getShader().interpolateColor();
        
            /*
             * private int[][] tri = new int[3][];
             * tri[0] = t0; for(int i =1;i <pLength;i++) { tri[1]
             * =polygon[i-1]; tri[2] =polygon[i];
             * scanPolyI(tri,3,xmin,xmax-1,ymin,ymax-1); } tri[1] =polygon[0];
             * tri[2] =polygon[pLength-1];
             * scanPolyI(tri,3,xmin,xmax-1,ymin,ymax-1);
             */
       
        scanPolygon(polygon, pLength/*, xmin, xmax, ymin, ymax*/);

    }
    /*
     * a class for interpolatable quantities;
     * 
     * @version 1.0
     * @author timh
     *
     */
    private final static class Quantity {
        public int leftValue;
        public int leftYIncrement;
        public int rightValue;
        public int rightYIncrement;
        public int value;
        public int xIncrement;
        
        Quantity() {
            super();
        }
        
        final  void makeXIncrement(final int dxi, final int frac) {
            
                xIncrement = NewPolygonRasterizer.fpTimes((rightValue- leftValue), dxi);
                value = leftValue + NewPolygonRasterizer.fpTimes(xIncrement , frac);
                
//                System.out.println("value "+(value/(double)FIXPS)+" incr "+(xIncrement/(double)FIXPS)+" frac "+(frac/(double)FIXPS)+" leftV "+(leftValue/(double)FIXPS));
         
        }
        
        final void makeLeftYIncrement(final int start, final int end, final int dyi, final int frac) {
            
                leftYIncrement = NewPolygonRasterizer.fpTimes((end - start), dyi);
                leftValue = start + NewPolygonRasterizer.fpTimes(leftYIncrement, frac);
//                System.out.println("YINC: leftValue "+(leftValue/(double)FIXPS)+" leftYIncr "+(leftYIncrement/(double)FIXPS));
           //     System.out.println("   start "+(( start)/(double)FIXPS));
//                System.out.println("    (end - start)*dy "+(fpTimes((end - start),dy)/(double)FIXPS));
//                System.out.println("    dy "+(dy/(double)FIXPS));
//                System.out.println("  ---> "+(fpTimes((int)(0.01*FIXPS),FIXPS))/(double)FIXPS  );
//                System.out.println("  ---> "+(((int)(-0.01*FIXPS)))/(double)FIXPS  );
           
        
        }
        final void makeRightYIncrement(final int start, final int end, final int dy, final int frac) {
           
                rightYIncrement = NewPolygonRasterizer.fpTimes((end - start),dy);
                rightValue = start + NewPolygonRasterizer.fpTimes(rightYIncrement, frac);
            
        }
        final void incrementY() {
            
                leftValue += leftYIncrement;
                rightValue += rightYIncrement;
                //System.out.println("Yincrement leftValue "+leftValue+" value "+value);
           
            
        }
        final void incrementX() {
           
                value += xIncrement;
            
        }

        /**
         * 
         */
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
        final int[][] p,
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

        int minY = Integer.MAX_VALUE;
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

        //y =(int)Math.ceil(ymin -.5);
        y = fpCeil(minY-FIXPS/2);
        //y = ((ymin + FIXPS / 2 - 1) >> FIXP);

        ly = ry = y - 1;

        while (rem > 0) {
            int i =li;
            while (ly <= y && rem > 0) {
                rem--;
                li = i;
                i = i - 1;
                if (i < 0)
                    i = plength - 1;
                //ly = (int)Math.floor(p[i][Polygon.SY] +.5);
                ly = fpFloor(p[i][Polygon.SY] +FIXPS/2);
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
                //ry = (int)Math.floor(p[i][Polygon.SY] + .5) ;
                ry = fpFloor(p[i][Polygon.SY] + FIXPS/2) ;
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
                zzz.incrementY();
                if(interpolateW)www.incrementY();
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

 

    
    private final void makeYIncrement(final int[] p1, final int[] p2, final int y, final boolean left) {
        
        final int p1y = p1[Polygon.SY];
        //final int dy = fpInverse(Math.max(FIXPS, (p2[Polygon.SY] - p1y)));
        final int tmp = (p2[Polygon.SY] - p1y);
        final int dy = fpInverse(FIXPS>tmp ? FIXPS : tmp);
        final int frac = (y<<FIXP) + FIXPS/2 - p1y;

        if(left) {
            xxx.makeLeftYIncrement(p1[Polygon.SX],p2[Polygon.SX],dy,frac);
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
        final int l = xxx.leftValue, r = xxx.rightValue;
        final boolean lr = l<r;
        int lx, rx,inc;
        //final int inc;
        if(lr) {
            lx = fpCeil (l - FIXPS/2);
            rx = fpFloor(r - FIXPS/2);
            //TODO are these bound checks really unnecessary or
            // can we get in trouble?
            if(lx < xmin) lx = xmin;
            if(rx >= xmax) rx = xmax-1;
            if(lx > rx) return;
            inc = 1;
        }else {
            lx = fpFloor(l - FIXPS/2);
            rx = fpCeil (r - FIXPS/2);
            if(rx < xmin) rx = xmin;
            if(lx >= xmax) lx = xmax-1;
            if(rx > lx) return;
            inc = -1;
        }
        
        makeXincrement(lx);
        //final int inc=(lr)? 1: -1;
        final int posOff=y*w;
        colorize(lx+posOff);
        //for(int x = lx+inc+posOff; lr? (x <= rx+posOff): (x >= rx+posOff); x+=inc) {
        final int end = inc*(rx+posOff);
        for(int x = lx+inc+posOff; x*inc <=end; x+=inc) {
            //colorize(x+posOff);
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
    //Math.abs is not final...
    private final static int abs(final int a) {
        return a<0 ? -a : a;
    }
    
    private final void makeXincrement(final int leftBound) {
        //final int dx = fpInverse(Math.max(FIXPS, Math.abs(xxx.rightValue - xxx.leftValue)));
        final int tmp = abs(xxx.rightValue - xxx.leftValue);
        final int dx = fpInverse(tmp > FIXPS ? tmp : FIXPS);
        
        //final int frac = Math.abs( ((leftBound<<FIXP)  + FIXPS/2 - xxx.leftValue));
        final int frac = abs( (leftBound<<FIXP)  + FIXPS/2 - xxx.leftValue);
        //This seems to be faster than Math.abs(...) :
        //final int a = ((leftBound<<FIXP)  + FIXPS/2 - xxx.leftValue);
        //final int frac = (a>=0)?a:-a;
        
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
//        for(int k = 2; k<INTERPOLATION_LENGTH; k++) 
//            quantities[k].makeXIncrement(dx, frac);
//            Quantity q =quantities[Polygon.R];
//            if(q.value<0|| q.value>1) {
//                //System.out.println("z WARNING " + q.value+" dx "+dx);
//        System.out.println("->R: "+q.value+" xIncr "+q.xIncrement+" left "+q.leftValue+" right "+q.rightValue);
//            }
        //System.out.println("r "+quantities[Polygon.R].value);
        
    }
    
    private final int[] color = new int[4];

    private final void colorize(final int pos) {
        final int z = zzz.value;
        if (z >= zBuffer[pos]) return;
        
        final int ww = www.value;
        
        final double factor = interpolateColor ? (255./ww) : (255./(FIXPS*MAX_RANGE_FACTOR));
        int   r = (int)((rrr.value*factor)); 
        int   g = (int)((ggg.value*factor));
        int   b = (int)((bbb.value*factor));
        //This is slower than the above version which uses doubles!!!
//        final int factor = interpolateColor ? (ww/255) : ((FIXPS*MAX_RANGE_FACTOR)/255);
//        int   r = (int)((rrr.value)/factor); 
//        int   g = (int)((ggg.value)/factor);
//        int   b = (int)((bbb.value)/factor);
        
//        if(r<0||r>255) System.out.println("R: "+r+" W "+ww);
//        if(g<0||g>255) System.out.println("G: "+r);
//        if(b<0||b>255) System.out.println("B: "+r);
//        if(r<0||r>255)  www.dump();
        
        int omt= (255*(FIXPS - transparency))>>FIXP;
        if(interpolateA) omt = ((int)((1 - dTransparency)*factor*aaa.value));
        
        if (interpolateUV) {
            final int[] color=this.color;
            final double WW = ((double)ww);
            color[0] = r;
            color[1] = g;
            color[2] = b;
            color[3] = omt;
            texture.getColor(
                    uuu.value /WW, vvv.value /WW,pos%w,pos/w, color);
            //This is now done in getColor:
//            omt = (omt*color[3]*COLOR_CH_SCALE)>>FIXP;
//            r   = (r  *color[0]*COLOR_CH_SCALE)>>FIXP;
//            g   = (g  *color[1]*COLOR_CH_SCALE)>>FIXP;
//            b   = (b  *color[2]*COLOR_CH_SCALE)>>FIXP;   
          omt = (color[3]*COLOR_CH_SCALE)>>FIXP;
          r   = (color[0]*COLOR_CH_SCALE)>>FIXP;
          g   = (color[1]*COLOR_CH_SCALE)>>FIXP;
          b   = (color[2]*COLOR_CH_SCALE)>>FIXP;   
//              r   = color[0];
//              g   = color[1];
//              b   = color[2];
//              omt = color[3];
        }
        if(omt<255) {
            final int sample = pixels[pos];

            int t = (255-omt);
            
            int sb =  sample      & COLOR_CH_MASK;
            int sg = (sample>>8)  & COLOR_CH_MASK;
            int sr = (sample>>16) & COLOR_CH_MASK;
            
            r = ((r*omt + t*sr)*257) & R_MASK ;
            g = (((g*omt + t*sg)*257)>>8)&G_MASK;
            b = (((b*omt + t*sb)*257)>>16)&B_MASK;
        } else{
            r = (r<<16)&R_MASK;;
            g =(g<<8)&G_MASK;
            b = b&B_MASK;
        }

        pixels[pos]  = OPAQUE |r |g | b;
        zBuffer[pos]= z;
    }
    

    


    //
    //
    // line with z
    //
    //
    private static final int ZEPS = (int) (-0.0001 * FIXPS);
    private final void line(
            final int x1,
            final int y1,
            final int z1,
            final int x2,
            final int y2,
            final int z2,
            final int xmin,
            final int xmax,
            final int ymin,
            final int ymax) {
        int dirX = x2 - x1;
        int dirY = y2 - y1;
        int dirZ = z2 - z1;
        double l = ((double) dirX) * dirX + ((double) dirY) * dirY;
        l = Math.sqrt(l);
        int il = 1 * (((int) l) >> FIXP);
        if (il == 0)
            return;
        dirX /= il;
        dirY /= il;
        dirZ /= il;

        int zz1 = z1 - dirZ + ZEPS;
        int zz2 = z2 + dirZ + ZEPS;

        //TODO find a replacement
        //interpolateColor = false;
        //interpolateTexture = false;
        
        scanPolygon(new int[][] {
                { x1 - dirX - dirY, y1 - dirY + dirX, zz1, 0, 0, 0, 0, 0, 1 },
                {
                    x1 - dirX + dirY, y1 - dirY - dirX, zz1, 0, 0, 0, 0, 0, 1 }, {
                        x2 + dirX + dirY, y2 + dirY - dirX, zz2, 0, 0, 0, 0, 0, 1 }, {
                            x2 + dirX - dirY, y2 + dirY + dirX, zz2, 0, 0, 0, 0, 0, 1 }
        }, 4 /*, xmin, xmax - 1, ymin, ymax - 1*/);

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
            zBuffer=new int[numPx];
        }

    }
    public final void setSize(double width, double height) {
        wh = ((int) width)  / 2;
        hh = ((int) height)  / 2;
        minDim = Math.min(wh, hh);
        maxDim = Math.max(wh, hh);
        wh *= FIXPS;
        hh *= FIXPS;
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
        if(imager != null)
            imager.process(pixels,zBuffer,w,h);
    }
    
    
    
    // 
    //
    // Some methods for doing the fix pint arithetics:
    //
    //
    public static final int fpTimes(final int x, final int y) {
        
        
        if(true) {
            // This would give the correct result and it is faster than computing
            // the correct answer in the below way.
            // Here in long seems better than double:
        
            return (int)( ( x * (long)y)>>FIXP);
            //return (int)( ( x * (double)y/FIXPS));
            
        }else {
            // This version is not correct but it seems to be good enough
            // in most cases: we neglect the product of the low parts:
        
            final int xhi = (x+FIXPS/2)>>FIXP ;
            final int yhi = y>>FIXP;
            //final int xlo = (x  -(x&HIGH_BITS));//fpFraction(x);
            final int ylo = (y  -(y&HIGH_BITS));//fpFraction(y);
            return (x*yhi) + (xhi*ylo);
            // to get an even better answer we would need the following, but
            // it is slower than simply casting to long:
            //return ((x*yhi + xhi*y) - (xhi*yhi<<FIXP) + ( (xlo*ylo)>>FIXP));
            //return ((x*yhi) + (xhi*ylo) +( (xlo*ylo)>>FIXP));
        }
    }
    
    public static final long fpTimes(final long x, final long y) {
        return ( ( x * y)>>FIXP);
    }
    
    public static final int  fpInverse( final int x) {
        //return (int) (LONG_DFIXPS/x);
        // On my machine it turns out to be a little faster to use 
        // double division than long division.
        return (int) (DOUBLE_DFIXPS/x);
        
        // This one seems to work since we only invert numbers >1
        // but again on my machine it is not faster than double 
        // division:
        //return  ((1<<(2*FIXP-2))/x)<<2;
        
    }
    
    public static long  fpInverse( final long x) {
        return  (LONG_DFIXPS/x);
    }
    /*
     public static final int fpInteger(final int  x)
     {
     final int floor = x >> FIXP;
     final int  bits = (x & FIXMASK);
     
//        if (x < 0) {
//            if (bits!=0) return(floor);
     if (x < 0 && bits != 0 )
     return(floor+1);
     else 
     return (floor);
     }
     // return fraction part, in bits 
     public static final int fpFraction(final int  x)
     {
     if (x < 0) return((-x) & FIXMASK);
     else return(x & FIXMASK);
     }
     */
    public static final int fpCeil(final int x) {
        return ((x + FIXPS - 1) >> FIXP);
    }
    public static final int fpFloor(final int x) {
        return (x >> FIXP);
    }
    
}
