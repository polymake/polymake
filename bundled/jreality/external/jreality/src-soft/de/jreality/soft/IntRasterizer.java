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

/**
 * 
 * @version 1.0
 * @author <a href="mailto:hoffmann@math.tu-berlin.de">Tim Hoffmann</a>
 * @author Holger Pietsch 
 *
 */
public final class IntRasterizer extends ModularIntegerPolygonRasterizer {
        
  private static final int OPAQUE = (255 << 24);
  private static final int ROUND_DOWN = (255 << FIXP);
  private static final boolean OPTIMIZED = true;
  protected int zBuffer[];
  private int w;
  private int h;

  private int[] pixels;
  private int background;
  

  /**
   * 
   */
  public IntRasterizer(int[] pixelBuf) {
    super();
    pixels=pixelBuf;
    System.out.println(">INT RASTERIZER<");
    
  }

  /**
   * @param x
   * @param y
   * @deprecated the ModularINtegerRasterizer does not use this anymore...
   */
  protected final void setPixel(final int x, final int y) {
    final int pos= (x + w * y);
    if (apszI >= zBuffer[pos]) return;
    if (transparency == 0) {
      //use this if the color channels are packed in an int
        // The following fails since FIXP = 14 so FIXP-16 = -2.
        // unfortunately  >> -2 IS NOT <<2 !!!
        //TODO THIS SHOULD PROBABLY BE DONE 
        // IN A WAY THAT RESPECTS THE SETTING IF FIXP...
        //int r=((aprI&ROUND_DOWN)>>(FIXP-16)),
        
        int r=((aprI&ROUND_DOWN)<<(2)),
        g=((apgI&ROUND_DOWN)>>(FIXP-8)),
          b=( apbI>>FIXP)     ;//no need for masking blue as bits get lost anyway 
        
        
      pixels[pos]  = OPAQUE |  r |  g | b;
      
      //zBuffer[pos]= apszI;
    } else {
      // once again this is for int version :
			final int sample = pixels[pos];
//      int sb = sample &255;
//      sample = sample>>8;
//      int sg = sample &255;
//      sample = sample>>8;
//      int sr = sample &255;
      int sb = sample & 0xff;
      int sg = (sample>>8) & 0xff;
      int sr = (sample>>16) & 0xff;

//      int r = ( (/*oneMinusTransparency*/(aprI>>FIXP) + (transparency*sr))/255)<<16;
//      int g = ( (/*oneMinusTransparency*/(apgI>>FIXP) + (transparency*sg))/255)<<8;
//      int b = ( (/*oneMinusTransparency*/(apbI>>FIXP) + (transparency*sb))/255);

      int r = (  (/*oneMinusTransparency*/(aprI>>FIXP) + (transparency*sr))*257)    &0xff0000;
      int g = (( (/*oneMinusTransparency*/(apgI>>FIXP) + (transparency*sg))*257)>>8)&0x00ff00;
      int b = (  (/*oneMinusTransparency*/(apbI>>FIXP) + (transparency*sb))*257)>>16;
      
//      int r = (  crop((aprI>>FIXP)/255 + (sr))<<16) &0xff0000;
//      int g = (  crop((apgI>>FIXP)/255 + (sg))<<8 ) &0x00ff00;
//      int b = (  crop((apbI>>FIXP)/255 + (sb))    ) &255;
      
			pixels[pos]  = OPAQUE | r | g | b;
    }
    zBuffer[pos]= apszI;

  }

//  private final static int crop(int x) {
//      return x<0?0:( x>255?255:x);
//  }
  
  protected final void setPixel(final int x, final int y, final int z, int red,  int green,  int blue, int transparency) {
      final int pos= (x + w * y);
      if (z >= zBuffer[pos]) return;
      if (transparency == 0) {
          red  <<= 16;
          green<<=  8;
      } else {
          // once again this is for int version :
          final int sample = pixels[pos];
          int sb =  sample      & 0xff;
          int sg = (sample>>8)  & 0xff;
          int sr = (sample>>16) & 0xff;

          red   = (  (/*oneMinusTransparency*/(red)   + (transparency*sr))*257)    &0xff0000;
          green = (( (/*oneMinusTransparency*/(green) + (transparency*sg))*257)>>8)&0x00ff00;
          blue  = (  (/*oneMinusTransparency*/(blue)  + (transparency*sb))*257)>>16;
      }
      pixels[pos]  = OPAQUE |  red |  green | blue;
      zBuffer[pos]= z;
  }
  
  public void setBackground(int argb) {
    background=argb;
  }
  /* (non-Javadoc)
   * @see de.jreality.soft.Renderer#clear()
   */
  public void clear() {
    Arrays.fill(zBuffer, Integer.MAX_VALUE);
    Arrays.fill(pixels, background);
  }
  /* (non-Javadoc)
   * @see de.jreality.soft.PolygonRasterizer#setWindow(int, int, int, int)
   */
  public void setWindow(int xmin, int xmax, int ymin, int ymax) {
    super.setWindow(xmin, xmax, ymin, ymax);
    int nw=xmax-xmin, nh=ymax-ymin;
    if(nw!=w||nh!=h) {
      w=nw;
      h=nh;
      final int numPx=w*h;
//      pixelsR=new byte[numPx*3];
      zBuffer=new int[numPx];
    }
  }

/* (non-Javadoc)
 * @see de.jreality.soft.PolygonRasterizer#start()
 */
public void start() {
    // TODO Auto-generated method stub
    
}

/* (non-Javadoc)
 * @see de.jreality.soft.PolygonRasterizer#stop()
 */
public void stop() {
    // TODO Auto-generated method stub
    
}



}
