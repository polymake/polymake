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


package de.jreality.math;

import java.io.Serializable;

import de.jreality.util.ArrayUtility;

/**
* Interpolates a sorted function table <i>y[i] = f( x[i] )</i> by cubic B-splines.
* The x values have to be sorted, that is, <i>x[i] <= x[i+1]</i>.
* <p>
* The underlying mathematics is described in the classic scientific computing cookbook,
* <b>Numerical Recipes</b>.
* <p>
* There are three subclasses to choose from (TODO: document them):
* <ul>
* <li>Default: </li>
* <li>Natural: </li>
* <li>Periodic: </li>
* </ul>
*/
public class CubicBSpline
 implements Serializable {

double[] x;
double[] y;

double[] ddy;

/** arrays for equations. */
double[] a, b, c, r;

/** Size of table */
int n;

/** Number of equations; equals n for default and natural
* and n-1 for peridic spline. */
int N;

private CubicBSpline() {
}
/**
*
* Constructs natrual cubic b-spline for given table.
* @param x sorted x-values of table
* @param y y-values of table
*/
private CubicBSpline(double[] x, double[] y) {

 if (y.length != x.length) {
   throw new IllegalArgumentException("length of arrays must coinside");
 }

 if (y.length < 2) {
   throw new IllegalArgumentException(
       "length of arrays must be bigger then 1");
 }

 this.x = ArrayUtility.copy(x, null);
 this.y = ArrayUtility.copy(y, null);

}

public static class Default
   extends CubicBSpline {


   double dYInFirstX = 0;
   double dYInLastX = 0;

 /**
  * Constructs cubic b-spline with prescribed
  * first derivatives on boundary for given table.
  * @param x sorted x-values of table
  * @param y y-values of table
  */
 public Default(double[] x, double[] y,
                double dYInFirstX,
                double dYInLastX) {

   super(x, y);

   this.dYInFirstX = dYInFirstX;
   this.dYInLastX = dYInLastX;

   compute();
 }

 public Default(double[] x, double[] y) {

   this(x, y, 0, 0);
 }

 public double getDYInFirstX() {
   return dYInFirstX;
 }

 public void setDYInFirstX(double derivative) {

   if (dYInFirstX == derivative) {
     return;
   }

   dYInFirstX = derivative;

   compute();
 }

 public double getDYInLastX() {
   return dYInLastX;
 }

 public void setDYInLastX(double derivative) {

   if (dYInLastX == derivative) {
     return;
   }

   dYInLastX = derivative;

   compute();
 }

 void compute() {

   n = N = y.length;

   super.compute();

   c[0] = a[n - 1] = 1;

   r[0] = 6 / h(1) * ( (y[1] - y[0]) / h(1) - dYInFirstX);
   r[n -
       1] = -6 / h(n - 1) * ( (y[n - 1] - y[n - 2]) / h(n - 1) - dYInLastX);

   tridag(a, b, c, r, ddy);
 }

}

public static class Natural
   extends CubicBSpline {

 public Natural(double[] x, double[] y) {

   super(x, y);

   compute();
 }

 public void compute() {

   n = N = y.length;

   super.compute();

   c[0] = a[n - 1] = 0;
   r[0] = r[n - 1] = 0;

   tridag(a, b, c, r, ddy);
 }

}

public static class Periodic
   extends CubicBSpline {

 public Periodic(double[] x, double[] y) {

   super(x, y);

   compute();
 }

 public void compute() {

   n = y.length;
   N = n - 1;

   super.compute();

   final double lambda0 = h(1) / (h(1) + h(N)); //N-1 ???
   final double mu0 = 1 - lambda0;

   final double beta = mu0;

   final double alpha = lambda(N - 1);
   c[0] = lambda0;

   r[0] = 6 / (h(1) + h(N)) *
       ( (y[1] - y[0]) / h(1) - (y[N] - y[N - 1]) / h(N));

   cyclic(a, b, c, alpha, beta, r, ddy);

   ddy[N] = ddy[0];
 }
}

void compute() {

 if (ddy == null || ddy.length != y.length) {
   ddy = new double[y.length];
 }
 if (a == null || a.length != N) {
   a = new double[N];
   b = new double[N];
   c = new double[N];
   r = new double[N];

   for( int i=0; i<N; i++ ) b[i]=2;
 }

 for (int i = 1; i < n - 1; i++) {
   a[i] = mu(i);
 }

 for (int i = 1; i < N - 1; i++) { // N-1 is important
   c[i] = lambda(i);
 }

 for (int i = 1; i < n - 1; i++) {
   r[i] = d(i);
 }

}

/**
* @param j = 1,...,n-1
* @return
*/

final double h(int j) {
 return x[j] - x[j - 1];
}

/**
* @param i = 1,...,n-2
* @return
*/
final double d(int j) {
 final int k = j + 1;
 final double h_j = h(j);
 final double h_k = h(k);
 return 6 / (h_j + h_k) * ( (y[k] - y[j]) / h_k - (y[j] - y[j - 1]) / h_j);
}

/**
* @param i = 1,...,n-2
* @return
*/
final double lambda(int j) {
 final int k = j + 1;
 final double h_j = h(j);
 final double h_k = h(k);
 return h_k / (h_j + h_k);
}

/**
* @param i = 1,...,n-2
* @return
*/
final double mu(int j) {
 final int k = j + 1;
 final double h_j = h(j);
 final double h_k = h(k);
 return h_j / (h_j + h_k);
}

int klo, khi;

final private void searchInterval(double X) {
 klo = 0;
 khi = n-1;
 while (khi - klo > 1) {
   int k = (khi + klo) >> 1;
   if (x[k] > X) {
     khi = k;
   }
   else {
     klo = k;
   }
 }
}

public double valueAt(double x) {
 return valueAt(x, 0);
}

public double valueAt(double X, int derivative) {
 searchInterval(X);

 double h = x[khi] - x[klo];

 if (h == 0.0) {
   throw new RuntimeException();
 }

 double a = (x[khi] - X) / h;
 double b = (X - x[klo]) / h;

 switch (derivative) {
   case 0:
     return a * y[klo] + b * y[khi] +
         ( (a * a * a - a) * ddy[klo] + (b * b * b - b) * ddy[khi]) * (h * h) /
         6.0;
   case 1:
     return (y[khi] - y[klo]) / h -
         ( (3 * a * a - 1) * ddy[klo] - (3 * b * b - 1) * ddy[khi]) * h / 6;
   case 2:
     return a * ddy[klo] + b * ddy[khi];
   default:
     return 0;
 }
}

public int getLengthOfTable() {
 return n;
}

public double[] getX() {
 return (double[]) x.clone();
}

public double getX(int index) {
 return x[index];
}

public double getY(int index) {
 return y[index];
}

public double[] getY() {
 return (double[]) y.clone();
}

public double[] getDDY() {
 return (double[]) ddy.clone();
}

public void setX(double[] x) {

 if (x.length != n) {
   throw new IllegalArgumentException("x array has wrong size");
 }

 if (ArrayUtility.equal(this.x, x)) {
   return;
 }

 this.x = ArrayUtility.copy(x, this.x);

 compute();
}

public void setY(double[] y) {

 if (y.length != n) {
   throw new IllegalArgumentException("y array has wrong size");
 }

 if (ArrayUtility.equal(this.y, y)) {
   return;
 }

 this.y = ArrayUtility.copy(y, this.y);

 compute();
}

public void setXY(int index, double X, double Y) {
 if (index != 0 && x[index - 1] >= X) {
   throw new IllegalArgumentException
       ("x value in wrong range. It must hold: x[index-1] < x < x[index+1]");
 }

 if (index != n - 1 && X >= x[index + 1]) {
   throw new IllegalArgumentException
       ("x value in wrong range. It must hold: x[index-1] < x < x[index+1]");
 }

 if (this.x[index] == X && this.y[index] == Y) {
   return;
 }

 this.x[index] = X;
 this.y[index] = Y;

 compute();
}

public int getIndexOfClosestXValue(final double X) {
 if (X <= x[0]) {
   return 0;
 }

 if (X >= x[n - 1]) {
   return n - 1;
 }

 int klo = 0;
 int khi = n - 1;
 while (khi - klo > 1) {
   int k = (khi + klo) >> 1;
   if (x[k] > X) {
     khi = k;
   }
   else {
     klo = k;
   }
 }

 return X - x[klo] < x[khi] - X ? klo : khi;
}

public void setXY(double[] x, double[] y) {

 if (y.length != x.length) {
   throw new IllegalArgumentException("length of arrays must coinside");
 }

 if (y.length < 2) {
   throw new IllegalArgumentException(
       "length of arrays must be bigger then 1");
 }

 if (ArrayUtility.equal(this.y, y)) {
   if (ArrayUtility.equal(this.x, x)) {
     return;
   }
this.x = ArrayUtility.copy(x, this.x);
 }
 else {
   this.x = ArrayUtility.copy(x, this.x);
   this.y = ArrayUtility.copy(y, this.y);
 }

 compute();
}

double[] tmp1, tmp2, tmp3, tmp4, tmp5;

void tridag(double a[], double b[], double c[],
           double r[], double x[]) {

 if (tmp5 == null || tmp5.length != a.length) {
   tmp5 = new double[a.length];
 }

 tridag(a, b, c, r, x, tmp5);
}

void tridag(double a[], double b[], double c[],
           double r[], double x[], double[] tmp) {

 final int n = a.length;

 final double[] gam = tmp;

 if (b[0] == 0.0) {
   throw new RuntimeException("tridiag not diagonal dominant");
 }

 double bet = b[0];

 x[0] = r[0] / bet;

 for (int j = 1; j < n; j++) {
   gam[j] = c[j - 1] / bet;
   bet = b[j] - a[j] * gam[j];
   if (bet == 0.0) {
     throw new RuntimeException("tridiag not diagonal dominant");
   }
   x[j] = (r[j] - a[j] * x[j - 1]) / bet;
 }

 for (int j = (n - 2); j >= 0; j--) {
   x[j] -= gam[j + 1] * x[j + 1];
 }
}

void cyclic(double a[], double b[], double c[],
           double alpha, double beta,
           double r[], double x[]) {

 if (tmp1 == null || tmp1.length != a.length) {
   tmp1 = new double[a.length];
   tmp2 = new double[a.length];
   tmp3 = new double[a.length];
   tmp4 = new double[a.length];
 }

 cyclic(a, b, c, alpha, beta, r, x, tmp1, tmp2, tmp3, tmp4);
}

void cyclic(double a[], double b[], double c[],
           double alpha, double beta,
           double r[], double x[],
           double[] tmp1, double[] tmp2,
           double[] tmp3, double[] tmp4) {

 final int n = a.length;

 final double[] bb = tmp1;
 final double[] u = tmp2;
 final double[] z = tmp3;

 if (n <= 2) {
   new IllegalArgumentException("n too small in cyclic");
 }

 double gamma = -b[0];

 bb[0] = b[0] - gamma;
 bb[n - 1] = b[n - 1] - alpha * beta / gamma;
 for (int i = 1; i < n - 1; i++) {
   bb[i] = b[i];

 }
 tridag(a, bb, c, r, x, tmp4);

 u[0] = gamma;
 u[n - 1] = alpha;
 for (int i = 1; i < n - 1; i++) {
   u[i] = 0.0;

 }
 tridag(a, bb, c, u, z, tmp4);

 double fact = (x[0] + beta * x[n - 1] / gamma) /
     (1.0 + z[0] + beta * z[n - 1] / gamma);
 for (int i = 0; i < n; i++) {
   x[i] -= fact * z[i];
 }
}

}