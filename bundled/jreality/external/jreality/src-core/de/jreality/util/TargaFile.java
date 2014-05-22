package de.jreality.util;
/*
 * Copyright (c) 2003, Xith3D Project Group
 * All rights reserved.
 *
 * Portions based on the Java3D interface, Copyright by Sun Microsystems.
 * Many thanks to the developers of Java3D and Sun Microsystems for their
 * innovation and design.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * Neither the name of the 'Xith3D Project Group' nor the names of its 
 * contributors may be used to endorse or promote products derived from this 
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) A
 * RISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE
 *
 */

import java.awt.image.BufferedImage;
import java.io.IOException;
import java.io.InputStream;
import java.util.LinkedList;
import java.util.logging.Level;
/**
 * Handles dealing with targa image files.
 * +--------------------------------------+
 * | File Header                          |
 * +--------------------------------------+
 * | Bitmap Data                          |
 * +--------------------------------------+
 *
 * @@author Scott Shaver
 */
public class TargaFile {

    private byte  FHimageIDLength   =    0;
    private byte  FHcolorMapType    =    0; // 0 = no pallete
    private byte  FHimageType       =    0; // uncompressed RGB=2, uncompressed grayscale=3
    private short FHcolorMapOrigin  =    0;
    private short FHcolorMapLength  =    0;
    private byte  FHcolorMapDepth   =    0;
    private short FHimageXOrigin    =    0;
    private short FHimageYOrigin    =    0;
    private short FHwidth           =    0;
    private short FHheight          =    0;
    private byte  FHbitCount        =    0; // 16,24,32
    private byte  FHimageDescriptor =    0; // 24 bit = 0x00, 32-bit=0x08
    private int   filePointer       =    0;
    private byte  fileContents[]    = null;

    private byte[] data = null;

    public TargaFile() {
    }

    public byte[] getData() {
        return data;
    }

    public int getWidth() {
        return FHwidth;
    }

    public int getHeight() {
        return FHheight;
    }

    public int getBPP() {
        return FHbitCount;
    }

    public int getDataLength() {
        return data.length;
    }

    public static BufferedImage getBufferedImage(InputStream is){
      TargaFile loader = new TargaFile();
      loader.load(is);

      int width        = loader.getWidth(),
          height       = loader.getHeight(),
          bytePerPixel = loader.getBPP()/8;

      BufferedImage bufferedImage = new BufferedImage(width, height, BufferedImage.TYPE_INT_ARGB);

      byte[] imageData = loader.getData();

      for(int j = height - 1; j >= 0; j--)
        for(int i = 0; i < width; i++) {
          int  index = ((height - 1 - j) * width + i) * bytePerPixel;
          byte alpha = (bytePerPixel == 4) ? imageData[index + 3] : (byte)255;
          // switched index 0 <-> 2...
          int color  = (       alpha         & 0xFF) << 24|
                       (imageData[index + 0] & 0xFF) << 16|
                       (imageData[index + 1] & 0xFF) <<  8|
                       (imageData[index + 2] & 0xFF);
          bufferedImage.setRGB(i,j, color);
      }
      return bufferedImage;
    }

//  public void printHeaders() {
//    System.out.println("-----------------------------------");
//    System.out.println("File Header");
//    System.out.println("-----------------------------------");
//    System.out.println("      Image ID Length:"+FHimageIDLength);
//    System.out.println("       Color Map Type:"+FHcolorMapType);
//    System.out.println("           Image Type:"+FHimageType);
//    System.out.println("     Color Map Origin:"+FHcolorMapOrigin);
//    System.out.println("     Color Map Length:"+FHcolorMapLength);
//    System.out.println(" Color Map Entry Size:"+FHcolorMapDepth);
//    System.out.println("       Image X Origin:"+FHimageXOrigin);
//    System.out.println("       Image Y Origin:"+FHimageYOrigin);
//    System.out.println("                Width:"+FHwidth);
//    System.out.println("               Height:"+FHheight);
//    System.out.println("                  BBP:"+FHbitCount);
//    System.out.println("     Image Descriptor:"+FHimageDescriptor);
//  }

  public void load(InputStream dis) {
    // reset everything
    FHimageIDLength   = 0;
    FHcolorMapType    = 0;
    FHimageType       = 0;
    FHcolorMapOrigin  = 0;
    FHcolorMapLength  = 0;
    FHcolorMapDepth   = 0;
    FHimageXOrigin    = 0;
    FHimageYOrigin    = 0;
    FHwidth           = 0;
    FHheight          = 0;
    FHbitCount        = 0;
    FHimageDescriptor = 0;
    filePointer       = 0;

    try{
      LinkedList<Byte> content = new LinkedList<Byte>();
      int v;
      while ((v = dis.read())!=-1) content.add((byte)v);
      fileContents = new byte[content.size()];
      int i=0;
      for (byte b:content) fileContents[i++]=b;
      System.out.println("read "+fileContents.length);
      

      // read the file header
      FHimageIDLength   = (byte)readUnsignedByte();
      FHcolorMapType    = (byte)readUnsignedByte();
      FHimageType       = (byte)readUnsignedByte();
      FHcolorMapOrigin  =       readShort();
      FHcolorMapLength  = readShort();
      FHcolorMapDepth   = (byte)readUnsignedByte();
      FHimageXOrigin    =       readShort();
      FHimageYOrigin    =       readShort();
      FHwidth           =       readShort();
      FHheight          =       readShort();
      FHbitCount        = (byte)readUnsignedByte();
      FHimageDescriptor = (byte)readUnsignedByte();

      if(FHimageType!=2 && FHimageType!=3) { // only deal with these two types
        if(FHimageType == 10)
          loadCompressed();
        fileContents = null;
        return;
      }

      int bytesPerPixel = (FHbitCount/8);

      // allocate memory for the pixel data
      data = new byte[FHwidth*FHheight*bytesPerPixel];
      // read the pixel data
      System.arraycopy(fileContents, filePointer, data, 0, data.length);

      if(FHbitCount==24 || FHbitCount==32){
        // swap the R and B values to get RGB, targa color format is BGR
        for(int loop=0;loop<data.length;loop+=bytesPerPixel){
          byte btemp = data[loop];
          data[loop] = data[loop+2];
          data[loop+2] = btemp;
        }
      }
      fileContents = null;
    }
    catch(Exception x){
      x.printStackTrace();
      LoggingSystem.getLogger(this).log(Level.WARNING, "exception while loading", x.getMessage());
    }

  }

  public void loadCompressed() {
//    printHeaders();
    int bytesPerPixel = (FHbitCount/8);
    data = new byte[FHwidth*FHheight*bytesPerPixel];

    int pixelcount   = FHwidth*FHheight,
        currentbyte  = 0,
        currentpixel = 0;

    byte[] colorbuffer = new byte[bytesPerPixel];

    try{
      do{
        int chunkheader = 0;
        chunkheader = (int)readUnsignedByte();
//        System.out.println(chunkheader);
        if(chunkheader < 128){
          chunkheader++;
          for(short counter = 0; counter < chunkheader; counter++){

            readColorBuffer(colorbuffer);
            data[currentbyte + 0] = colorbuffer[2];
            data[currentbyte + 1] = colorbuffer[1];
            data[currentbyte + 2] = colorbuffer[0];


            if(bytesPerPixel == 4)
              data[currentbyte + 3] = (byte)readUnsignedByte();

            currentbyte += bytesPerPixel;
            currentpixel++;
            if(currentpixel > pixelcount)
              throw new IOException("Too many pixels read");
          }
        }
        else{
          chunkheader -= 127;
          readColorBuffer(colorbuffer);
          for(short counter = 0; counter < chunkheader; counter++){																			// by the header
            data[currentbyte + 0] = colorbuffer[2];
            data[currentbyte + 1] = colorbuffer[1];
            data[currentbyte + 2] = colorbuffer[0];

            if(bytesPerPixel == 4)
              data[currentbyte + 3] = (byte)readUnsignedByte();

            currentbyte += bytesPerPixel;
            currentpixel++;
            if(currentpixel > pixelcount)
              throw new IOException("Too many pixels read");
          }
        }
      } while (currentpixel < pixelcount);
    }
    catch(Exception x){
      x.printStackTrace();
      LoggingSystem.getLogger(this).log(Level.WARNING, "exception while loading compressed", x.getMessage());
    }
  }

  private void readColorBuffer(byte[] buffer){
    buffer[0] = (byte)readUnsignedByte();
    buffer[1] = (byte)readUnsignedByte();
    buffer[2] = (byte)readUnsignedByte();
  }

  private int readUnsignedByte(){
	return (int) fileContents[filePointer++] & 0xFF;
  }

  private short readShort(){
    int s1 = (fileContents[filePointer++] & 0xFF),
        s2 = (fileContents[filePointer++] & 0xFF) << 8;
    return ((short)(s1 | s2));
  }

}