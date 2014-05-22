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


package de.jreality.shader;

import java.awt.Component;
import java.awt.Graphics;
import java.awt.Image;
import java.awt.MediaTracker;
import java.awt.Toolkit;
import java.awt.image.BufferedImage;
import java.awt.image.ImageObserver;
import java.awt.image.ImageProducer;
import java.awt.image.PixelGrabber;
import java.awt.image.WritableRaster;
import java.io.IOException;
import java.io.Serializable;
import java.util.logging.Level;

import javax.imageio.ImageIO;

import de.jreality.math.Rn;
import de.jreality.util.Input;
import de.jreality.util.LoggingSystem;


/**
 * 
 * This class covers data of an image either 
 * created as a {@link java.awt.Image} or as a
 * {@link byte[]} array containing RGBA values 
 * for each pixel (row/column order ?).
 * 
 * If bytes or Image is available use a Constructor -
 * otherwise use a factory method to load an image
 * from an {@link de.jreality.util.Input} source
 * 
 * <p>
 * Note: this class is immutable. The Constructors 
 * that have an image parameter just extract the data
 * from the given image and don't reference it any longer.
 * byte[] data is copied.
 * 
 * Pending: do we want to keep the original image
 * to pass out as RO instance?
 * 
 * @author weissman 
 */
public class ImageData implements Serializable {

  private transient byte[] handOutArray;
  private transient Image img, origImg;
  private byte[] byteArray, origByteArray;
  private int width;
  private int height;

  public static ImageData load(Input input) throws IOException {
    return new ImageData(loadInput(input));
  }

  public static ImageData load(Input input, double[] channelMatrix) throws IOException {
    return new ImageData(loadInput(input), channelMatrix);
  }
  
  private static Image loadInput(Input in) throws IOException {
    String urlString = in.toString().toLowerCase();
    if (urlString.endsWith(".jpeg") || urlString.endsWith(".jpg")
        || urlString.endsWith(".gif") || urlString.endsWith(".png"))
        return Toolkit.getDefaultToolkit().getImage(in.toURL());
    LoggingSystem.getLogger(ImageData.class).log(Level.INFO,
        "loading " + in + " trying ImageIO");
    Image img = ImageIO.read(in.getInputStream());
    if (img == null)
      throw new IOException("read failed: " + in);
    else
      return img;
  }

  public ImageData(byte[] data, int width, int height) {
    if (data.length != 4*width*height)
      throw new IllegalArgumentException("data doesn't match image dimensions"); 
    byteArray = new byte[data.length];
    System.arraycopy(data, 0, byteArray, 0, data.length);
    this.width = width;
    this.height = height;
  }

  public ImageData(Image img) {
    this(img, null);
  }

  public ImageData(Image img, int width, int height) {
    this(img, width, height, null);
  }

  public ImageData(Image img, double[] channelMatrix) {
    this(img, img.getWidth(null), img.getHeight(null), channelMatrix);
  }

  public ImageData(Image img, int width, int height, double[] channelMatrix) {
    if (width == -1 || height == -1) {
      long st = System.currentTimeMillis();
      wait(img);
      st = System.currentTimeMillis() - st;
      LoggingSystem.getLogger(this).log(Level.FINER,
          "waited {0} ms for image loading.", new Long(st));
      width = img.getWidth(null);
      height = img.getHeight(null);
    }
    this.width = width;
    this.height = height;
    readBytes(img, channelMatrix);
  }

  static void wait(Image img) {
    MediaTracker t = new MediaTracker(new Component() {
    });
    t.addImage(img, 0);
    for (boolean gotIt = false; !gotIt;)
      try {
        t.waitForAll();
        gotIt = true;
      } catch (InterruptedException e) {
        throw new Error();
      }
  }

  private Image createImage() {
    BufferedImage bi = new BufferedImage(width, height,
        BufferedImage.TYPE_INT_ARGB);
    WritableRaster raster = bi.getRaster();
    int[] pix = new int[4];
    for (int y = 0, ptr = 0; y < height; y++)
      for (int x = 0; x < width; x++, ptr += 4) {
        pix[0] = byteArray[ptr + 3];
        pix[1] = byteArray[ptr];
        pix[2] = byteArray[ptr + 1];
        pix[3] = byteArray[ptr + 2];
        raster.setPixel(x, y, pix);
      }
    return bi;//new ROImage(bi);
  }

  private Image createOrigImage() {
	    BufferedImage bi = new BufferedImage(width, height,
	        BufferedImage.TYPE_INT_ARGB);
	    WritableRaster raster = bi.getRaster();
	    int[] pix = new int[4];
	    for (int y = 0, ptr = 0; y < height; y++)
	      for (int x = 0; x < width; x++, ptr += 4) {
	        pix[3] = byteArray[ptr + 3];
	        pix[0] = byteArray[ptr];
	        pix[1] = byteArray[ptr + 1];
	        pix[2] = byteArray[ptr + 2];
	        raster.setPixel(x, y, pix);
	      }
	    return bi;//new ROImage(bi);
	  }

  private void readBytes(Image theImage, double[] channelArithmeticMatrix) {
    if (byteArray == null) {
      int[] pixelsI = new int[width * height];
      PixelGrabber p = new PixelGrabber(theImage, 0, 0, width, height, pixelsI,
          0, width);
      try {
        p.grabPixels();
      } catch (InterruptedException e) {
      }
      int num = pixelsI.length << 2;
      byteArray = new byte[num];
      for (int i = 0, j = 0; j < num; i++, j += 4) {
        final int px = pixelsI[i];
        byteArray[j + 3] = (byte) (px >>> 24);
        byteArray[j] = (byte) (px >>> 16);
        byteArray[j + 1] = (byte) (px >>> 8);
        byteArray[j + 2] = (byte) px;
      }
      if (channelArithmeticMatrix != null) {
        double[] pixel = new double[4];
        for (int j = 0; j < num; j += 4) {
          for (int i = 0; i < 4; ++i)
            pixel[i] = (double) byteArray[j + i];
          Rn.matrixTimesVector(pixel, channelArithmeticMatrix, pixel);
          for (int i = 0; i < 4; ++i)
            byteArray[j + i] = (byte) pixel[i];
        }
      }
    }
  }

  public int getHeight() {
    return height;
  }
  public int getWidth() {
    return width;
  }
  /**
   * @return a readonly instance of the Image
   */
  public Image getImage() {
    return img == null ? img = createImage() : img;
  }
  /**
   * @return a copy of the byte data
   */
  public byte[] getByteArray() {
    if (handOutArray == null) handOutArray=(byte[]) byteArray.clone();
    else System.arraycopy(byteArray, 0, handOutArray, 0, byteArray.length);
    return handOutArray;
  }
  
  public Image getOriginalImage()	{
	  return origImg == null ? origImg = createOrigImage() : origImg;
  }

  /**
   * applies the given matrix to all pixel values.
   *
   * @param channelArithmeticMatrix the matrix to multiply the byte array with
   * @return the transformed byte array 
   */
  public byte[] getByteArray(double[] channelArithmeticMatrix) {
    int numBytes = byteArray.length;
    byte[] ret = new byte[numBytes];
    if (channelArithmeticMatrix != null) {
      double[] pixel = new double[4];
      for (int j = 0; j < numBytes; j += 4) {
        for (int i = 0; i < 4; ++i)
          pixel[i] = (double) byteArray[j + i];
        Rn.matrixTimesVector(pixel, channelArithmeticMatrix, pixel);
        for (int i = 0; i < 4; ++i)
          ret[j + i] = (byte) pixel[i];
      }
    }
    return ret;
  }
  
  public String toString() {
    return "ImageData: width=" + width + " height=" + height;
  }
  /**
   * this class prevents a created BufferedImage from being changed
   */
  private final class ROImage extends Image {
    private final Image img;
    ROImage(Image img) {
      this.img=img;
    }
    public boolean equals(Object obj) {
      return img.equals(obj);
    }
    public void flush() {
      img.flush();
    }
    public Graphics getGraphics() {
      return img.getGraphics();
    }
    public int getHeight(ImageObserver observer) {
      return img.getHeight(observer);
    }
    public Object getProperty(String name, ImageObserver observer) {
      return img.getProperty(name, observer);
    }
    public Image getScaledInstance(int width, int height, int hints) {
      return img.getScaledInstance(width, height, hints);
    }
    public ImageProducer getSource() {
      return img.getSource();
    }
    public int getWidth(ImageObserver observer) {
      return img.getWidth(observer);
    }
    public int hashCode() {
      return img.hashCode();
    }
    public String toString() {
      return img.toString();
    }
  }
}
