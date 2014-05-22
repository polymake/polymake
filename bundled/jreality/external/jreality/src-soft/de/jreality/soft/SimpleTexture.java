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

import de.jreality.math.Matrix;
import de.jreality.shader.ImageData;
import de.jreality.shader.Texture2D;

/**
 * 
 * @version 1.0
 * @author <a href="mailto:hoffmann@math.tu-berlin.de">Tim Hoffmann</a>
 *
 */
public class SimpleTexture implements Texture {
    protected final  byte[] bytes;
    protected final int width;
    protected final int height;

    protected final double matrix[];
    protected final boolean clampU;
    protected final boolean clampV;
    protected final int incr;
    protected final boolean interpolate;
    
    private static final double[] identity = new Matrix().getArray();
	
	public SimpleTexture(ImageData id) {
		this.bytes = (byte[]) id.getByteArray();//.clone();
	      this.width=id.getWidth();
	      this.height = id.getHeight();
		this.matrix = identity;
	      this.clampU = true;
	      this.clampV = true;
		interpolate = true;
        incr =3*width*height== bytes.length?3:4;
	}
    public SimpleTexture(Texture2D texture) {
      this.bytes = (byte[]) texture.getImage().getByteArray();//.clone();
      this.width=texture.getImage().getWidth();
      this.height = texture.getImage().getHeight();
      //this.uscale = texture.getSScale();
      //this.vscale = texture.getTScale();
      this.matrix = texture.getTextureMatrix().getArray();
      this.clampU = texture.getRepeatS()==Texture2D.GL_CLAMP;
      this.clampV = texture.getRepeatT()==Texture2D.GL_CLAMP;
      incr =3*width*height== bytes.length?3:4;
      interpolate =(texture.getMinFilter()==Texture2D.GL_LINEAR);
  }

    public void  getColor(double u, double v,int x, int y, int color[]) {
        //if(u!= 0)System.out.println(((int)(u*width*uscale+.5))%width);
        //int c = pixels[((int)(u*width*uscale+.5))%width +width*(((int)(v*height*vscale+.5))%height)];
        if(interpolate)
            getPixelInterpolate(u,v,color);
        else
            getPixelNearest(u,v,color);
        
    }

    protected final void getPixelNearest(final double uu, final double vv,  final int[] color) {
        int a,b;
        double u = width*(uu*matrix[0] + vv*matrix[1] + matrix[3]);
        double v = height*(uu*matrix[4+0] + vv*matrix[4+1] + matrix[4+3]);
        u = u<0?u -Math.floor(u/width):u;
        v = v<0?v -Math.floor(v/height):v;

        
        if(clampU) {
            a = (int)(u);
            a =a<0?0:a>=width?width-1:a;
        } else {
            a = ((int)(u))%width;
        }
        if(clampV) {
            b = (int)(v);
            b =b<0?b:b>=height?height-1:b;
        } else {
            b = (((int)(v))%height);
        }
        //c = pixels[a +width*b];
        //c = pixels[((int)(u*width*uscale))%width +width*(((int)(v*height*vscale))%height)];
        int pos =incr *(a +width*b);
//        color[0]   = ((255&bytes[pos+0])  *color[0]*NewPolygonRasterizer.COLOR_CH_SCALE)>>NewPolygonRasterizer.FIXP;
//        color[1]   = ((255&bytes[pos+1])  *color[1]*NewPolygonRasterizer.COLOR_CH_SCALE)>>NewPolygonRasterizer.FIXP;
//        color[2]   = ((255&bytes[pos+2])  *color[2]*NewPolygonRasterizer.COLOR_CH_SCALE)>>NewPolygonRasterizer.FIXP;
        color[0]  *= 255&bytes[pos+0];
        color[1]  *= 255&bytes[pos+1];
        color[2]  *= 255&bytes[pos+2];
        
        if(incr == 4)
//            color[3] = (255&bytes[pos+3]*color[3]*NewPolygonRasterizer.COLOR_CH_SCALE)>>NewPolygonRasterizer.FIXP;
            color[3]  *= 255&bytes[pos+3];
        else
//            color[3] = (255*color[3]*NewPolygonRasterizer.COLOR_CH_SCALE)>>NewPolygonRasterizer.FIXP;
            color[3] *= 255;
    
        
        //color[0]  =255;
        //color[1]  =255;
        //color[2]  =255;
        //color[3]  =255;
    }
    
    protected final void getPixelInterpolate(final double uu, final double vv,  final int[] color) {
        int ap,am, bp, bm;
        //double[] tmpColor =new double[4];
        double r=0;
        double g=0;
        double b=0;
        double a=0;
        double dam;
        double dap;
        double dbm;
        double dbp;
        double u = width*(uu*matrix[0] + vv*matrix[1] + matrix[3]);
        double v = height*(uu*matrix[4+0] + vv*matrix[4+1] + matrix[4+3]);
        u = u<0?u -Math.floor(u/width):u;
        v = v<0?v -Math.floor(v/height):v;
        dam = (u);
        am  = (int)dam;
        dam = 1 - (dam-am);

        dap = (u + 1);
        ap  = (int)dap;
        dap = 1 - dam;
        
        if(clampU) {
            am =am<0?am:am>=width?width-1:am;
            
            ap =ap<0?ap:ap>=width?width-1:ap;
        } else {
            am = (am)%width;
            ap = (ap)%width;
        }
        
        dbm = (v);
        bm  = (int)dbm;
        dbm = 1 - (dbm - bm);

        dbp = (v + 1);
        bp  = (int)dbp;
        dbp = 1- dbm;
        
        if(clampV) {
            bm =bm<0?bm:bm>=height?height-1:bm;
            
            bp =bp<0?bp:bp>=height?height-1:bp;
        } else {
            bm = (bm)%height;
            bp = (bp)%height;
        }
        
        
        int pos =incr *(am +width*bm);
        double fac=dam*dbm;
        r  += fac*(255&bytes[pos+0]);
        g  += fac*(255&bytes[pos+1]);
        b  += fac*(255&bytes[pos+2]);
        if(incr == 4) 
            a  += fac*(255&bytes[pos+3]);
        
        pos =incr *(ap +width*bm);
        fac=dap*dbm;
        r  += fac*(255&bytes[pos+0]);
        g  += fac*(255&bytes[pos+1]);
        b  += fac*(255&bytes[pos+2]);
        if(incr == 4)
            a  += fac*(255&bytes[pos+3]);

        pos =incr *(ap +width*bp);
        fac=dap*dbp;
        r  += fac*(255&bytes[pos+0]);
        g  += fac*(255&bytes[pos+1]);
        b  += fac*(255&bytes[pos+2]);
        if(incr == 4)
            a  += fac*(255&bytes[pos+3]);
        
        pos =incr *(am +width*bp);
        fac=dam*dbp;
        r  += fac*(255&bytes[pos+0]);
        g  += fac*(255&bytes[pos+1]);
        b  += fac*(255&bytes[pos+2]);
        if(incr == 4)
            a  += fac*(255&bytes[pos+3]);
        
        
        
        color[0] *=255&((int)r);
        color[1] *=255&((int)g);
        color[2] *=255&((int)b);
        if(incr == 4)
            color[3] *=255&((int)a);
        
        else
            color[3] *= 255;
        
    }

    /*
     * We might need this for MIP MAPPING...
     *  
        final static  int LogTable256[] = 
        {
            0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
            4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
            5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
            5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
            6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
            6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
            6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
            6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
            7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
            7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
            7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
            7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
            7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
            7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
            7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
            7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
    };

    public static final int findLog(int v) { // 32-bit word to find the log of
        int c = 0; // c will be lg(v)
        int t, tt; // temporaries

        if ((tt = v >> 16) !=0) {
            c = ((t = v >> 24)!=0) ? 24 + LogTable256[t] : 16 + LogTable256[tt & 0xFF];
        }
        else {
            c = ((t = v & 0xFF00)!=0) ? 8 + LogTable256[t >> 8] : LogTable256[v & 0xFF];
        }
        return c;
    }
    
    static final int b[] = {0x2, 0xC, 0xF0, 0xFF00, 0xFFFF0000};
    static final int S[] = {1, 2, 4, 8, 16};
    public static final int findLog2(int v){

        int c = 0; // result of log2(v) will go here
        for (int i = 4; i >= 0; i--) // unroll for speed...
        {
            if ((v & b[i])!=0)
            {
                v >>= S[i];
                c |= S[i];
            } 
        }
        return c;
    }

     */
}
