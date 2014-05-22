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

//import java.awt.Graphics;
import java.util.Arrays;

/**
 * 
 * @version 1.0
 * @author timh
 * @deprecated
 */
public class DoubleRasterizerInt extends ModularDoublePolygonRasterizer {
    private static final int OPAQUE = (255 << 24);
    
  protected double zBuffer[];
  private int w= 0;
  private int h= 0;
  


  // there are two main ways to deal with color pixels:
  // an array of ints or an array of bytes three times the size.
  // note that in the latter case, the buffered image is assumed to have type
  // TYPE_3BYTE_BGR!

  //use this for bytes packed in an int
  //protected int pixels[];
  //and this for three discrete bytes per color chanel
  private int[] pixels;
  private int background;
  /**
   * 
   */
  public DoubleRasterizerInt(int[] pixelBuffer) {
    super();
    pixels=pixelBuffer;
  }

 
  protected final void setPixel(final int x, final int y, final double z, final double red, final double green, final double blue, final double transparency) {
      // set the pixel here!
      //if(x<0 ||x>=w|| y<0||y>=h) return;

      int pos= (x + w * y);
      if (z > zBuffer[pos])
          return;
      
      //        if (true)  {
      if (transparency == 0) {

          //use this if the color channels are packed in an int
          //pixels[pos]  = (255 << 24) |  (aprI>>FIXP) << 16 |  (apgI>>FIXP) << 8 | (apbI>>FIXP);

          // and this for three discrete bytes:
          pixels[pos]= OPAQUE | ((((int)red)<<16) + (((int)green)<<8) + ((int) (blue)));
      } else {
          int trans =(int)(255*transparency);
          final int sample = pixels[pos];
          int sb = sample & 0xff;
          int sg = (sample>>8) & 0xff;
          int sr = (sample>>16) & 0xff;

//          int r = (  (/*oneMinusTransparency*/((int)red) + (int)(trans*sr))*257)    &0xff0000;
//          int g = (( (/*oneMinusTransparency*/((int)green) + (int)(trans*sg))*257)>>8)&0x00ff00;
//          int b = (  (/*oneMinusTransparency*/((int)blue) + (int)(trans*sb))*257)>>16;

          int r = (  (/*oneMinusTransparency*/((int)red)*255   + (trans*sr))*257)    &0xff0000;
          int g = (( (/*oneMinusTransparency*/((int)green)*255 + (trans*sg))*257)>>8)&0x00ff00;
          int b = (  (/*oneMinusTransparency*/((int)blue)*255  + (trans*sb))*257)>>16;
          
          pixels[pos]  = OPAQUE | r | g | b;


      }
      zBuffer[pos]= z;
  }
  
  public void setBackground(int argb) {
      background=argb;
  }
  /* (non-Javadoc)
   * @see de.jreality.soft.Renderer#clear()
   */
  public void clear() {
      Arrays.fill(zBuffer, Double.MAX_VALUE);
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
      zBuffer=new double[numPx];
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
