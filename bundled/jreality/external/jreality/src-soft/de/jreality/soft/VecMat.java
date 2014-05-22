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

/**
 * This class is a collection of useful methods for dealing with vectors
 * and matrices.
 * @version 1.0
 * @author <a href="mailto:timh@math.umass.edu">Tim Hoffmann</a>
 *
 */
class VecMat {

  public static final void vecAssign(double[] to, double[] from) {
    to[0]= from[0];
    to[1]= from[1];
    to[2]= from[2];
  }

  
  public static final void vecAssignPlus(final double[] to, final double x, final double y, final double z) {
      to[0] += x;
      to[1] += y;
      to[2] += z;
  }
  
  public static final void vecAssignPlus(final double[] to, final double[] a) {
      to[0] += a[0];
      to[1] += a[1];
      to[2] += a[2];
  }
  
  public static final void vecAssignPlus(float[] to, float[] a) {
    to[0] += a[0];
    to[1] += a[1];
    to[2] += a[2];
  }

  public static final void vecAssignMinus(final double[] to, final double[] a) {
      to[0] -= a[0];
      to[1] -= a[1];
      to[2] -= a[2];
  }

  public static final void vecAssignMinus(final double[] to, final double x, final double y, final double z) {
      to[0] -= x;
      to[1] -= y;
      to[2] -= z;
  }
  
  public static final void vecArrayToArray(double[][] v, double[] l) {
    int n= v[0].length;
    int k= v.length;
    //for(int i =0;i<k;i++) 
    //System.arraycopy(v[i],0,l,n*i,n);
    for (int i= 0; i < k; i++) {
      double d[]= v[i];
      int p= n * i;
      for (int j= 0; j < n; j++) l[p + j]= d[j];
    }
  }
  public static final void arrayToVecArray(double[] l, double[][] v) {
    int n= v[0].length;
    int k= v.length;
    //for(int i =0;i<k;i++) 
    //System.arraycopy(l,n*i,v[i],0,n);
    for (int i= 0; i < k; i++) {
      double[] d= v[i];
      int p= i * n;
      for (int j= 0; j < n; j++) d[j]= l[p + j];
    }

  }
  public static final void transform(final double m[], final double x,
    final double y, final double z, final double v[]) {
    for (int j= 0; j < 3; j++)
      v[j]=
        m[j * 4 + 0] * x + m[j * 4 + 1] * y + m[j * 4 + 2] * z + m[j * 4 + 3];
  }
  public static final void transform(final double m[], final double x,
    final double y, final double z, final double v[], final int s) {
    for (int j= 0; j < 3; j++)
      v[j + s]=
        m[j * 4 + 0] * x + m[j * 4 + 1] * y + m[j * 4 + 2] * z + m[j * 4 + 3];
  }
  
  public static final void transform(final double m[], final double x,
          final double y, final double z, final double w, final double v[]) {
      for (int j= 0; j < 4; j++)
          v[j]=
              m[j * 4 + 0] * x + m[j * 4 + 1] * y + m[j * 4 + 2] * z + m[j * 4 + 3] * w;
  }
  public static final void transform(final double m[], final double x,
          final double y, final double z, final double w, final double v[], final int s) {
      for (int j= 0; j < 4; j++)
          v[j + s]=
              m[j * 4 + 0] * x + m[j * 4 + 1] * y + m[j * 4 + 2] * z + m[j * 4 + 3] * w;
  }

  public static final void transformNormal(final double m[], final double x,
    final double y, final double z, final double v[]) {
    for (int j= 0; j < 3; j++)
      v[j]= m[j * 4 + 0] * x + m[j * 4 + 1] * y + m[j * 4 + 2] * z;
    normalize(v);
  }
  public static final void transformUnNormalized(final double m[],
    final double x, final double y, final double z, final double v[]) {
    for (int j= 0; j < 3; j++)
      v[j]= m[j * 4 + 0] * x + m[j * 4 + 1] * y + m[j * 4 + 2] * z;
  }
  public static final void transformUnNormalized(
    final double m[], final double x, final double y, final double z,
    final double v[], final int s) {
    for (int j= 0; j < 3; j++)
      v[j + s]= m[j * 4 + 0] * x + m[j * 4 + 1] * y + m[j * 4 + 2] * z;
  }
  public static final void transformNormal(
    final double m[], final double x, final double y, final double z,
    final double v[], final int s) {
    for (int j= 0; j < 3; j++)
      v[j + s]= m[j * 4 + 0] * x + m[j * 4 + 1] * y + m[j * 4 + 2] * z;
    normalize(v, s);
  }

  public static void assignIdentity(double[] m) {
            m[ 1] = m[ 2] = m[ 3] = 
    m[ 4] =         m[ 6] = m[ 7] = 
    m[ 8] = m[ 9] =         m[11] = 
    m[12] = m[13] = m[14] =         0.0;
    m[ 0] = m[ 5] = m[10] = m[15] = 1.0;
  }

  public static final void cross(double[] a, double[] b, double[] axb) {
    axb[0]= a[1] * b[2] - a[2] * b[1];
    axb[1]= a[2] * b[0] - a[0] * b[2];
    axb[2]= a[0] * b[1] - a[1] * b[0];
  }

  public static final double dot(double a[], double b[]) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
  }
  public static final double dot(double a[],int ai, double b[],int bi) {
      return a[ai] * b[bi] + a[ai+1] * b[bi+1] + a[ai+2] * b[bi+2];
  }

  public static final void multiply(double a[], double q) {
	  a[0]*=q;
	  a[1]*=q;
	  a[2]*=q;
  }
  public static final double[] product(double a[], double q) {
	  return new double[] { q*a[0], q*a[1], q*a[2] };
  }
  public static final double[] sum(double a[], double b[]) {
	  return new double[] { a[0]+b[0], a[1]+b[1], a[2]+b[2] };
  }
  public static final double[] difference(double a[], double b[]) {
	  return new double[] { a[0]-b[0], a[1]-b[1], a[2]-b[2] };
  }
  public static final double[] crossproduct(double a[], double b[]) {
	  double axb[]=new double[3];
	  cross(a, b, axb);
	  return axb;
  }
  
  public static final float norm(float[] v) {
    return (float)Math.sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
  }

  public static final double norm(double[] v) {
    return Math.sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
  }

  public static final void cross(double[] a, double[] b, float[] axb) {
    axb[0]= (float) (a[1] * b[2] - a[2] * b[1]);
    axb[1]= (float) (a[2] * b[0] - a[0] * b[2]);
    axb[2]= (float) (a[0] * b[1] - a[1] * b[0]);
  }

  public static final void normalize(final double v[]) {
    final double a= v[0];
    final double b= v[1];
    final double c= v[2];
    final double n= Math.sqrt(a * a + b * b + c * c);
    if (n == 0) return;
    v[0] = a/n;
    v[1] = b/n;
    v[2] = c/n;
  }
  public static final void normalize(final double v[], final int i) {
    final double a= v[i];
    final double b= v[i + 1];
    final double c= v[i + 2];
    final double n= Math.sqrt(a * a + b * b + c * c);
    if (n == 0) return;
    v[i]     = a/n;
    v[i + 1] = b/n;
    v[i + 2] = c/n;
  }

  public static final void normalize(float v[]) {
    final float n= (float)Math.sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    v[0] /= n;
    v[1] /= n;
    v[2] /= n;
  }

  public static final void multiply( final double[] l,
    final double[] r, final double[] dst) {
    for (int i= 0; i < 4; i++)
      for (int j= 0; j < 4; j++) {
        double sum= 0;
        for (int k= 0; k < 4; k++)
          sum += l[i * 4 + k] * r[k * 4 + j];
        dst[i * 4 + j]= sum;
      }
  }
  public static final void copyMatrix(final double[] a, final double[] b) {
    for (int ix= 0; ix < 16; ix++)
      b[ix]= a[ix];
  }

  public static final void multiplyFromRight(double[] m, double[] t) {
    double[] tmp= new double[16];
    copyMatrix(m, tmp);
    for (int i= 0; i < 4; i++)
      for (int j= 0; j < 4; j++) {
        double sum= 0;
        for (int k= 0; k < 4; k++)
          sum += tmp[i * 4 + k] * t[k * 4 + j];
        m[i * 4 + j]= sum;
      }
  }

  public static final void multiplyFromLeft(double[] m, double[] t) {
    double[] tmp= new double[16];
    copyMatrix(m, tmp);
    for (int i= 0; i < 4; i++)
      for (int j= 0; j < 4; j++) {
        double sum= 0;
        for (int k= 0; k < 4; k++)
          sum += t[i * 4 + k] * tmp[k * 4 + j];
        m[i * 4 + j]= sum;
      }
  }

  public static final void assignScale(final double[] m, final double s) {
    for (int i= 0; i < 4; i++)
      for (int j= 0; j < 4; j++)
        m[i * 4 + j]= (i == j ? s : 0);
    m[3 * 4 + 3]= 1;
  }

  public static final void assignScale(final double[] m,
    final double sx, final double sy, final double sz) {
    for (int i= 0; i < 4; i++)
      for (int j= 0; j < 4; j++)
        m[i * 4 + j]=
          (i == j ? (i == 0 ? sx : (i == 1 ? sy : (i == 2 ? sz : 1))) : 0);
    m[3 * 4 + 3]= 1;
  }

  public static final void assignRotationX(double[] m, double phi) {
    assignIdentity(m);
    m[1 * 4 + 1]= m[2 * 4 + 2]= Math.cos(phi);
    m[1 * 4 + 2]= -Math.sin(phi);
    m[2 * 4 + 1]= -m[1 * 4 + 2];
  }

  public static final void assignRotationY(double[] m, double phi) {
    assignIdentity(m);
    m[2 * 4 + 2]= m[0 * 4 + 0]= Math.cos(phi);
    m[2 * 4 + 0]= -Math.sin(phi);
    m[0 * 4 + 2]= -m[2 * 4 + 0];
  }

  public static final void assignRotationZ(double[] m, double phi) {
    assignIdentity(m);
    m[0 * 4 + 0]= m[1 * 4 + 1]= Math.cos(phi);
    m[0 * 4 + 1]= -Math.sin(phi);
    m[1 * 4 + 0]= -m[0 * 4 + 1];
  }

  public static final void assignTranslation(double[] m, double[] v) {
    assignIdentity(m);
    m[0 * 4 + 3]= v[0];
    m[1 * 4 + 3]= v[1];
    m[2 * 4 + 3]= v[2];
  }

  public static final void assignTranslation(double[] m,
    double v0, double v1, double v2) {
    assignIdentity(m);
    m[0 * 4 + 3]= v0;
    m[1 * 4 + 3]= v1;
    m[2 * 4 + 3]= v2;
  }

  public static void invert(double src[], double dst[]) {

    // adjoint cofactor:

    for (int i= 0; i < 3; i++)
      for (int j= 0; j < 3; j++) {
        int i0= (i + 1) % 3;
        int i1= (i + 2) % 3;
        int j0= (j + 1) % 3;
        int j1= (j + 2) % 3;
        dst[4 * j + i]= src[4 * i0 + j0] * src[4 * i1 + j1]
                      - src[4 * i0 + j1] * src[4 * i1 + j0];
      }

    // now determinant for the rotatin and scale part:

    double determinant= src[4 * 0 + 0] * dst[4 * 0 + 0]
                      + src[4 * 1 + 0] * dst[4 * 0 + 1]
                      + src[4 * 2 + 0] * dst[4 * 0 + 2];
    for (int i= 0; i < 3; i++)
      for (int j= 0; j < 3; j++)
        dst[4 * i + j] /= determinant;

    // finally the translation:

    for (int i= 0; i < 3; i++)
      dst[4 * i + 3]= - dst[4 * i + 0] * src[4 * 0 + 3]
                      - dst[4 * i + 1] * src[4 * 1 + 3]
                      - dst[4 * i + 2] * src[4 * 2 + 3];
    // TODO see if the following is o.k. : I added the following for the Inverse:
    dst[4 * 3 + 3]= 1;
  }

}
