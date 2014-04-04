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

import de.jreality.math.Rn;
import de.jreality.util.LoggingSystem;
import de.jreality.util.Secure;
import de.jreality.util.SystemProperties;

/**
 * 
 * @version 1.0
 * @author <a href="mailto:hoffmann@math.tu-berlin.de">Tim Hoffmann</a>
 *
 */
public final class Polygon {
  public static final int MAXPOLYVERTEX;
  static {
    int pvInt = 14;
    try {
      String pv = Secure.getProperty(SystemProperties.SOFT_MAX_POLYVERTEX);
      if (pv != null) pvInt = Integer.parseInt(pv);
    } catch (Exception e) {
      // hm... now we are back with 14 verts each polygon
    }
    MAXPOLYVERTEX = pvInt;
    LoggingSystem.getLogger(Polygon.class).finer("Setting MAXPOLYVERTEX="+MAXPOLYVERTEX);
  }
  public static final int SX= 0;
  public static final int SY= 1;
  public static final int SZ= 2;
  public static final int SW = 3;
  public static final int R= 4;
  public static final int G= 5;
  public static final int B= 6;
  public static final int A= 7;
  
  public static final int U= 8;
  public static final int V= 9;
  
  public static final int NX= 10;
  public static final int NY= 11;
  public static final int NZ= 12;
  public static final int VERTEX_LENGTH= 13;

  private double centerZ;
  protected int length;
  protected int[] vertices= new int[MAXPOLYVERTEX];
  private PolygonShader shader;
  //		protected double transparency = 0.;

  public Polygon() {
    super();
    length= 0;
  }

  /**
   * 
   */
  public Polygon(int l) {
    super();
    length= l;
  }

  
  public final double getCenterZ() {
    return centerZ;
  }
  
  final void setCenterZ(double d) {
      centerZ = d;
  }
  /**
   * This method computes the z component of the polygon for sorting.
   * It should be called once after perspective transformation for all polygons, that
   * are be be sorted prior to rendering.
   * The polygons will be rendered front to back to have as little work in the @link PolygonRasterizer#setPixel
   * method. 
   * However, for transparent polygons, the method sets the centerZ property to 2-centerZ in order
   * to ensure, that transparent polygons are rendered last and in back to front order.
   * @param data
   */
  public final void computeCenterZ(final double data[]) {
    centerZ= 0;
    for (int i= 0; i < length; i++) {
      centerZ += data[vertices[i] + SZ];
    }
    centerZ /= length;
    // this was the version for drawing opaque polygons
    // front to back. This cannot be used with the
    // svg rasterizer so we make all back to front
//    if (shader.getVertexShader().getTransparency() != 0.)
//        centerZ= 2 - centerZ;
      //  centerZ= 1 - centerZ;
  }

  public final void computeMaxZ(final double data[]) {
      centerZ= -Double.MAX_VALUE;
      for (int i= 0; i < length; i++) {
          double d = data[vertices[i] + SZ]/data[vertices[i] + SW];
          if(d > centerZ) centerZ = d;
      }
      //centerZ= 1 - centerZ;
  }
  public final void computeMinZ(final double data[]) {
      centerZ= Double.MAX_VALUE;
      for (int i= 0; i < length; i++) {
          double d = data[vertices[i] + SZ]/data[vertices[i] + SW];
          if(d < centerZ) centerZ = d;
      }
      //centerZ= 1 - centerZ;
  }
  public final double getMinZ(final double data[]) {
      double minZ= Double.MAX_VALUE;
      for (int i= 0; i < length; i++) {
          double d = data[vertices[i] + SZ]/data[vertices[i] + SW];
          if(d < minZ) minZ = d;
      }
      return minZ;
  }
  public final double getMaxZ(final double data[]) {
      double maxZ= -Double.MAX_VALUE;
      for (int i= 0; i < length; i++) {
          double d = data[vertices[i] + SZ]/data[vertices[i] + SW];
          if(d > maxZ) maxZ = d;
      }
      return maxZ;
  }
  
  public final PolygonShader getShader() {
    return shader;
  }

  /**
   * Sets the shader.
   * @param shader The shader to set
   */
  public final void setShader(PolygonShader shader) {
    this.shader= shader;
  }

  public double[] getVertexNormal(int i, double[] data) {
      double[] x = new double[] {
          data[vertices[i]+SX] - data[vertices[(i-1+length)%length]+SX],
          data[vertices[i]+SY] - data[vertices[(i-1+length)%length]+SY],
          data[vertices[i]+SZ] - data[vertices[(i-1+length)%length]+SZ]
      };
      double[] y = new double[] {
              data[vertices[(i+1)%length]+SX] - data[vertices[i]+SX],
              data[vertices[(i+1)%length]+SY] - data[vertices[i]+SY],
              data[vertices[(i+1)%length]+SZ] - data[vertices[i]+SZ]
      };
      
      double[] xxy = new double[3];
      Rn.crossProduct(xxy,x,y);
      double l =Rn.euclideanNorm(xxy);
      if(l!= 0) {
          xxy[0] /=l;
          xxy[1] /=l;
          xxy[2] /=l;
      }
      return xxy;
  }
  public double[] getVertex(int i, double[] data) {
      double[] x = new double[] {
              data[vertices[i]+SX],
              data[vertices[i]+SY],
              data[vertices[i]+SZ]
      };
      
      return x;
  }

/**
 * @param vertexData
 */
public void print(double[] vertexData) {
   for (int i = 0; i < length; i++) {
       int pos =vertices[i];
       System.out.println(""+vertexData[pos+SX]+", "+vertexData[pos+SY]+", "+vertexData[pos+SZ]+", "+vertexData[pos+SW]);
}
    
}
  
}
