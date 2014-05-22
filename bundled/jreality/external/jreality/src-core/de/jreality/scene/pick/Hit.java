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


package de.jreality.scene.pick;

import java.util.Arrays;
import java.util.Comparator;

import de.jreality.math.Matrix;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphNode;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.DataList;
import de.jreality.scene.data.DoubleArray;
import de.jreality.scene.data.DoubleArrayArray;
import de.jreality.scene.data.IntArray;

public class Hit implements PickResult {
  final SceneGraphPath path;
  final double[] pointWorld;
  final double[] pointObject;
  final double[] barycentric;
  double[] texCoords = null;
  final int pickType;
  final int index;
  final int triIndex;
  final double dist;
  final double affineCoordinate;		// pointWorld = lambda(from + affineCoord*to)

  public Hit(SceneGraphPath path, double[] pointWorld, double dist, double affineCoord,int pickType, int index,int triIndex) {
	  this(path, pointWorld, dist, affineCoord, null, pickType, index, triIndex);
  }
  
 public Hit(SceneGraphPath path, double[] pointWorld, double dist, double affineCoord, double[] bary, int pickType, int index,int triIndex) {
    this.path = (SceneGraphPath) path;
    Matrix m = new Matrix();
    path.getInverseMatrix(m.getArray());
    this.pointWorld= pointWorld;
    this.pointObject=m.multiplyVector(pointWorld);
    this.barycentric = bary == null ? null : bary.clone();
    this.dist = dist;
    this.affineCoordinate = affineCoord;
    this.pickType=pickType;
    this.index=index;
    this.triIndex=triIndex;
  }

  public SceneGraphPath getPickPath() {
      return path;
  }
  public double[] getWorldCoordinates() {
      return pointWorld;
  }
  public double[] getObjectCoordinates() {
      return pointObject;
  }
  public double getDist() {
    return dist;
  }

  public int getIndex() {
    return index;
  }

  public int getSecondaryIndex() {
	    return triIndex;
	  }


  private String nameForType(int pickType)	{
	  if (pickType == PickResult.PICK_TYPE_FACE) return "face";
	  if (pickType == PickResult.PICK_TYPE_LINE) return "edge";
	  if (pickType == PickResult.PICK_TYPE_POINT) return "point";
	  return "object"; 
  }
  public String toString() {
    StringBuffer sb = new StringBuffer();
    sb.append("AABB-Pick: ");
    sb.append(" dist=").append(dist);
    sb.append(" type=").append(nameForType(pickType));
    sb.append(" index=").append(index);
    sb.append(" tc=").append(Arrays.toString(getTextureCoordinates()));
    sb.append(" world=").append(Arrays.toString(pointWorld));
    sb.append(" path=").append(path.toString());
    sb.append(" affine coordinate =").append(affineCoordinate);
    return sb.toString();
  }

  public int getPickType() {
    return pickType;
  }

  public double getAffineCoordinate() {
    return affineCoordinate;
  }

  private int hasTextureCoordinates() {
      if(texCoords== null) {
          if(triIndex > -1) {
          SceneGraphNode end = path.getLastElement();
          if (end instanceof IndexedFaceSet) {
              IndexedFaceSet ifs = (IndexedFaceSet) end;
              DataList txc = ifs.getVertexAttributes(Attribute.TEXTURE_COORDINATES);
              if(txc != null) {
                  DataList indices=ifs.getFaceAttributes(Attribute.INDICES);
                  if(indices != null){
                      DoubleArrayArray points
                      =ifs.getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray();
                      IntArray faceIndices = indices.item(index).toIntArray();
                      int l = faceIndices.size();
                      int ptIndex0 = faceIndices.getValueAt(0);
                      int ptIndex1 = faceIndices.getValueAt((triIndex+1)%l);
                      int ptIndex2 = faceIndices.getValueAt((triIndex+2)%l);
                      DoubleArray tc0 = txc.item(ptIndex0).toDoubleArray();
                      DoubleArray tc1 = txc.item(ptIndex1).toDoubleArray();
                      DoubleArray tc2 = txc.item(ptIndex2).toDoubleArray();
                      final int textureLength = tc0.getLength();
                      texCoords = new double[textureLength];
                      
                      // two methods the first one uses a smart algorithm
                      // grabed from Markus code in numerical methods
                      // the second is straight and dumb
                  if(true) {
                      // get the points
                      double[] a = new double[3];
                      DoubleArray     pt = points.item(ptIndex0).toDoubleArray();
                      a[0] = pt.getValueAt(0);
                      a[1] = pt.getValueAt(1);
                      a[2] = pt.getValueAt(2);
                      if(pt.size()==4) {
                          double w = pt.getValueAt(3);
                          a[0] /= w;
                          a[1] /= w;
                          a[2] /= w;
                      }
                      double[] b = new double[3];
                      pt = points.item(ptIndex1).toDoubleArray();
                      b[0] = pt.getValueAt(0);
                      b[1] = pt.getValueAt(1);
                      b[2] = pt.getValueAt(2);
                      if(pt.size()==4) {
                          double w = pt.getValueAt(3);
                          b[0] /= w;
                          b[1] /= w;
                          b[2] /= w;
                      }
                      double[] c = new double[3];
                      pt = points.item(ptIndex2).toDoubleArray();
                      c[0] = pt.getValueAt(0);
                      c[1] = pt.getValueAt(1);
                      c[2] = pt.getValueAt(2);
                      if(pt.size()==4) {
                          double w = pt.getValueAt(3);
                          c[0] /= w;
                          c[1] /= w;
                          c[2] /= w;
                      }

                      double bc[] = new double[3];
                      convertToBary(bc,a,b,c,pointObject);
                      for(int j = 0 ; j<textureLength;j++) {
                          texCoords[j] = bc[0]*tc0.getValueAt(j) + bc[1] * tc1.getValueAt(j) + bc[2] * tc2.getValueAt(j);
                      }
                  } else {                           
                      DoubleArray     pt = points.item(ptIndex2).toDoubleArray();
                      final double cx = pt.getValueAt(0);
                      final double cy = pt.getValueAt(1);
                      final double cz = pt.getValueAt(2);
                      
                      final double c = cx-pointObject[0];
                      final double f = cy-pointObject[1];
                      final double i = cz-pointObject[2];
                  
                      pt = points.item(ptIndex0).toDoubleArray();
                      final double a = pt.getValueAt(0)-cx;
                      final double d = pt.getValueAt(1)-cy;
                      final double g = pt.getValueAt(2)-cz;
                      pt = points.item(ptIndex1).toDoubleArray();
                      final double b = pt.getValueAt(0)-cx;
                      final double e = pt.getValueAt(1)-cy;
                      final double h = pt.getValueAt(2)-cz;
                      
                    // p = u ptIndex0 + v ptIndex1 + w ptIndex2 :
                      
                      final double u = (b*(f+i)-c*(e+h))/(a*(e+h)-b*(d+g));
                      final double v = (a*(f+i)-c*(d+g))/(b*(d+g)-a*(e+h));
                      final double w = 1-u-v;
                      if(u<0 || u>1 || v<0 ||v>1)
                          System.err.println("bad uv interpolate "+u+ " "+v);
//                      System.err.println("tc "+tc0.getValueAt(0)+" "+tc0.getValueAt(1));
//                      System.err.println("tc "+tc1.getValueAt(0)+" "+tc1.getValueAt(1));
//                      System.err.println("tc "+tc2.getValueAt(0)+" "+tc2.getValueAt(1));
                      for(int j = 0 ; j<textureLength;j++) {
                          texCoords[j] = u*tc0.getValueAt(j) + v * tc1.getValueAt(j) + + w * tc2.getValueAt(j);
                      }
//                      System.err.println("tc "+texCoords[0]+" "+texCoords[1]);

}
                      return texCoords.length;
                  }
              }
          }
      }
      texCoords = new double[0];
      }
      return texCoords.length;
  }
  
  private static final double EPS = 0.00001; 
  /*
   * calc barycentric coordinates bary for point x in anElement el, not
   * necessarily 0 <= b[i] <= 1
   */
  public static boolean  convertToBary(
      double[] bary,
      double[] x0,
      double[] x1,
      double[] x2,
      double[] x) {
      //TODO: use exception handling for degenerate situations
      //TODO: the return type should be void or double which should give
      // distance from plane

      int i0 = 0, i1 = 1, i2 = 2;

      double det;

      /* find two linear independent rows */
      for (;;) {
          det =
              x1[i0] * x2[i1]
                  - x1[i1] * x2[i0]
                  - (x0[i0] * x2[i1] - x0[i1] * x2[i0])
                  + x0[i0] * x1[i1]
                  - x0[i1] * x1[i0];

          if (Math.abs(det) > EPS)
              break;

          if (i1 == 1) {
              i1 = 2;
              i2 = 1;
          } else if (i0 == 0) {
              i0 = 1;
              i2 = 0;
          } else {
              
//            LoggingSystem.getLogger(Hit.class).fine("mBaryInElementConvertFromVec3: triangle degenerated?");
              //              
              //              fprintf(stderr, "gmBaryInElementConvertFromVec3: triangle
              // degenerated?\n");
              //              fprintf(stderr, "\tcan't compute barycentric
              // coordinates.\n");
              //              fprintf(stderr, "vertex0: (%f, %f, %f)\n", x0[0], x0[1],
              // x0[2]);
              //              fprintf(stderr, "vertex1: (%f, %f, %f)\n", x1[0], x1[1],
              // x1[2]);
              //              fprintf(stderr, "vertex2: (%f, %f, %f)\n", x2[0], x2[1],
              // x2[2]);
              //              fprintf(stderr, "point : (%f, %f, %f)\n", x[0], x[1], x[2]);
              //              fflush(stderr);

              return false;
          }
      }

      /* calculate barycentric coordinates */
      bary[0] =
          (x1[i0] * x2[i1]- x1[i1] * x2[i0]
           - (x[i0] * x2[i1] - x[i1] * x2[i0])
           + x[i0] * x1[i1]- x[i1] * x1[i0])
           / det;
      bary[1] =
          (x[i0] * x2[i1]
              - x[i1] * x2[i0]
              - (x0[i0] * x2[i1] - x0[i1] * x2[i0])
              + x0[i0] * x[i1]
              - x0[i1] * x[i0])
              / det;
      bary[2] = 1.0 - bary[0] - bary[1];

      /* test third row */
      if (Math
          .abs(x0[i2] * bary[0] + x1[i2] * bary[1] + x2[i2] * bary[2] - x[i2])
          > 1.e-3) {
    	  // this is not an error and should not in my opinion be set to info level -gunn 18.5.6
//        LoggingSystem.getLogger(Hit.class).fine("gmBaryInElementConvertFromVec3: test for third row failed."
//                  +Math.abs(x0[i2] * bary[0] + x1[i2] * bary[1] + x2[i2] * bary[2] - x[i2])
//                  );

          return false;
      }

      correct(bary);

      return true;
  }
  final static void correct(double[] bary) {

      double sum = 0;
      int j, k, i = 0;

      for (i = 0; i < 3; i++) {
          if (Math.abs(bary[i]) < EPS)
              bary[i] = 0;
          sum += bary[i];
      }

      for (i = 0; bary[i] == 0. && i < 3; i++);

      bary[(j = (i + 1) % 3)] /= sum;
      bary[(k = (i + 2) % 3)] /= sum;
      bary[i] = 1. - bary[j] - bary[k];
  }
  
  public double[] getTextureCoordinates() {
      hasTextureCoordinates();
          return texCoords;
  }
  
  public static class HitComparator implements Comparator<Hit> {
    public int compare(Hit hit1, Hit hit2) {
      // distance from ray
      double a = hit1.getAffineCoordinate();
      double b = hit2.getAffineCoordinate();
      return a>b ? 1 : b>a ? -1:0;
      
    }
  }

public double[] getBarycentric() {
	return barycentric;
}

}
