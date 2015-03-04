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

import java.util.Arrays;
import java.util.logging.Level;

import de.jreality.util.LoggingSystem;

/**
 * Static methods for n-dimensional Euclidean vector space R<sup>n</sup>. All
 * vectors are of type double[].
 * <p>
 * This includes a set of methods related to linear transformations of Rn.
 * Because of the need to have matrices stored in contiguous memory (rather than
 * Java-style as an array of pointers), matrices are also of type double[].
 * Since all the matrices that occur here are square ones, the actual dimensions
 * can be calculated easily from the over-all length. For example, an array of
 * length 16 corresponds to a 4x4 matrix. Currently the limit on the size of
 * these square matrices is 10x10. Methods where there is a lower limit annotate
 * this restriction.
 * <p>
 * Conventions:
 * <ul>
 * <li>matrices act on <b>column vectors</b> standing to the <b> right</b> of
 * the matrix.</li>
 * <li>
 * Some software packages of this type attempt to provide many versions of the
 * same method, for example, <c>add()</c>, which have different number of
 * arguments depending on whether one of the arguments is re-used. This is
 * <b>not</b> the policy here. Each such method typically exists in only one
 * form. For example, in the case of <c>add()</c>, it has the form
 * <code> public static double[] add(double[] dst, double[]src1, double[] src2) </code>
 * . The operation is then given by <code> dst = src1 + src2 </code>. Care has
 * been taken to allow re-use of the same variable, for example, in the
 * invocation <code> Rn.add(a, a, a) </code> which adds a vector <tt> a</tt> to
 * itself and stores the result back into itself.</li>
 * <li>
 * Just as in the written form of the addition statement above, the destination
 * array comes <b>first</b> in the argument list. This is true for every method
 * which calculates a vector or matrix.</li>
 * <li>These methods generally return this vector or matrix. This allows nested
 * used of these methods; for example to add three vectors <i>a,b,c</i> together
 * into a fourth <i>d</i>: <code> Rn.add(d, c, Rn.add(null, a, b)) </code> in
 * the process.</li>
 * <li>
 * Methods return the computed vector or matrix, in general. It is in general
 * possible to pass in a <code>null</code> value for the destination vector or
 * matrix. The method itself in this case then allocates a new one, fills it
 * accordingly, and returns it. Unless otherwise noted, this is the behavior of
 * the static methods in this class.
 * <p>
 * </ul>
 * <p>
 * Methods try as much as possible to react reasonably to differing argument
 * types. In general, the methods check the lengths of the arguments, determine
 * a minimum valid length on which the desired operation can be carried out, and
 * does so. This allows some flexibility in terms of lengths of input vectors
 * which the following paragraph attempts to justify on practical grounds.
 * <p>
 * For example. the method
 * {@link #matrixTimesVector(double[], double[], double[])} is happiest when the
 * dimension of the square matrix matches the length of the input vectors.
 * However, it is often the case that users have dehomogenized coordinates for
 * vectors, that is, one dimension less than the dimension of the matrix. For
 * example, points in Euclidean 3-space are naturally stored as 3-vectors
 * although the natural setting for isometries is 4x4 matrices. In this case the
 * method assumes an implicit final coordinate of 1.0 and carries out the
 * multiplication accordingly. At the same time, the method also operates
 * correctly on 4-vectors.
 * <p>
 * Some commonly used methods come in two varieties: one acts on single vectors
 * (<code> double[] </code>) and another acts on array of vectors (
 * <code>double[][]</code>). Compare
 * {@link #matrixTimesVector(double[], double[], double[])} and
 * {@link #matrixTimesVector(double[][], double[], double[][])}.
 * <p>
 * 
 * @author Charles Gunn
 * @see de.jreality.math.Pn
 */
final public class Rn {

	/**
	 * A table of identity matrices for quick access.
	 */
	final static double[][] identityMatrices = new double[5][];

	/**
	 * The default tolerance used for checking numerical equality = 10E-8.
	 */
	public static final double TOLERANCE = 10E-8;
	/*
	 * A lookup table for finding the side of a square matrix from the number of
	 * its entries
	 */
	static {
		for (int i = 1; i < 5; ++i)
			identityMatrices[i] = identityMatrix(i);
	}

	/* no instance allowed */
	private Rn() {
		super();
	}

	/**
	 * @param ds
	 * @param ds2
	 */
	public static double[] abs(double[] dst, double[] src) {
		int n = src.length;
		if (dst == null)
			dst = new double[n];
		for (int i = 0; i < n; ++i)
			dst[i] = Math.abs(src[i]);
		return dst;

	}

	/**
	 * Add the vector <i>src1</i> to the vector <i>src2</i> and put the result
	 * in <i>dst</i>.
	 * 
	 * @param dst
	 * @param src1
	 * @param src2
	 * @return dst
	 */
	public static double[] add(double[] dst, double[] src1, double[] src2) {
		if (dst == null)
			dst = new double[src1.length];
		int n = src1.length;
		if (src1.length != src2.length)
			n = Math.min(Math.min(dst.length, src1.length), src2.length);
		for (int i = 0; i < n; ++i)
			dst[i] = src1[i] + src2[i];
		return dst;
	}

	/**
	 * Calculate the adjoint of the square matrix <i>src</i> and put the result
	 * into <i>dst</i>. The adjoint of a matrix is a same-size matrix, whose
	 * (i,j)th entry is the determinant of the sub-matrix of the original matrix
	 * obtained by deleting the (i)th row and (j)th column. The dimension of the
	 * input matrix can be no greater than 4.
	 * 
	 * @param dst
	 *            may be null
	 * @param src
	 *            may be = dst
	 * @return dst
	 */
	public static double[] adjoint(double[] dst, double[] src) {
		int n = mysqrt(src.length);
		int sgn = 1;
		// assert dim checks
		if (dst == null)
			dst = (double[]) src.clone();
		double[] out;
		boolean rewrite = false;
		if (dst == src) {
			out = new double[dst.length];
			rewrite = true;
		} else {
			out = dst;
		}
		for (int i = 0; i < n; ++i) {
			for (int j = 0; j < n; ++j) {
				out[i * n + j] = cofactor(src, i, j)
						* (((i + j) % 2 == 1) ? -1 : 1);
				sgn *= -1;
			}
		}
		if (rewrite)
			System.arraycopy(out, 0, dst, 0, dst.length);
		return dst;
	}

	/**
	 * Calculate the average of the elements in the vector list <i>vlist</i>.
	 * 
	 * @param dst
	 * @param vlist
	 * @return
	 */
	public static double[] average(double[] dst, double[][] vlist) {
		// assert dim check
		if (dst == null)
			dst = new double[vlist[0].length];
		if (vlist.length == 0)
			return null;
		double[] tmp = new double[dst.length];
		for (int i = 0; i < vlist.length; ++i) {
			add(tmp, tmp, vlist[i]);
		}
		times(dst, 1.0 / vlist.length, tmp);
		return dst;
	}

	/**
	 * Calculate the euclidean coordinates of the point whose barycentric
	 * coordinates with respect to the triangle <i>corners</i> are
	 * <i>coords</i>. This is just the weighted average of the corners of the
	 * triangle. Return the result in <i>dst</i>.
	 * 
	 * @param dst
	 *            double[n] (may be null)
	 * @param corners
	 *            double[3][n]
	 * @param weights
	 *            double[3]
	 * @return
	 */
	public static double[] barycentricTriangleInterp(double[] dst,
			double[][] corners, double[] weights) {
		double[] ddst;
		// assert coords.length == 3
		if (dst == null)
			ddst = new double[corners[0].length];
		else
			ddst = dst;
		int n = Math.min(corners[0].length, ddst.length);
		double[] tmp = new double[n];
		Arrays.fill(ddst, 0);
		for (int i = 0; i < 3; ++i) {
			add(ddst, ddst, times(tmp, weights[i], corners[i]));
		}
		return ddst;
	}

	/**
	 * Given a list of vectors, calculate the minimum and maximum values in each
	 * coordinate. Return the result in <i>bounds</i>. For example
	 * <i>bounds[0][1]</i> is the minimum y-value of the vectors in the list,
	 * etc.
	 * 
	 * @param bounds
	 *            double[2][n]
	 * @param vlist
	 *            double[][n]
	 * @return
	 */
	public static double[][] calculateBounds(double[][] bounds, double[][] vlist) {
		int vl = vlist[0].length;
		int bl = bounds[0].length;
		// assert dim check
		// if (bounds.length != 2 && bounds[0].length != n) bounds = new
		// double[2][n];
		if (vl > bl) {
			throw new IllegalArgumentException("invalid dimension");
		}

		// fill bounds with appropriate values
		for (int i = 0; i < vl; ++i) {
			bounds[0][i] = Double.MAX_VALUE;
			bounds[1][i] = -Double.MAX_VALUE;
		}
		for (int i = vl; i < bl; ++i) {
			bounds[0][i] = bounds[1][i] = 0.0;
		}
		for (int i = 0; i < vlist.length; ++i) {
			max(bounds[1], bounds[1], vlist[i]);
			min(bounds[0], bounds[0], vlist[i]);
			if (Double.isNaN(bounds[0][0]))
				throw new IllegalStateException("calculate bounds: nan");
		}
		return bounds;
	}

	/**
	 * Calculate the <i>(i,j)th cofactor</i> of the matrix <i>m</i> The
	 * dimension of the matrix <i>m</i> can be no greater than 5.
	 * 
	 * @param m
	 *            double[n][n]
	 * @param row
	 * @param column
	 * @return m
	 */
	public static double cofactor(double[] m, int row, int column) {
		int n = mysqrt(m.length);
		// assert dim check
		return determinant(submatrix((double[]) null, m, row, column));
	}

	/**
	 * Form the conjugate of the matrix <i>m</i> by the matrix <i>c</i>: <c>dst
	 * = c . m . Inverse(c)</c>
	 * 
	 * @param dst
	 *            double[n][n]
	 * @param m
	 *            double[n][n]
	 * @param c
	 *            double[n][n]
	 * @return dst
	 */
	public static double[] conjugateByMatrix(double[] dst, double[] m,
			double[] c) {
		int n = mysqrt(c.length);
		// assert dim checks
		if (dst == null)
			dst = new double[c.length];
		times(dst, c, times(null, m, inverse(null, c)));
		return dst;
	}

	/**
	 * inlines the given 2-dim array. Assumes equal length for sub-arrays
	 * 
	 * @return the target array (a new one if target == null)
	 */
	public static double[] convertArray2DToArray1D(double[] target,
			double[][] src) {
		// TODO decide if this belongs here -- doesn't have anything to do with
		// Geometry per se.
		final int slotLength = src[0].length;
		if (target == null)
			target = new double[src.length * slotLength];
		for (int i = 0; i < src.length; i++)
			for (int j = 0; j < slotLength; j++)
				target[i * slotLength + j] = src[i][j];
		return target;
	}

	public static double[] convertArray3DToArray1D(double[][][] V) {
		return convertArray3DToArray1D(V, 1, 1);
	}

	/**
	 * @param V
	 * @return
	 */
	public static double[] convertArray3DToArray1D(double[][][] V, int usample,
			int vsample) {
		int n = V.length;
		int m = V[0].length;
		int p = V[0][0].length;
		double[] newV = new double[((n + vsample - 1) / vsample)
				* ((m + usample - 1) / usample) * p];
		for (int i = 0, ind = 0; i < n; i += vsample) {
			for (int j = 0; j < m; j += usample) {
				for (int k = 0; k < p; k++, ind++)
					newV[ind] = V[i][j][k];
			}
		}
		return newV;
	}

	/**
	 * @param V
	 * @return
	 */
	public static double[][] convertArray3DToArray2D(double[][][] V) {
		int n = V.length;
		int m = V[0].length;
		int p = V[0][0].length;
		double[][] newV = new double[n * m][p];
		for (int i = 0; i < n; i++) {
			for (int j = 0; j < m; ++j) {
				System.arraycopy(V[i][j], 0, newV[i * m + j], 0, p);
			}
		}
		return newV;
	}

	public static float[] convertDoubleToFloatArray(double[] ds) {
		int n = ds.length;
		float[] fs = new float[n];
		for (int i = 0; i < n; ++i)
			fs[i] = (float) ds[i];
		return fs;
	}

	/**
	 * Make a copy of <i>src</i> in <i>dst</i>. The length of <i>dst</i> may be
	 * larger than that of <i>src</i>. Excess terms will be left.
	 * 
	 * @param dst
	 * @param src
	 * @return
	 */
	public static double[] copy(double[] dst, double[] src) {
		if (dst == null)
			dst = new double[src.length];
		System.arraycopy(src, 0, dst, 0, Math.min(dst.length, src.length));
		return dst;
	}

	/**
	 * Calculate the cross product of the two vectors <i>u</i> and <i>v</i>.
	 * Note that the only valid dimension is 3.
	 * 
	 * @param dst
	 *            double[3]
	 * @param u
	 *            double[3]
	 * @param v
	 *            double[3]
	 * @return dst
	 */
	public static double[] crossProduct(double[] dst, double[] u, double[] v) {
		if (u.length < 3 || v.length < 3) {
			throw new IllegalArgumentException("Vectors too short");
		}
		if (dst == null)
			dst = new double[3];
		double[] tmp = dst;
		if (dst == u || dst == v) {
			tmp = new double[3];
		}
		tmp[0] = u[1] * v[2] - u[2] * v[1];
		tmp[1] = u[2] * v[0] - u[0] * v[2];
		tmp[2] = u[0] * v[1] - u[1] * v[0];
		if (tmp != dst)
			System.arraycopy(tmp, 0, dst, 0, 3);
		return dst;
	}

	/**
	 * Calculate the determinate of the square matrix <i>m</i>. TODO: figure out
	 * how to generate the necessary permutations without having to recursively
	 * call submatrix()
	 * 
	 * @param m
	 * @return
	 */
	public static double determinant(double[] m) {
		double det = 0.0;
		int n = mysqrt(m.length);
		if (n > 4) {
			double[] subm = new double[(n - 1) * (n - 1)];
			for (int i = 0; i < n; ++i) {
				double tmp = m[i] * determinant(submatrix(subm, m, 0, i));
				det += ((i % 2) == 0) ? tmp : (-tmp);
			}
		} else
			return determinantOld(m);
		return det;
	}

	public static double determinant(double[][] m) {
		int n = m.length;
		if (n != m[0].length)
			throw new IllegalArgumentException("Must be square matrix");
		double[] lm = new double[n * n];
		for (int i = 0; i < m.length; ++i)
			System.arraycopy(m[i], 0, lm, n * i, n);
		return determinant(lm);
	}

	/**
	 * This is optimized for low dimensional determinants.
	 * 
	 * @param m
	 * @return
	 */
	private static double determinantOld(double[] m) {
		double det = 0.0;
		int n = mysqrt(m.length);
		switch (n) {
		// optimize following by parenthesizing
		case 4:
			det = m[3] * m[6] * m[9] * m[12] - m[2] * m[7] * m[9] * m[12]
					- m[3] * m[5] * m[10] * m[12] + m[1] * m[7] * m[10] * m[12]
					+ m[2] * m[5] * m[11] * m[12] - m[1] * m[6] * m[11] * m[12]
					- m[3] * m[6] * m[8] * m[13] + m[2] * m[7] * m[8] * m[13]
					+ m[3] * m[4] * m[10] * m[13] - m[0] * m[7] * m[10] * m[13]
					- m[2] * m[4] * m[11] * m[13] + m[0] * m[6] * m[11] * m[13]
					+ m[3] * m[5] * m[8] * m[14] - m[1] * m[7] * m[8] * m[14]
					- m[3] * m[4] * m[9] * m[14] + m[0] * m[7] * m[9] * m[14]
					+ m[1] * m[4] * m[11] * m[14] - m[0] * m[5] * m[11] * m[14]
					- m[2] * m[5] * m[8] * m[15] + m[1] * m[6] * m[8] * m[15]
					+ m[2] * m[4] * m[9] * m[15] - m[0] * m[6] * m[9] * m[15]
					- m[1] * m[4] * m[10] * m[15] + m[0] * m[5] * m[10] * m[15];
			break;
		case 3:
			det = -(m[2] * m[4] * m[6]) + m[1] * m[5] * m[6] + m[2] * m[3]
					* m[7] - m[0] * m[5] * m[7] - m[1] * m[3] * m[8] + m[0]
					* m[4] * m[8];
			break;
		case 2:
			det = m[0] * m[3] - m[2] * m[1];
			break;
		case 1:
			det = m[0];
			break;
		default:
			det = determinant(m);
			// raise exception
		}
		return det;
	}

	/**
	 * Calculate whether the two arrays contain exactly the same values. Returns
	 * true only if the {@link Rn#manhattanNorm(double[] ) "manhattan norm"} of
	 * the difference of the two vectors does not exceed the default
	 * {@link Rn#TOLERANCE tolerance}.
	 * 
	 * @param u
	 *            double[n]
	 * @param v
	 *            double[n]
	 * @return
	 * @see #equals(double[], double[], double) equals()
	 */
	public static boolean equals(double[] u, double[] v) {
		return equals(u, v, 0);
	}

	/**
	 * Calculate whether the two arrays contain the same values. Returns true
	 * only if the {@link Rn#manhattanNorm(double[] ) "manhattan norm"} of the
	 * difference of the two vectors does not exceed the provided <i>tol</i>.
	 * 
	 * @param u
	 *            double[n]
	 * @param v
	 *            double[n]
	 * @param tol
	 * @return
	 */
	public static boolean equals(double[] u, double[] v, double tol) {
		int n = u.length;
		if (v.length < u.length)
			n = v.length;
		for (int i = 0; i < n; ++i) {
			double d = u[i] - v[i];
			if (d > tol || d < -tol)
				return false;
		}
		return true;
	}

	/**
	 * Calculate the euclidean angle between the vectors <i>u</i> and <i>v</i>.
	 * 
	 * @param u
	 * @param v
	 * @return
	 */
	public static double euclideanAngle(double[] u, double[] v) {
		if (u.length != v.length) {
			throw new IllegalArgumentException("Vectors must have same length");
		}
		double uu = innerProduct(u, u);
		double vv = innerProduct(v, v);
		double uv = innerProduct(u, v);
		if (uu == 0 || vv == 0) // TODO: check for <epsilon rather than ==0
			return (Double.MAX_VALUE);
		double f = uv / Math.sqrt(Math.abs(uu * vv));
		if (f > 1.0)
			f = 1.0;
		if (f < -1.0)
			f = -1.0;
		double d = Math.acos(f);
		return d;
	}

	/**
	 * Calculates and returns the euclidean distance between the two points
	 * <i>u</i> and <i>v</i>.
	 * 
	 * @param u
	 * @param v
	 * @return
	 */
	public static double euclideanDistance(double[] u, double[] v) {
		return Math.sqrt(euclideanDistanceSquared(u, v));
	}

	/**
	 * calculates and returns the square of the euclidean distance between the
	 * two points <i>u</i> and <i>v</i>.
	 * 
	 * @param u
	 *            double[n]
	 * @param v
	 *            double[n]
	 * @return
	 */
	public static double euclideanDistanceSquared(double[] u, double[] v) {
		double[] tmp = new double[u.length];
		subtract(tmp, u, v);
		return euclideanNormSquared(tmp);
	}

	/**
	 * Calculates and returns the euclidean norm of the input vector.
	 * 
	 * @param vec
	 *            double[n]
	 * @return
	 * @see #innerProduct(double[], double[]) innerProduct()
	 */
	public static double euclideanNorm(double[] vec) {
		return Math.sqrt(innerProduct(vec, vec));
	}

	/**
	 * Calculates and returns the square of the euclidean norm of the input
	 * vector.
	 * 
	 * @param vec
	 *            double[n]
	 * @return
	 */
	public static double euclideanNormSquared(double[] vec) {
		return innerProduct(vec, vec);
	}

	/**
	 * Extract a rectangular submatrix of the input matrix.
	 * 
	 * @param subm
	 *            double[b-t][r-l]
	 * @param src
	 *            double[n][n]
	 * @param l
	 *            begin with the lth column
	 * @param r
	 *            and include the rth column
	 * @param t
	 *            begin with the t-th row
	 * @param b
	 *            and include the b-th row
	 * @return subm
	 */
	public static double[] extractSubmatrix(double[] subm, double[] src, int l,
			int r, int t, int b) {
		// TODO fill this method in!
		// assert dim checks: (b-t) == (r-l)
		if (r - l != b - t) {
			throw new IllegalArgumentException("(b-t) must equal (r-l)");
		}
		int n = mysqrt(src.length);
		int submsize = (b - t + 1) * (r - l + 1);
		int count = 0;
		if (subm.length != submsize)
			subm = new double[submsize];
		for (int i = t; i <= b; ++i) {
			for (int j = l; j <= r; ++j) {
				subm[count++] = src[i * n + j];
			}
		}
		return subm;
	}

	/**
	 * Create and return an identity matrix of dimension <i>dim</i>.
	 * 
	 * @param dim
	 * @return
	 */
	public static double[] identityMatrix(final int dim) {
		final double[] m = new double[dim * dim];
		for (int i = 0, k = 0, doffs = dim + 1; i < dim; i++, k += doffs)
			m[k] = 1.0;
		return m;
	}

	public static double[] diagonalMatrix(double[] dst, double[] entries) {
		int n = entries.length;
		if (dst == null)
			dst = identityMatrix(n);

		for (int i = 0; i < n; ++i)
			dst[n * i + i] = entries[i];
		return dst;
	}

	/**
	 * Calculate and return the euclidean inner product of the two vectors. If
	 * they are of different lengths only evaluates the inner product on the
	 * lesser of the two lengths.
	 * 
	 * @param u
	 * @param v
	 * @return
	 */
	public static double innerProduct(double[] u, double[] v) {
		// assert dim check
		if (u.length != v.length) {
			// allow inner product for homogenious and dehomogenized vectors
			if (Math.abs(u.length - v.length) != 1)
				throw new IllegalArgumentException(
						"Vectors must have same length");
		}
		double norm = 0.0;
		int n = u.length < v.length ? u.length : v.length;
		for (int i = 0; i < n; ++i) {
			norm += u[i] * v[i];
		}
		return norm;
	}

	/**
	 * Calculate and return at most n terms of the inner product of the two
	 * vectors. Useful when using homogenous coordinates but one wants to ignore
	 * the final coordinate.
	 * 
	 * @param u
	 * @param v
	 * @return
	 */
	public static double innerProduct(double[] u, double[] v, int n) {
		// assert dim check
		if (u.length < n || v.length < n) {
			throw new IllegalArgumentException("Vectors not long enough");
		}
		double norm = 0.0;
		int m = u.length < n ? u.length : n;
		for (int i = 0; i < m; ++i) {
			norm += u[i] * v[i];
		}
		return norm;
	}

	/**
	 * Use Gaussian pivoting to calculate the inverse matrix of the input
	 * matrix. This method is safe when <i>minvIn == m</i>.
	 * 
	 * @param minv
	 *            double[n][n]
	 * @param m
	 *            double[n][n]
	 * @return minv
	 */
	public static double[] inverse(double[] minvIn, double[] m) {
		int n = mysqrt(m.length);
		int i, j, k;
		double x, f;
		double[] t;
		int largest;

		t = new double[m.length];
		System.arraycopy(m, 0, t, 0, m.length);

		double[] minv;
		if (minvIn == null)
			minv = new double[m.length];
		else
			minv = minvIn;
		setIdentityMatrix(minv);

		for (i = 0; i < n; i++) {
			largest = i;
			double largesq = t[n * i + i] * t[n * i + i];
			// find the largest entry in the ith column below the ith row
			for (j = i + 1; j < n; j++)
				if ((x = t[j * n + i] * t[j * n + i]) > largesq) {
					largest = j;
					largesq = x;
				}

			for (k = 0; k < n; ++k) { // swap the ith row with the row with
										// largest entry in the ith column
				x = t[i * n + k];
				t[i * n + k] = t[largest * n + k];
				t[largest * n + k] = x;
			}
			for (k = 0; k < n; ++k) { // do the same in the inverse matrix
				x = minv[i * n + k];
				minv[i * n + k] = minv[largest * n + k];
				minv[largest * n + k] = x;
			}
			// now for each remaining row, subtract off a multiple of the ith
			// row so that the
			// entry in the ith column of that row becomes 0. Previous entries
			// are already 0
			for (j = i + 1; j < n; j++) {
				f = t[j * n + i] / t[i * n + i];
				for (k = 0; k < n; ++k)
					t[j * n + k] -= f * t[i * n + k];
				for (k = 0; k < n; ++k)
					minv[j * n + k] -= f * minv[i * n + k];
			}
		}

		for (i = 0; i < n; i++) {
			f = t[i * n + i];
			if (f == 0.0) {
				// throw new ArithmeticException("Divide by zero");
				// hack
				f = 10E-16;
				LoggingSystem.getLogger(Rn.class).log(Level.WARNING,
						"Divide by zero, returning identity matrix");
				setIdentityMatrix(minv);
				return minv;
			}
			for (f = 1.0 / f, j = 0; j < n; j++) {
				t[i * n + j] *= f;
				minv[i * n + j] *= f;
			}
		}
		for (i = n - 1; i >= 0; i--)
			for (j = i - 1; j >= 0; j--) {
				f = t[j * n + i];
				for (k = 0; k < n; ++k)
					t[j * n + k] -= f * t[i * n + k];
				for (k = 0; k < n; ++k)
					minv[j * n + k] -= f * minv[i * n + k];
			}
		return minv;

	}

	private static double[] inverseSlow(double[] dst, double[] src) {
		// assert dim checks
		if (dst == null)
			dst = new double[src.length];
		double[] tmp = new double[dst.length];
		double det = determinant(src);
		if (det == 0.0) {
			throw new IllegalArgumentException(
					"Input matrix has determinant zero");
		}
		// adjoint(tmp,src);
		times(dst, 1.0 / det, transpose(tmp, adjoint(dst, src)));
		return dst;
	}

	/**
	 * Calculates whether the input matrix in within <i>tol</i> of the identity
	 * matrix.
	 * 
	 * @param mat
	 * @param tol
	 * @return
	 */
	public static boolean isIdentityMatrix(double[] mat, double tol) {
		// assert dim check
		int n = mysqrt(mat.length);
		double[] tmp = new double[mat.length];
		double[] idd = identityMatrices[n];
		for (int i = 0; i < tmp.length; ++i)
			if (Math.abs(mat[i] - idd[i]) > tol)
				return false;
		return true;
	}

	/**
	 * Calculates whether the input matrix is within <i>tol</i> of being a
	 * "special" matrix. To be exact, returns true if the absolute value of the
	 * determinant is within <i>tol</i> of 1.0.
	 * 
	 * @param mat
	 * @param tol
	 * @return
	 */
	public static boolean isSpecialMatrix(double[] mat, double tol) {
		// assert dim check
		double d = determinant(mat);
		return (Math.abs(Math.abs(d) - 1) < tol);
	}

	/**
	 * Calculate the linear combination <i>dst = a*aVec + b*bVec</i>.
	 * 
	 * @param dst
	 *            double[n]
	 * @param a
	 * @param aVec
	 *            double[n]
	 * @param b
	 * @param bVec
	 *            double[n]
	 * @return
	 */
	public static double[] linearCombination(double[] dst, double a,
			double[] aVec, double b, double[] bVec) {
		if (aVec.length != bVec.length) {
			throw new IllegalArgumentException("Vectors must be same length");
		}
		if (dst == null)
			dst = new double[aVec.length];
		double[] tmp = new double[dst.length];
		return add(dst, times(tmp, a, aVec), times(dst, b, bVec));
	}

	/**
	 * Calculate the "manhattan norm" of the input vector. This is the sum of
	 * the absolute values of the entries.
	 * 
	 * @param vec
	 * @return
	 */
	public static double manhattanNorm(double[] vec) {
		double sum = 0;
		for (int i = 0; i < vec.length; ++i)
			sum = sum + Math.abs(vec[i]);
		return sum;
	}

	/**
	 * Calculate the "manhattan norm" distance between the two input vectors.
	 * 
	 * @param u
	 * @param v
	 * @return
	 */
	public static double manhattanNormDistance(double[] u, double[] v) {
		double[] tmp = new double[u.length];
		subtract(tmp, u, v);
		return manhattanNorm(tmp);
	}

	/**
	 * Multiply the input vector <i>src</i> by the matrix <i>m</i> and put the
	 * result into <i>dst</i>. This method works correctly when the vectors are
	 * given using homogeneous <b>or</b> dehomogenous coordinates (but this must
	 * be consistent for both <i>src</i> and <i>dst</i>).
	 * 
	 * @param dst
	 *            double[n] (may be null)
	 * @param m
	 *            double[n][n]
	 * @param src
	 *            double[n]
	 * @return dst
	 */
	public static double[] matrixTimesVector(double[] dst, double[] m,
			double[] src) {
		// assert dim check
		double[] out;
		boolean rewrite = false;
		if (dst == m || dst == src) {
			out = new double[dst.length];
			rewrite = true;
		} else if (dst == null)
			out = new double[src.length];
		else
			out = dst;

		_matrixTimesVectorSafe(out, m, src);

		if (rewrite) {
			System.arraycopy(out, 0, dst, 0, dst.length);
			return dst;
		}
		return out;
	}

	/**
	 * This behaves correctly even if <i>src=dst</i>.
	 * 
	 * @param dst
	 * @param m
	 * @param src
	 */
	private static void _matrixTimesVectorSafe(double[] dst, double[] m,
			double[] src) {
		int sl = src.length;
		int ml = mysqrt(m.length);
		boolean dehomog = false;
		if (ml == sl + 1)
			dehomog = true;
		if (sl + 1 < ml || sl > ml) {
			throw new IllegalArgumentException(
					"Invalid dimension in _matrixTimesVectorSafe");
		}
		double[] out;
		if (dehomog) {
			out = new double[ml];
		} else
			out = dst;
		for (int i = 0; i < ml; ++i) {
			out[i] = 0;
			for (int j = 0; j < ml; ++j)
				if (dehomog && j == ml - 1)
					out[i] += m[i * ml + j]; // src[last] = 1.0
				else
					out[i] += m[i * ml + j] * src[j];
		}
		if (dehomog) {
			Pn.dehomogenize(dst, out);
		}
	}

	/**
	 * A vectorized version of
	 * {@link #matrixTimesVector(double[], double[], double[])}.
	 * 
	 * @param dst
	 *            double[m][n] (may be null)
	 * @param m
	 *            double[n][n]
	 * @param src
	 *            double[m][n]
	 * @return dst
	 */
	public static double[][] matrixTimesVector(double[][] dst, double[] m,
			double[][] src) {
		int n = mysqrt(m.length);
		double[][] out;
		boolean rewrite = false;
		if (dst == null || dst == src) {
			out = new double[src.length][src[0].length];
			if (dst == src)
				rewrite = true;
		} else {
			out = dst;
		}

		int nv = src.length;
		for (int k = 0; k < nv; ++k)
			_matrixTimesVectorSafe(out[k], m, src[k]);

		if (rewrite) {
			System.arraycopy(out, 0, dst, 0, dst.length);
			return dst;
		}
		return out;
	}

	/**
	 * Like {@link #matrixToString(double[])}, but formatted for direct
	 * insertion as java source code
	 * 
	 * @param v
	 * @return
	 */
	public static String matrixToJavaString(double[] v) {
		StringBuffer sb = new StringBuffer();
		sb.append("{");
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				sb.append(String.format("%g", new Object[] { v[4 * i + j] }));
				if (i != 3 || j != 3)
					sb.append(",");
				sb.append(j == 3 ? "\n" : "\t");
			}
		}
		sb.append("};");
		return sb.toString();
	}

	/**
	 * Print the square matrix <i>m</i> into a string using default format from
	 * {@link String#format(java.lang.String, java.lang.Object[])} and return
	 * the string.
	 * 
	 * @param m
	 * @return
	 */
	public static String matrixToString(double[] m) {
		return matrixToString(m, "%g");
	}

	/**
	 * Print the square matrix <i>m</i> into a string using <i>formatString</i>
	 * and return the string.
	 * 
	 * @param m
	 * @param formatString
	 * @return
	 * @see String#format(java.lang.String, java.lang.Object[])
	 */
	public static String matrixToString(double[] m, String formatString) {
		StringBuffer sb = new StringBuffer();
		int n = mysqrt(m.length);
		for (int i = 0; i < n; i++) {
			for (int j = 0; j < n; j++) {
				// sb.append(df.format(v[4*i+j])).append(j == 3 ? "\n":"\t");
				sb.append(String.format(formatString, new Object[] { m[n * i
						+ j] }));
				sb.append(j == (n - 1) ? "\n" : "\t");
			}
		}
		return sb.toString();
	}

	/**
	 * Calculate the maximum of two vectors, coordinate-wise.
	 * 
	 * @param dst
	 * @param src1
	 * @param src2
	 * @return
	 */
	public static double[] max(double[] dst, double[] src1, double[] src2) {
		int n = Math.min(src1.length, src2.length);
		if (dst == null)
			dst = new double[n];
		if (dst.length != n) {
			throw new IllegalArgumentException("Invalid target vector length");
		}
		int lim = Math.min(dst.length, n);
		for (int i = 0; i < lim; ++i)
			dst[i] = Math.max(src1[i], src2[i]);
		return dst;
	}

	/**
	 * Calculate the "max norm" of a vector. That is the maximum of the absolute
	 * values of the entries.
	 * 
	 * @param vec
	 * @return
	 */
	public static double maxNorm(double[] vec) {
		double max = 0;
		for (int i = 0; i < vec.length; ++i)
			max = Math.max(max, Math.abs(vec[i]));
		return max;
	}

	/**
	 * Calculate the distance between two points based on the
	 * {@link #max(double[], double[], double[]) "max norm"}.
	 * 
	 * @param u
	 * @param v
	 * @return
	 */
	public static double maxNormDistance(double[] u, double[] v) {
		double[] tmp = new double[u.length];
		subtract(tmp, u, v);
		return maxNorm(tmp);
	}

	/**
	 * Calculate the minimum of two vectors, coordinate-wise.
	 * 
	 * @param dst
	 * @param src1
	 * @param src2
	 * @return
	 */
	public static double[] min(double[] dst, double[] src1, double[] src2) {
		int n = Math.min(src1.length, src2.length);
		if (dst == null)
			dst = new double[n];
		if (dst.length != n) {
			throw new IllegalArgumentException("Invalid target vector length");
		}
		for (int i = 0; i < n; ++i)
			dst[i] = Math.min(src1[i], src2[i]);
		return dst;
	}

	/**
	 * <i>dst[i] = -src[i]</i>
	 * 
	 * @param dst
	 * @param src
	 * @return
	 */
	public static double[] negate(double[] dst, double[] src) {
		if (dst == null)
			dst = new double[src.length];
		if (dst.length != src.length) {
			throw new IllegalArgumentException("Vectors must have same length");
		}
		int n = Math.min(dst.length, src.length);
		for (int i = 0; i < n; ++i)
			dst[i] = -src[i];
		return dst;
	}

	/**
	 * Normalize <i>src</i> using the {@link Rn#euclideanNorm(double[])
	 * euclidean norm}.
	 * 
	 * @param dst
	 * @param src
	 * @return
	 */
	public static double[] normalize(double[] dst, double[] src) {
		return setEuclideanNorm(dst, 1.0, src);
	}

	/**
	 * @param ds
	 */
	public static double[][] normalize(double[][] dst, double[][] src) {
		if (dst == null)
			dst = new double[src.length][src[0].length];
		if (dst.length != src.length || dst[0].length != src[0].length) {
			throw new IllegalArgumentException("Vectors must have same length");
		}
		int n = Math.min(dst.length, src.length);
		for (int i = 0; i < n; ++i)
			Rn.normalize(dst[i], src[i]);
		return dst;
	}

	/**
	 * @param plane
	 * @param ds
	 * @param ds2
	 */
	public static double[] planeParallelToPassingThrough(double[] plane,
			double[] ds, double[] ds2) {
		if (plane == null)
			plane = new double[4];
		System.arraycopy(ds, 0, plane, 0, 3);
		plane[3] = -innerProduct(plane, ds2, 3);
		return plane;
	}

	/**
	 * @param q
	 * @param s
	 * @param m
	 * @return
	 * @see "Graphics Gems IV", p. 207 for details
	 */
	public static double[] polarDecompose(double[] q, double[] s, double[] m) {
		int old = 0, nw = 1;
		double[][] qq = new double[2][];
		double[] qit = (double[]) m.clone();
		qq[old] = (double[]) m.clone();
		qq[nw] = (double[]) m.clone();
		double tol = 10E-12;
		int count = 0;

		/*
		 * TODO: there is an acceleration technique which should be used here in
		 * Graphics Gems IV, p 216
		 */
		do {
			transpose(qit, inverse(qit, qq[old]));
			add(qq[nw], qq[old], qit);
			times(qq[nw], 0.5, qq[nw]);
			nw = 1 - nw;
			old = 1 - old;
			count++;
		} while (count < 20 && !(equals(qq[nw], qq[old], tol)));
		System.arraycopy(qq[nw], 0, q, 0, m.length);
		transpose(qit, qq[nw]);
		times(s, qit, m);
		return m;
	}

	/**
	 * Project orthogonally <i>src</i> onto <i>fixed</i>.
	 * 
	 * @param dst
	 * @param src
	 * @param fixed
	 * @return
	 */
	public static double[] projectOnto(double[] dst, double[] src,
			double[] fixed) {
		if (dst == null)
			dst = new double[src.length];
		// double[] nfixed = Rn.normalize(null, fixed);
		double d = Rn.innerProduct(fixed, fixed);
		double f = Rn.innerProduct(fixed, src);
		// System.err.println("inner product = "+f);
		Rn.times(dst, f / d, fixed);
		return dst;
	}

	/**
	 * Project <i>src</i> onto the orthogonal complement of <i>fixed</i>.
	 * 
	 * @param dst
	 * @param src
	 * @param fixed
	 * @return
	 */
	public static double[] projectOntoComplement(double[] dst, double[] src,
			double[] fixed) {
		return Rn.subtract(dst, src, projectOnto(null, src, fixed));
	}

	/**
	 * Construct and return a diagonal matrix with the entries given by the
	 * vector <i>diag</i>. The number of entries of <i>diag</i> cannot be more
	 * than the dimension of the square matrix <i>dst</i>.
	 * 
	 * @param dst
	 * @param diag
	 * @return
	 */
	public static double[] setDiagonalMatrix(double[] dst, double[] diag) {
		int n2 = diag.length;
		if (dst == null)
			dst = new double[n2 * n2];
		int n1 = mysqrt(dst.length);
		if (n1 < n2) {
			throw new IllegalArgumentException("Incompatible lengths");
		}
		setIdentityMatrix(dst);
		int n = Math.min(n1, n2);
		for (int i = 0; i < n; ++i)
			dst[n1 * i + i] = diag[i];
		return dst;
	}

	/**
	 * Scale the vector to have the given length.
	 * 
	 * @param dst
	 * @param length
	 * @param src
	 * @return
	 */
	public static double[] setEuclideanNorm(double[] dst, double length,
			double[] src) {
		if (dst == null)
			dst = new double[src.length];
		if (dst.length != src.length) {
			throw new IllegalArgumentException("Incompatible lengths");
		}
		double norm = euclideanNorm(src);
		if (norm == 0) {
			System.arraycopy(src, 0, dst, 0, Math.min(src.length, dst.length));
			return dst;
		}
		return times(dst, length / norm, src);
	}

	/**
	 * Set the matrix to be the identity matrix.
	 * 
	 * @param mat
	 * @return
	 */
	public static double[] setIdentityMatrix(double[] mat) {
		int n = mysqrt(mat.length), noffs = n + 1;
		Arrays.fill(mat, 0);
		for (int i = 0, k = 0; i < n; i++, k += noffs)
			mat[k] = 1.0;
		return mat;
	}

	public static float[] setIdentityMatrix(float[] mat) {
		int n = mysqrt(mat.length), noffs = n + 1;
		Arrays.fill(mat, 0);
		for (int i = 0, k = 0; i < n; i++, k += noffs)
			mat[k] = 1.0f;
		return mat;
	}

	/**
	 * Set the destination to vector to have constant value <i>val</i>.
	 * 
	 * @param dst
	 * @param val
	 * @return
	 */
	public static double[] setToValue(double[] dst, double val) {
		Arrays.fill(dst, val);
		return dst;
	}

	/**
	 * Initialize the 2-vector <i>dst</i>.
	 * 
	 * @param dst
	 * @param x
	 * @param y
	 * @return
	 */
	public static double[] setToValue(double[] dst, double x, double y) {
		if (dst == null)
			dst = new double[2];
		if (dst.length != 2) {
			throw new IllegalArgumentException("Incompatible length");
		}
		dst[0] = x;
		dst[1] = y;
		return dst;
	}

	/**
	 * Initialize the 3-vector <i>dst</i>.
	 * 
	 * @param dst
	 * @param x
	 * @param y
	 * @param z
	 * @return
	 */
	public static double[] setToValue(double[] dst, double x, double y, double z) {
		if (dst == null)
			dst = new double[3];
		if (dst.length != 3) {
			throw new IllegalArgumentException("Incompatible length");
		}
		dst[0] = x;
		dst[1] = y;
		dst[2] = z;
		return dst;
	}

	/**
	 * Initialize the 4-vector <i>dst</i>.
	 * 
	 * @param dst
	 * @param x
	 * @param y
	 * @param z
	 * @param w
	 * @return
	 */
	public static double[] setToValue(double[] dst, double x, double y,
			double z, double w) {
		if (dst == null)
			dst = new double[4];
		if (dst.length != 4) {
			throw new IllegalArgumentException("Incompatible length");
		}
		dst[0] = x;
		dst[1] = y;
		dst[2] = z;
		dst[3] = w;
		return dst;
	}

	/**
	 * Returns the square root of the <code>sq</code> parameter. Will return
	 * very fast for square roots of 0 to 10.
	 * 
	 * @param sq
	 * @return
	 */
	public static int mysqrt(int sq) {
		switch (sq) {
		case 16:
			return 4;
		case 9:
			return 3;
		case 4:
			return 2;
		case 1:
			return 1;
		case 25:
			return 5;
		case 36:
			return 6;
		case 49:
			return 7;
		case 64:
			return 8;
		case 81:
			return 9;
		case 100:
			return 10;
		case 0:
			return 0;
		default:
			if (sq < 0)
				throw new IllegalArgumentException(String.valueOf(sq));
			return (int) Math.sqrt(sq);
		}
	}

	/**
	 * Extract the submatrix gotten by deleting the given row and column.
	 * 
	 * @param subm
	 *            double[n-1 * n-1]
	 * @param m
	 *            double[n*n]
	 * @param row
	 * @param column
	 * @return
	 */
	public static double[] submatrix(double[] subm, double[] m, int row,
			int column) {
		int n = mysqrt(m.length);
		if (subm == null)
			subm = new double[(n - 1) * (n - 1)];
		if (subm.length != (n - 1) * (n - 1)) {
			throw new IllegalArgumentException(
					"Invalid dimension for submatrix");
		}
		for (int i = 0, cnt = 0; i < n; ++i) {
			if (i == row)
				continue;
			for (int j = 0; j < n; ++j) {
				if (j != column)
					subm[cnt++] = m[i * n + j];
			}
		}
		return subm;
	}

	/**
	 * <i>dst[i] = src1[i] - src2[i]</i>
	 * 
	 * @param dst
	 * @param src1
	 * @param src2
	 * @return
	 */
	public static double[] subtract(double[] dst, double[] src1, double[] src2) {
		int n = Math.min(src1.length, src2.length);
		if (dst == null)
			dst = new double[n];
		if (dst.length > n) {
			throw new IllegalArgumentException("Invalid dimension for target");
		}
		int x = 0;
		for (int i = 0; i < dst.length; ++i)
			dst[i] = src1[i] - src2[i];
		return dst;
	}

	public static double[][] subtract(double[][] dst, double[][] src1, double[][] src2)	{
		if (dst == null)
			dst = new double[src1.length][src1[0].length];
		if (dst.length != src1.length) {
			throw new IllegalArgumentException("Vectors must be same length");
		}
		int n = src1.length;
		for (int i = 0; i < n; ++i) {
			subtract(dst[i], src1[i], src2[i]);
		}
		return dst;

	}
	/**
	 * Swap the contents of the two vectors.
	 * 
	 * @param u
	 * @param v
	 */
	public static void swap(double[] u, double[] v) {
		if (v.length != v.length) {
			throw new IllegalArgumentException("Inputs must be same length");
		}
		double tmp;
		int n = u.length;

		for (int i = 0; i < n; ++i) {
			tmp = u[i];
			u[i] = v[i];
			v[i] = tmp;
		}
	}

	/**
	 * Multiply the vector <src> by the scalar value <i>factor</i> and put
	 * result into <i>dst</i>.
	 * 
	 * @param dst
	 * @param factor
	 * @param src
	 * @return
	 */
	public static double[] times(double[] dst, double factor, double[] src) {
		if (dst == null)
			dst = new double[src.length];
		if (dst.length != src.length) {
			throw new IllegalArgumentException("Vectors must be same length");
		}
		int n = dst.length;
		for (int i = 0; i < n; ++i)
			dst[i] = factor * src[i];
		return dst;
	}

	/**
	 * Multiply the square matrices according to the equation <i>dst = src1 *
	 * src2 </i>. This operation is overwrite-safe; that is, it gives correct
	 * results when <i>src1=dst</i> or <i>src2=dst</i>.
	 * 
	 * @param dst
	 *            double[n*n] may be null
	 * @param src1
	 *            double[n*n]
	 * @param src2
	 *            double[n*n]
	 * @return
	 */
	public static double[] times(double[] dst, double[] src1, double[] src2) {
		if (src1.length != src2.length) {
			throw new IllegalArgumentException("Matrices must be same size");
		}
		int n = mysqrt(src1.length);
		double[] out;
		boolean rewrite = false;
		if (dst == src1 || dst == src2 || dst == null) {
			out = new double[src1.length];
			if (dst != null)
				rewrite = true;
		} else {
			out = dst;
		}
		if (out.length != src1.length) {
			throw new IllegalArgumentException("Matrices must be same size");
		}

		for (int i = 0; i < n; ++i) {
			for (int j = 0; j < n; ++j) {
				out[i * n + j] = 0.0;
				for (int k = 0; k < n; ++k) {
					// the (i,j)th position is the inner product of the ith row
					// and
					// the jth column of the two factors
					out[i * n + j] += src1[i * n + k] * src2[k * n + j];
				}
			}
		}
		if (dst == null)
			return out;
		if (rewrite)
			System.arraycopy(out, 0, dst, 0, dst.length);
		return out;
	}

	/**
	 * A vectorized version of {@link #times(double[], double, double[])};
	 * 
	 * @param dst
	 * @param factor
	 * @param src
	 * @return
	 */
	public static double[][] times(double[][] dst, double factor, double[][] src) {
		if (dst == null)
			dst = new double[src.length][src[0].length];
		if (dst.length != src.length) {
			throw new IllegalArgumentException("Vectors must be same length");
		}
		int n = src.length;
		for (int i = 0; i < n; ++i) {
			times(dst[i], factor, src[i]);
		}
		return dst;
	}

	/**
	 * Print the array <i>v</i> into a string using default format from
	 * {@link String#format(java.lang.String, java.lang.Object[])} and return
	 * the string.
	 * 
	 * @param v
	 * @return
	 */
	public static String toString(double[] v) {
		return toString(v, "%g");
	}

	/**
	 * Print the array <i>v</i> into a string using <i>formatString</i> and
	 * return the string.
	 * 
	 * @param v
	 * @return
	 * @see String#format(java.lang.String, java.lang.Object[])
	 */
	public static String toString(double[] v, String formatString) {
		int n = v.length;
		StringBuffer strb = new StringBuffer();
		for (int i = 0; i < n; ++i) {
			strb.append(String.format(formatString, new Object[] { v[i] }));
			strb.append("\t");
		}
		return strb.toString();
	}

	/**
	 * A vectorized version of {@link #toString(double[])}.
	 * 
	 * @param v
	 * @return
	 */
	public static String toString(double[][] v) {
		return toString(v, -1);
	}

	public static String toString(double[][] v, int n) {
		if (n < 0) n = v.length;
		StringBuffer strb = new StringBuffer();
		for (int i = 0; i < n; ++i) {
			strb.append(toString(v[i]) + "\t");
			strb.append("\n");
		}
		return (new String(strb));
	}

	public static String toString(double[][][] v) {
		int n = v.length, m = v[0].length;
		StringBuffer strb = new StringBuffer();
		for (int i = 0; i < n; ++i) {
			for (int j = 0; j < m; ++j) {
				strb.append(toString(v[i][j]) + "\t");
				strb.append("\n");
			}
			strb.append("\n");
		}
		return (new String(strb));
	}

	public static String toString(float[] v) {
		double[] cp = new double[v.length];
		for (int i = 0; i < v.length; ++i)
			cp[i] = v[i];
		return toString(cp);
	}

	/**
	 * Calculate the trace of the given square matrix.
	 * 
	 * @param m
	 * @return
	 */
	public static double trace(double[] m) {
		int n = mysqrt(m.length);
		double t = 0;
		for (int i = 0; i < n; ++i)
			t += m[i * n + i];
		return t;
	}

	/**
	 * Transpose the given square matrix <i>src</i> into <i>dst</i>. This
	 * operation is overwrite-safe; that is, it gives correct results when
	 * <i>src=dst</i>.
	 * 
	 * @param dst
	 * @param src
	 * @return
	 */
	public static double[] transpose(double[] dst, double[] src) {
		int n = mysqrt(src.length);
		// assert dim checks
		double[] out;
		boolean rewrite = false;
		if (dst == null)
			dst = new double[src.length];
		if (dst.length != src.length) {
			throw new IllegalArgumentException("Matrices must be same size");
		}
		if (dst == src) {
			out = new double[dst.length];
			rewrite = true;
		} else {
			out = dst;
		}
		int i, j;
		for (i = 0; i < n; ++i) {
			for (j = 0; j < n; ++j) {
				out[i * n + j] = src[j * n + i];
			}
		}
		if (rewrite)
			System.arraycopy(out, 0, dst, 0, dst.length);
		return dst;
	}

	/**
	 * Transpose the given 4x4 matrix <i>src </i> into <i>dst </i>. <br>
	 * converts from <code>double</code> to <code>float</code>. This is useful
	 * for hooking up to GL libraries, for example.
	 * 
	 * @param dst
	 * @param src
	 * @return
	 */
	public static float[] transposeD2F(float[] dst, double[] src) {
		if (dst == null)
			dst = new float[16];
		int i, j;
		for (i = 0; i < 4; ++i) {
			for (j = 0; j < 4; ++j) {
				dst[i * 4 + j] = (float) src[j * 4 + i];
			}
		}
		return dst;
	}

	/**
	 * Transpose the given 4x4 matrix <i>src </i> into <i>dst </i>. <br>
	 * converts from <code>float</code> to <code>double</code>.
	 * 
	 * @param dst
	 * @param src
	 * @return
	 */
	public static double[] transposeF2D(double[] dst, float[] src) {
		if (dst == null)
			dst = new double[16];
		int i, j;
		for (i = 0; i < 4; ++i) {
			for (j = 0; j < 4; ++j) {
				dst[i * 4 + j] = (float) src[j * 4 + i];
			}
		}
		return dst;
	}

	public static double[] bilinearInterpolation(double[] ds, double u,
			double v, double[] vb, double[] vt, double[] cb, double[] ct) {
		if (ds == null)
			ds = new double[vb.length];
		double[] vv = linearCombination(null, 1 - u, vb, u, vt);
		double[] cc = linearCombination(null, 1 - u, cb, u, ct);
		linearCombination(ds, 1 - v, vv, v, cc);
		return ds;
	}

	public static double[] bezierCombination(double[] dst, double t,
			double[] v0, double[] t0, double[] t1, double[] v1) {
		double tmp1 = (1 - t);
		double tmp2 = tmp1 * tmp1;
		double c0 = tmp2 * tmp1;
		double c1 = 3 * tmp2 * t;
		double c2 = 3 * tmp1 * t * t;
		double c3 = t * t * t;
		dst = Rn.add(dst,
				Rn.add(null, Rn.times(null, c0, v0), Rn.times(null, c1, t0)),
				Rn.add(null, Rn.times(null, c2, t1), Rn.times(null, c3, v1)));
		return dst;

	}

	public static boolean isNan(double[] ds) {
		int n = ds.length;
		for (int i = 0; i < n; ++i) {
			if (Double.isNaN(ds[i]))
				return true;
		}
		return false;
	}

	public static double[] setToLength(double[] p1, double[] p12, double rad) {
		return times(p1, rad / euclideanNorm(p12), p12);
	}

	/**
	 * The array <i>partial</i> contains <i>m n-vectors</i> which are assumed to
	 * be linearly independent. The destination array <i>dst</i> is calculated
	 * so that it extends <i>partial</i> with <i>n-m</i> n-vectors, each of
	 * which is orthogonal to all elements of <i>partial</i> as well as to each
	 * other. That is, if <i>partial</i> is an orthogonal sub-basis, then
	 * <i>dst</i> will also be.
	 * 
	 * @param dst
	 *            double[n][n]
	 * @param partial
	 *            double[m][n]
	 * @return
	 */
	public static double[][] completeBasis(double[][] dst, double[][] partial) {
		int dim = partial[0].length;
		int size = partial.length;
		if (dst == null || dst.length != dim)
			dst = new double[dim][dim];
		double[] inline = new double[dim * dim];
		for (int i = 0; i < size; ++i) {
			System.arraycopy(partial[i], 0, inline, i * dim, dim);
		}
		for (int i = size; i < dim; ++i) {
			for (int j = 0; j < dim; ++j) {
				inline[i * dim + j] = Math.random();
			}
		}
		for (int i = size; i < dim; ++i) {
			double[] newrow = dst[i];
			for (int j = 0; j < dim; ++j) {
				newrow[j] = (((i + j) % 2 == 0) ? 1 : -1)
						* determinant(submatrix(null, inline, i, j));
			}
			System.arraycopy(newrow, 0, inline, i * dim, dim);
		}
		for (int i = 0; i < dim; ++i) {
			System.arraycopy(inline, i * dim, dst[i], 0, dim);
		}
		// for (int i = size; i<dim; ++i) {
		// for (int j = 0; j<dim; ++j) {
		// double d = innerProduct(dst[i], dst[j]);
		// System.err.println(i+"."+j+"="+d);
		// }
		// }
		return dst;
	}

	/**
	 * Calculate a permutation matrix which sends e(i) to e(perm[i])
	 * 
	 * @param dst
	 * @param perm
	 * @return
	 */
	public static double[] permutationMatrix(double[] dst, int[] perm) {
		int n = perm.length;
		if (dst == null)
			dst = new double[n * n];
		for (int i = 0; i < n; ++i) {
			dst[i * n + perm[i]] = 1;
		}
		Rn.transpose(dst, dst);
		return dst;
	}

	public static boolean isZero(double[] iline) {
		return isZero(iline, TOLERANCE);
	}

	public static boolean isZero(double[] iline, double tol) {
		for (double d : iline)
			if (Math.abs(d) > tol)
				return false;
		return true;
	}

	public static float[] times(float[] dst, float[] src1, float[] src2) {
		if (src1.length != src2.length) {
			throw new IllegalArgumentException("Matrices must be same size");
		}
		int n = mysqrt(src1.length);
		float[] out;
		boolean rewrite = false;
		if (dst == src1 || dst == src2 || dst == null) {
			out = new float[src1.length];
			if (dst != null)
				rewrite = true;
		} else {
			out = dst;
		}
		if (out.length != src1.length) {
			throw new IllegalArgumentException("Matrices must be same size");
		}

		for (int i = 0; i < n; ++i) {
			for (int j = 0; j < n; ++j) {
				out[i * n + j] = 0.0f;
				for (int k = 0; k < n; ++k) {
					// the (i,j)th position is the inner product of the ith row
					// and
					// the jth column of the two factors
					out[i * n + j] += src1[i * n + k] * src2[k * n + j];
				}
			}
		}
		if (dst == null)
			return out;
		if (rewrite)
			System.arraycopy(out, 0, dst, 0, dst.length);
		return out;
	}

//	public static double[][] transpose(double[][] dst, double[][] dm) {
//		if (dst == null || dst.length != dm[0].length || dst[0].length != dm.length) 
//			dst = new double[dm[0].length][dm.length];
//		for (int i = 0; i<dm.length; ++i)	{
//			for (int j = 0; j<dm[0].length; ++j)	{
//				dst[j][i] = dm[i][j];
//			}
//		}
//		return dst;
//	}

}
