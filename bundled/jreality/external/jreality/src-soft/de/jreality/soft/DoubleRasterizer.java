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
public class DoubleRasterizer extends ModularDoublePolygonRasterizer {

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
  private byte[] pixelsR;
  private byte backgroundB;
  private byte backgroundG;
  private byte backgroundR;
  /**
   * 
   */
  public DoubleRasterizer(byte[] pixelBuffer) {
    super();
    pixelsR=pixelBuffer;
  }

  /**
   * @param x
   * @param y
   * @deprecated ModularDoublePolygonRasterizer now uses setPixel(int,int,double,double,double,double)
   * 
   */
  protected final void setPixel(final int x, final int y) {
    // set the pixel here!
    //if(x<0 ||x>=w|| y<0||y>=h) return;

    int pos= (x + w * y);
    if (apsz > zBuffer[pos])
      return;
    int tpos= 3 * pos;
    //		if (true)  {
    if (transparency == 0) {

      //use this if the color channels are packed in an int
      //pixels[pos]  = (255 << 24) |  (aprI>>FIXP) << 16 |  (apgI>>FIXP) << 8 | (apbI>>FIXP);

      // and this for three discrete bytes:
      pixelsR[tpos++]= (byte) ((apr));
      pixelsR[tpos++]= (byte) ((apg));
      pixelsR[tpos]= (byte) (apb);
    zBuffer[pos]= apsz;
    } else {
      // once again this is for int version :
      //			int sample = pixels[pos];
      //			int sb = sample &255;
      //			sample = sample>>8;
      //			int sg = sample &255;
      //			sample = sample>>8;
      //			int sr = sample &255;
      //
      //			int r = (	(oneMinusTransparency*(aprI>>FIXP) + (transparency*sr))/255)<<16;
      //			int g = (	(oneMinusTransparency*(apgI>>FIXP) + (transparency*sg))/255)<<8;
      //			int b = (	(oneMinusTransparency*(apbI>>FIXP) + (transparency*sb))/255);
      //			pixels[pos]  = (255 << 24) | r | g | b;

      // and this is the version for bytes:
      pixelsR[tpos]=
        (byte) (((255 & pixelsR[tpos++]) * transparency +
      /*oneMinusTransparency*/
       (apr)));
      pixelsR[tpos]=
        (byte) (((255 & pixelsR[tpos++]) * transparency +
      /*oneMinusTransparency*/
       (apg)));
      pixelsR[tpos]=
        (byte) (((255 & pixelsR[tpos++]) * transparency +
      /*oneMinusTransparency*/
       (apb)));

      //			pixelsR[tpos] = (byte)(((255&pixelsR[tpos++])*transparency +oneMinusTransparency*(aprI>>FIXP))>>8);
      //			pixelsR[tpos] = (byte)(((255&pixelsR[tpos++])*transparency +oneMinusTransparency*(apgI>>FIXP))>>8);
      //			pixelsR[tpos] = (byte)(((255&pixelsR[tpos++  ])*transparency +oneMinusTransparency*(apbI>>FIXP))>>8);

    }
  }

  protected final void setPixel(final int x, final int y, final double z, final double red, final double green, final double blue, final double transparency) {
      // set the pixel here!
      //if(x<0 ||x>=w|| y<0||y>=h) return;

      int pos= (x + w * y);
      if (z > zBuffer[pos])
          return;
      int tpos= 3 * pos;
      //        if (true)  {
      if (transparency == 0) {

          //use this if the color channels are packed in an int
          //pixels[pos]  = (255 << 24) |  (aprI>>FIXP) << 16 |  (apgI>>FIXP) << 8 | (apbI>>FIXP);

          // and this for three discrete bytes:
          pixelsR[tpos]= (byte) ((red));
          pixelsR[tpos+1]= (byte) ((green));
          pixelsR[tpos+2]= (byte) (blue);
      } else {
          // once again this is for int version :
          //            int sample = pixels[pos];
          //            int sb = sample &255;
          //            sample = sample>>8;
          //            int sg = sample &255;
          //            sample = sample>>8;
          //            int sr = sample &255;
          //
          //            int r = (   (oneMinusTransparency*(aprI>>FIXP) + (transparency*sr))/255)<<16;
          //            int g = (   (oneMinusTransparency*(apgI>>FIXP) + (transparency*sg))/255)<<8;
          //            int b = (   (oneMinusTransparency*(apbI>>FIXP) + (transparency*sb))/255);
          //            pixels[pos]  = (255 << 24) | r | g | b;

          // and this is the version for bytes:
          pixelsR[tpos]=
              (byte) (((255 & pixelsR[tpos]) * transparency +
                      /*oneMinusTransparency*/
                      (red)));
          pixelsR[tpos+1]=
              (byte) (((255 & pixelsR[tpos+1]) * transparency +
                      /*oneMinusTransparency*/
                      (green)));
          pixelsR[tpos+2]=
              (byte) (((255 & pixelsR[tpos+2]) * transparency +
                      /*oneMinusTransparency*/
                      (blue)));

          //            pixelsR[tpos] = (byte)(((255&pixelsR[tpos++])*transparency +oneMinusTransparency*(aprI>>FIXP))>>8);
          //            pixelsR[tpos] = (byte)(((255&pixelsR[tpos++])*transparency +oneMinusTransparency*(apgI>>FIXP))>>8);
          //            pixelsR[tpos] = (byte)(((255&pixelsR[tpos++  ])*transparency +oneMinusTransparency*(apbI>>FIXP))>>8);

      }
      zBuffer[pos]= z;
  }
  
  public void setBackground(int argb) {
    backgroundB= (byte) argb;
    backgroundG= (byte) (argb >> 8);
    backgroundR= (byte) (argb >> 16);
  }
  /* (non-Javadoc)
   * @see de.jreality.soft.Renderer#clear()
   */
  public void clear() {
    //
    // we need to clear the z-buffer...
    //
    int imv= Integer.MAX_VALUE;
    //		int j = 0;
    //		int pl = zBuffer.length - 1;
    //		for (int i = 0; i < pl;) {
    //			 zBuffer[i++] = zBuffer[i] = imv;
    //			 pixels[j++] = pixels[j] = bgColor;
    //		}

    Arrays.fill(zBuffer, imv);

    for (int i= 0; i < pixelsR.length; i+=3) {
      pixelsR[i  ]= backgroundR;
      pixelsR[i+1]= backgroundG;
      pixelsR[i+2]= backgroundB;
    }

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
