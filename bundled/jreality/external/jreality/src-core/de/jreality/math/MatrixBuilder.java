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

import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.Transformation;

/**
 *
 * <p>
 * This class wraps a {@link de.jreality.math.Matrix} instance for easy specification of
 * concatenated operations.
 * <br> All the static methods are factory methods that
 * create an instance for a selected metric.
 * <br> <b>Note:</b> the factory methods with Transformation as a
 * parameter copy the underlying <code>double[]</code> and wrap
 * the copy into a new Matrix instance - the factory methods that
 * take a Matrix as a parameter simply work on the given Matrix.
 * Finally, the factory methods without parameters create a
 * new identity matrix to act on.
 * </p>
 * <p>
 * The instance methods which carry out matrix operations 
 * are always applyed on the right hand side of the
 * current value of the matrix. All
 * these methods return this instance as value, so that one can do
 * many calls in a row.
 * </p>
 * <pre>
 * Matrix m = MatrixBuilder.euclidean()
 *            .translate(2,2,2)
 *            .rotate(Math.PI/2, 1, 1, 1)
 *            .scale(3,1,2)
 *            .getMatrix();
 * 
 * SceneGraphComponent camCom = new SceneGraphComponent();
 * MatrixBuilder.euclidean().translate(0,2,3)
 *              .rotateFromTo(new double[]{0,0,-1}, new double[]{0,-2,-3})
 *              .scale(2)
 *              .assignTo(camComp); // Transformation gets set and assigned
 * </pre>
 * 
 * For explanation of metric, see {@link de.jreality.math.P3 P3}.
 * @author weissman
 */
public final class MatrixBuilder {
  
  private final Matrix matrix;
  private final int metric;

  private final double[] tmp = new double[16];
  

  /**
   * Create a matrix builder which generates isometries with respect to euclidean metric.
   * @param m
   * @return
   */public static MatrixBuilder euclidean(Transformation m) {
		Matrix mat=(m!=null) ? new Matrix(m) : new Matrix();
	    return new MatrixBuilder(mat, Pn.EUCLIDEAN);
	  }

  public static MatrixBuilder euclidean(Matrix m) {
	    return new MatrixBuilder(m, Pn.EUCLIDEAN);
	  }
	  
  /**
   * Create a matrix builder which generates isometries with respect to euclidean metric.
   * @param m
   * @return
   */public static MatrixBuilder euclidean() {
	    return euclidean(new Matrix());
	  }

   public static MatrixBuilder euclidean(SceneGraphComponent cmp) {
		if (cmp.getTransformation() == null) return euclidean();
		else return euclidean(cmp.getTransformation());
	}

  public static MatrixBuilder hyperbolic(Transformation m) {
	Matrix mat=(m!=null) ? new Matrix(m) : new Matrix();
    return new MatrixBuilder(mat, Pn.HYPERBOLIC);
  }
  
  public static MatrixBuilder hyperbolic(Matrix m) {
    return new MatrixBuilder(m, Pn.HYPERBOLIC);
  }
  
  /**
   * Create a matrix builder which generates isometries with respect to hyperbolic (or (3,1)) metric.
   * @param m
   * @return
   */public static MatrixBuilder hyperbolic() {
    return hyperbolic(new Matrix());
  }

   public static MatrixBuilder hyperbolic(SceneGraphComponent cmp) {
		if (cmp.getTransformation() == null) return hyperbolic();
		else return hyperbolic(cmp.getTransformation());
	}

  public static MatrixBuilder elliptic(Transformation m) {
	Matrix mat=(m!=null) ? new Matrix(m) : new Matrix();
    return new MatrixBuilder(mat, Pn.ELLIPTIC);
  }
  
  public static MatrixBuilder elliptic(Matrix m) {
    return new MatrixBuilder(m, Pn.ELLIPTIC);
  }
  
  /**
   * Create a matrix builder which generates isometries with respect to elliptic (or (4,0)) metric.
   * @param m
   * @return
   */public static MatrixBuilder elliptic() {
    return elliptic(new Matrix());
  }

   public static MatrixBuilder elliptic(SceneGraphComponent cmp) {
		if (cmp.getTransformation() == null) return elliptic();
		else return elliptic(cmp.getTransformation());
	}

   public static MatrixBuilder projective(Transformation m) {
	Matrix mat=(m!=null) ? new Matrix(m) : new Matrix();
    return new MatrixBuilder(mat, Pn.PROJECTIVE);
  }
  
  public static MatrixBuilder projective(Matrix m) {
    return new MatrixBuilder(m, Pn.PROJECTIVE);
  }
  
  /**
   * Create a matrix builder which strictly speaking doesn't know about metric: purely projective.
   * Results of using the isometry methods on this instance are undefined.
   * @param m
   * @return
   */public static MatrixBuilder projective() {
    return projective(new Matrix());
  }

  /**
   * This constructor accepts the metric as an argument.
   * It's often convenient to be able to specify the metric in this way rather than 
   * searching for the specific metric-specific method.
   */
  public static MatrixBuilder init(Matrix m, int metric)	{
  	return new MatrixBuilder(m==null ? new Matrix() : m, metric);
  }
  
  protected MatrixBuilder(Matrix m, int metric) {
    matrix = m;
    this.metric = metric;
  }

  public MatrixBuilder rotate(double angle, double axisX, double axisY, double axisZ) {
    return rotate(angle, new double[]{axisX, axisY, axisZ});
  }
  
  public MatrixBuilder rotate(double angle, double[] axis) {
    P3.makeRotationMatrix(tmp, axis, angle);
    matrix.multiplyOnRight(tmp);
    return this;
  }

  /**
   * rotate about the axis through the points p1 and p2
   * @param p1 first point on axis
   * @param p2 second point on axis
   * @param angle the angle to rotate
   * @return a MatrixBuilder...
   * @see P3#makeRotationMatrix(double[], double[], double[], double, int)
   */
  public MatrixBuilder rotate(double[] p1, double[] p2, double angle) {
    P3.makeRotationMatrix(tmp, p1, p2, angle, metric);
    matrix.multiplyOnRight(tmp);
    return this;
  }
  
  public MatrixBuilder rotateX(double angle) {
    P3.makeRotationMatrixX(tmp, angle);
    matrix.multiplyOnRight(tmp);
    return this;
  }
  
  public MatrixBuilder rotateY(double angle) {
    P3.makeRotationMatrixY(tmp, angle);
    matrix.multiplyOnRight(tmp);
    return this;
  }
  
  public MatrixBuilder rotateZ(double angle) {
    P3.makeRotationMatrixZ(tmp, angle);
    matrix.multiplyOnRight(tmp);
    return this;
  }
  
  /**
   * A rotation which takes vector <i>v1</i> to vector <i>v2</i>. These vectors
   * do not need to be normalized.
   * @param v1
   * @param v2
   * @return
   * @see P3#makeRotationMatrix(double[], double[], double[])
   */
  public MatrixBuilder rotateFromTo(double[] v1, double[] v2) {
    P3.makeRotationMatrix(tmp, v1, v2);
    matrix.multiplyOnRight(tmp);
    return this;
  }
  
  /**
   * 
   * @param scale
   * @return
   * @see P3#makeStretchMatrix(double[], double)
   */
  public MatrixBuilder scale(double scale) {
	    matrix.multiplyOnRight(P3.makeStretchMatrix(tmp, scale));
	    return this;
	  }

  /**
   * 
   * @param scale
   * @return
   * @see P3#makeStretchMatrix(double[], double[])
   */public MatrixBuilder scale(double[] scale) {
	    P3.makeStretchMatrix(tmp, scale);
	    matrix.multiplyOnRight(tmp);
	    return this;
	  }
  
  /**
   * 
   * @param scaleX
   * @param scaleY
   * @param scaleZ
   * @see P3#makeStretchMatrix(double[], double, double, double)
   * @return
   */
   public MatrixBuilder scale(double scaleX, double scaleY, double scaleZ) {
    P3.makeStretchMatrix(tmp, new double[]{scaleX, scaleY, scaleZ, 1});
    matrix.multiplyOnRight(tmp);
    return this;
  }

   public MatrixBuilder skew(int i, int j, double val)	{
	   P3.makeSkewMatrix(tmp, i, j, val);
	   matrix.multiplyOnRight(tmp);
	   return this;
  }
  
   public MatrixBuilder translate(double[] vector) {
	    P3.makeTranslationMatrix(tmp, vector, metric);
	    matrix.multiplyOnRight(tmp);
	    return this;
	  }
  public MatrixBuilder translate(double[] from, double[] to) {
	    P3.makeTranslationMatrix(tmp, from, to, metric);
	    matrix.multiplyOnRight(tmp);
	    return this;
	  }
	  
  /**
   * 
   * @param dx
   * @param dy
   * @param dz
   * @return
   * @see P3#makeTranslationMatrix(double[], double[], int)
   */public MatrixBuilder translate(double dx, double dy, double dz) {
    return translate(new double[]{dx, dy, dz});
  }
  
  /**
   * @param p1
   * @param p2
   * @return
   * @see P3#makeTranslationMatrix(double[], double[], double[], int)
   */
   public MatrixBuilder translateFromTo(double[] p1, double[] p2) {
    P3.makeTranslationMatrix(tmp, p1, p2, metric);
    matrix.multiplyOnRight(tmp);
    return this;
  }

  /**
   * reflects the wrapped Matrix at the plane
   * determined by the the given 3 points
   * 
   * @param v1 first point on reflection plane
   * @param v2 second point on reflection plane
   * @param v3 third point on reflection plane
   * 
   * @return this
   */
  public MatrixBuilder reflect(double[] v1, double[] v2, double[] v3) {
    return reflect(P3.planeFromPoints(null, v1, v2, v3));
  }
  
  /**
   * reflects the wrapped Matrix at the plane
   * 
   * @param plane
   * 
   * @return this
   * @see P3#makeReflectionMatrix(double[], double[], int)
   */
  public MatrixBuilder reflect(double[] plane) {
    P3.makeReflectionMatrix(tmp, plane, metric);
    matrix.multiplyOnRight(tmp);
    return this;
  }
  
  public MatrixBuilder conjugateBy(double[] c)	{
	  Rn.conjugateByMatrix(matrix.getArray(), matrix.getArray(), c);
	  return this;
  }
  /**
   * multiplies the given Matrix on the right hand side
   * @param matrix
   * @return this
   */
  public MatrixBuilder times(Matrix matrix) {
    return times(matrix.getArray());
  }
  
  /**
   * multiplies <i>array</i> (considered as 4x4 matrix) on the right hand side
   * @param array
   * @return this
   */
  public MatrixBuilder times(double[] array) {
    matrix.multiplyOnRight(array);
    return this;
  }
  
  /**
   * assigns ID to the underlying matrix 
   * @return
   */
  public MatrixBuilder reset() {
    matrix.assignIdentity();
    return this;
  }
  
  public Matrix getMatrix() {
    return matrix;
  }
  public void assignTo(SceneGraphComponent comp) {
    matrix.assignTo(comp);
  }
  public void assignTo(Transformation trafo) {
    matrix.assignTo(trafo);
  }
  public void assignTo(double[] array) {
    matrix.assignTo(array);
  }
  public void assignTo(Matrix m) {
    matrix.assignTo(m);
  }

	public double[] getArray() {
		return matrix.getArray();
	}

}
