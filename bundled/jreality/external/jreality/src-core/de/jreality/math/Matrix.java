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

import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.Transformation;
import de.jreality.scene.data.DoubleArray;

/**
 * A simple wrapper class for 4x4 real matrices.  The matrix is represented as a
 * linear array of 16 values (<tt>double[16]</tt> in order to avoid problems with Java's
 * multi-dimensional arrays. The elements are listed in row/column order and act on column vectors
 * sitting to the right of the matrix.
 * </p>
 * <p>
 * This class is not supposed to be a replacement for a full-fledged mathematical
 * package. It provides a convenient wrapper for double arrays that offers some basic
 * functionality for multiplying and inverting matrices and such, but if you want to
 * do more involved stuff, you probably want to use a dedicated math library.
 * 
 * @see de.jreality.math.Rn
 * @see de.jreality.math.P3
 * @see de.jreality.math.Pn
 * @author weissman
 *
 **/
public class Matrix implements Serializable {
  
	public final static double TOLERANCE = Rn.TOLERANCE;
	protected double[] matrix;
	  
	
	public Matrix() {
		this(Rn.setIdentityMatrix(new double[16]));
	}

	/**
	 * copy constructor
	 * @param T
	 */
	public Matrix(Matrix T) {
		matrix = new double[16];
		System.arraycopy(T.matrix, 0, matrix, 0, 16);
	}

    /**
     * plain wrapper for the array m; does NOT make a copy of m!
     * @param m the double array to be wrapped by this Matrix
     */
    public Matrix(double[] m) {
        if (m == null)
            m = Rn.setIdentityMatrix(new double[16]);
        if (m.length != 16)
            throw new IllegalArgumentException(
                    "invalid dimension for 4x4 matrix");
        matrix = m;
    }
    /**
     * TODO
     */
    public Matrix(double x00, double x01, double x02, double x03, double x10,
      double x11, double x12, double x13, double x20, double x21, double x22,
      double x23, double x30, double x31, double x32, double x33) {
    this(new double[] { x00, x01, x02, x03, x10, x11, x12, x13, x20, x21, x22,
        x23, x30, x31, x32, x33 });
  }

    /**
     * this constructor copies the content of the given DoubleArray. Note: the
     * given DoubleArray must have length == 16
     * 
     * @param the
     *          DoubleArray to copy and wrap
     */
    public Matrix(DoubleArray data) {
        this(data.toDoubleArray(null));
    }

    /**
     * this constructor copies the content of the given Transformation.
     * @param the Transformation to copy and wrap
     */
    public Matrix(Transformation data) {
        this(data == null ? null : data.getMatrix());
    }

	/**
	 * @param A
	 * @param B
	 * @return A*B
	 */
	public static Matrix times(Matrix A, Matrix B) {
		return new Matrix(Rn.times(null, A.matrix, B.matrix));
	}

	/**
	 * @param A
	 * @param B
	 * @return A+B
	 */
	public static Matrix sum(Matrix A, Matrix B) {
		return new Matrix(Rn.add(null, A.matrix, B.matrix));
	}

	public static Matrix power(Matrix A, int n)	{
		Matrix pow = new Matrix();
		for (int i = 0; i<n; ++i)
			pow.multiplyOnLeft(A);
		return pow;
	}
	/**
	 * 
	 * @param A
	 * @param B
	 * @return B * A * B^-1
	 */
	public static Matrix conjugate(Matrix A, Matrix B) {
		return new Matrix(Rn.conjugateByMatrix(null, A.matrix, B.matrix));
	}

  /**
   * this flag is kept for extending classes, that need to know
   * whether the matrix aray was changed. It's their responsibility
   * to reset this flag.
   */
  protected transient boolean matrixChanged=true;

	/**
	 * copies initValue
	 * @param initValue
	 */
	public void assignFrom(double[] initValue) {
    matrixChanged = true;
		System.arraycopy(initValue, 0, matrix, 0, 16);
	}

	/**
	 * copies initValue
	 * @param initValue
	 */
	public void assignFrom(Matrix initValue) {
    matrixChanged = true;
		System.arraycopy(initValue.matrix, 0, matrix, 0, 16);
	}
	
  /**
   * copies initValue
   * @param initValue
   */
	public void assignFrom(DoubleArray data) {
	matrixChanged = true;
    	if (data.getLength() != 16)
			throw new IllegalArgumentException(
					"invalid dimension for 4x4 matrix");
		data.toDoubleArray(matrix);
	}

  /**
   * copies initValue
   * @param initValue
   */
  public void assignFrom(Transformation trafo) {
    matrixChanged = true;
    trafo.getMatrix(matrix);
  }
  
  /**
   * TODO: assign single values!
   */
  public void assignFrom(double x00, double x01, double x02, double x03, double x10,
      double x11, double x12, double x13, double x20, double x21, double x22,
      double x23, double x30, double x31, double x32, double x33) {
    assignFrom(new double[] { x00, x01, x02, x03, x10, x11, x12, x13, x20, x21, x22,
        x23, x30, x31, x32, x33 });
  }


  public void assignTo(double[] array) {
    System.arraycopy(matrix, 0, array, 0, 16);
  }
  
  public void assignTo(Matrix m) {
    m.assignFrom(matrix);
  }
  
  public void assignTo(Transformation trafo) {
    trafo.setMatrix(matrix);
  }

  /**
   * Set the matrix of the transformation of <i>comp</i> to be this instance.  If <i>comp</i> has 
   * no Transformation, create one.
   * @param comp
   */public void assignTo(SceneGraphComponent comp) {
    Transformation t = comp.getTransformation();
    if (t == null) comp.setTransformation(new Transformation());
    assignTo(comp.getTransformation());
  }

	public void assignIdentity() {
    matrixChanged = true;
		Rn.setIdentityMatrix(matrix);
	}

	public double getDeterminant() {
		return Rn.determinant(matrix);
	}

	public double getTrace() {
		return Rn.trace(matrix);
	}

	public double getEntry(int row, int column) {
		return matrix[4 * row + column];
	}

	public void setEntry(int row, int column, double value) {
    if (matrix[4 * row + column] != value) matrixChanged = true;
		matrix[4 * row + column] = value;
	}

	public double[] getRow(int i) {
		return new double[] { matrix[4 * i], matrix[4 * i + 1],
				matrix[4 * i + 2], matrix[4 * i + 3] };
	}

	public void setRow(int i, double[] v) {
    matrixChanged = true;
    matrix[4 * i] = v[0];
		matrix[4 * i + 1] = v[1];
		matrix[4 * i + 2] = v[2];
		matrix[4 * i + 3] = v[3];
	}

	public double[] getColumn(int i) {
		return new double[] { matrix[i], matrix[i + 4], matrix[i + 8],
				matrix[i + 12] };
	}

  /**
   * assigns the values of the ith column with the values from v.
   * if v.length == 3, then the 4th entry of the column is set to 0.
   * @param i
   * @param v
   */
	public void setColumn(int i, double[] v) {
    matrixChanged = true;
		matrix[i] = v[0];
		matrix[i + 4] = v[1];
		matrix[i + 8] = v[2];
		matrix[i + 12] = (v.length > 3) ? v[3] : 0;
	}
    
	/**
	 * 
	 * @return reference to the current matrix
	 */
	public double[] getArray() {
		return matrix;
	}

	/**
	 * Copy the current matrix into <i>aMatrix</i> and return it.
	 * @param aMatrix
	 * @return  the filled in matrix
	 */
	public double[] writeToArray(double[] aMatrix) {
		if (aMatrix != null && aMatrix.length != 16)
			throw new IllegalArgumentException("matrix must have length 16");
		double[] copy = aMatrix == null ? new double[16] : aMatrix;
		System.arraycopy(matrix, 0, copy, 0, 16);
		return copy;
	}

	/**
	 * Let M be the current matrix. Then form the matrix product M*T and store it in M.
	 * 
	 * @param aMatrix
	 */
	public void multiplyOnRight(double[] T) {
    matrixChanged = true;
		Rn.times(matrix, matrix, T);
	}

	/**
	 * Let M be the current matrix. Then form the matrix product M*T and store it in M.
	 * 
	 * @param aMatrix
	 */
	public void multiplyOnRight(Matrix T) {
		multiplyOnRight(T.matrix);
	}

	/**
	 * Let M be the current matrix. Then form the matrix product T*M and store it in M.
	 * @param aMatrix
	 */
	public void multiplyOnLeft(double[] T) {
    matrixChanged = true;
		Rn.times(matrix, T, matrix);
	}

	/**
	 * Let M be the current matrix. Then form the matrix product T*M and store it in M.
	 * @param aMatrix
	 */
	public void multiplyOnLeft(Matrix T) {
		multiplyOnLeft(T.matrix);
	}

	/**
	 * Let M be the current Matrix.
	 * 
	 * assigns T * M * T^-1 
	 * 
	 * @param T
	 */
	public void conjugateBy(Matrix T) {
		matrixChanged = true;
		Rn.conjugateByMatrix(matrix, matrix, T.matrix);
	}

	/**
	 * Let M be the current Matrix.
	 * 
	 * assigns M + T 
	 * 
	 * @param T the matrix to add
	 */
	public void add(Matrix T) {
		matrixChanged = true;
		Rn.add(matrix, matrix, T.matrix);
	}

	/**
	 * Let M be the current Matrix.
	 * 
	 * assigns M - T 
	 * 
	 * @param T the matrix to subtract
	 */
	public void subtract(Matrix T) {
		matrixChanged = true;
		Rn.subtract(matrix, matrix, T.matrix);
	}

	/**
	 * Let M be the current Matrix.
	 * 
	 * assigns f * M 
	 * 
	 * @param f the scalar to multiply M with
	 */
	public void times(double f) {
		matrixChanged = true;
		Rn.times(matrix, f, matrix);
	}
	
	public Matrix getInverse() {
		return new Matrix(Rn.inverse(null, matrix));
	}

	public void invert() {
    matrixChanged = true;
		Rn.inverse(matrix, matrix);
	}

	public Matrix getTranspose() {
		return new Matrix(Rn.transpose(null, matrix));
	}

	public void transpose() {
		matrixChanged = true;
		Rn.transpose(matrix, matrix);
	}

    /**
     * Form the matrix-vector product <i>M.v</i> (<i>v</i> is column vector on the right).
     * @param v the vector v
     * @return M.v
     */public double[] multiplyVector(double[] v) {
        return Rn.matrixTimesVector(null, matrix, v);
    }
    
     /**
      * Form the matrix-vector product <i>M.v</i> (<i>v</i> is column vector on the right) and assign it to v.
      * @param vector the vector to transform
      */public void transformVector(double[] v) {
         Rn.matrixTimesVector(v, matrix, v);
     }
     
  public boolean equals(Matrix T) {
		return Rn.equals(matrix, T.matrix);
	}

  public String toString() {
    return Rn.matrixToString(matrix);
  }
  
  public boolean containsNanOrInfinite() {
	  for (double v : matrix) if (Double.isNaN(v) || Double.isInfinite(v)) return true;
	  return false;
  }

}