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

import java.awt.geom.Rectangle2D;
import java.util.logging.Level;
import java.util.logging.Logger;

import de.jreality.util.LoggingSystem;

/**
 * Static methods for geometry of  real projective 3-space (RP<sup>3</sup>). As with {@link Pn}, some methods 
 * are purely projective, while others relate to the various metric geometries contained 
 * within projective geometry. 
 * <p>
 * The bulk of the methods here fall into three categories: 
 * <ul>
 * <li>Methods to generate isometries of a particular 3-dimensional metric geometry, 
 * <li>Methods related to perspective transformations in 3D rendering, and </li>
 * <li>Methods related to line (Pluecker) coordinates of lines in RP3.</li>
 * </ul>
 * <p>
 * Only methods which are specific to 3-dimensional space are included here. Actually most of
 * the methods of the first sort listed above could be generalized and moved to {@link Pn}. 
 * Any volunteers?
 * <p>
 * Scaling transformations are included here even though scaling is not an isometry in any
 * metric.
 * 
 * {@see de.jreality.math.Rn}  for method conventions and representation of matrices.
 * {@see de.jreality.math.Pn}  for other methods applicable in n-dimensional real projective space.
 * @author Charles Gunn
 *
 */
public class P3 {

	private static boolean debug = false;

	private final static double[] hzaxis = {0,0,1,1};
	public static double[] p3involution = Rn.diagonalMatrix(null, new double[]{-1d,-1d,-1d,-1d});
	public static double[] Q_HYPERBOLIC, Q_EUCLIDEAN, Q_ELLIPTIC;

	public static double[][] Q_LIST;

	 
   final static double[] xaxis = {1,0,0};

	final static double[] yaxis = {0,1,0};

	final static double[] zaxis = {0,0,-1};

	static {
		Q_HYPERBOLIC = Rn.identityMatrix(4);
		Q_HYPERBOLIC[15] = -1.0;
		Q_EUCLIDEAN = Rn.identityMatrix(4);
		Q_EUCLIDEAN[15] = 0.0;
		Q_ELLIPTIC = Rn.identityMatrix(4);
		Q_LIST = new double[3][];
		Q_LIST[0] = Q_HYPERBOLIC;
		Q_LIST[1] = Q_EUCLIDEAN;
		Q_LIST[2] = Q_ELLIPTIC;
	}

	/**
	 * Assuming <code>p0,p1,p</code> are linearly dependent, calculate weights such that
	 * <code>p = weights[0]*p0 + weights[1]*p1</code>.
	 * @param weights
	 * @param p0	4-vector
	 * @param p1	4-vector
	 * @param p
	 * @return	
	 */
	static double[] zeroVector = {0,0,0,0};

	public static double[] originP3 = {0.0, 0.0, 0.0, 1.0};

	private P3()	{}

	/**
	 * 
	 * @param m
	 * @param transV
	 * @param rotQ
	 * @param stretchRotQ
	 * @param stretchV
	 * @param isFlipped
	 * @param metric
	 * @return
	 */
	public static double[] composeMatrixFromFactors(double[] m, double[] transV, Quaternion rotQ, 
		Quaternion stretchRotQ, double[] stretchV, boolean isFlipped, int metric)
	{
		// assert dim checks
		double[] transT 	= new double[16],
				rotT 	= new double[16],
				stretchRotT = new double[16],
				stretchT = new double[16],
				tmp	= new double[3];
	
		if (transV == null || rotQ == null || stretchV == null)	{
			throw new IllegalArgumentException("Null argument");
		}
			
	
		P3.makeTranslationMatrix(transT, transV, metric);
		Quaternion.quaternionToRotationMatrix(rotT, rotQ);
		//LoggingSystem.getLogger().log(Level.FINER,Rn.matrixToString(rotT));
	
		/* for now we ignore the stretch rotation */
		Quaternion.quaternionToRotationMatrix(stretchRotT, stretchRotQ);
	
		//LoggingSystem.getLogger().log(Level.FINER,Rn.matrixToString(stretchT));
		if (isFlipped == true)	{
			//tmp = Rn.times(null, -1.0, stretchV);
			for (int i = 0; i<3; ++i) tmp[i] = -stretchV[i];
		} else {
			System.arraycopy(stretchV, 0, tmp, 0, 3);
		}
		Rn.setDiagonalMatrix(stretchT, tmp);
		//LoggingSystem.getLogger().log(Level.FINER,"Stretch"+isFlipped+Rn.matrixToString(stretchT));
	
		/* what ** should **  happen with the "stretch rotataion */
		//Rn.conjugate(stretchT, stretchT, stretchRotT);
		Rn.times( m, rotT, stretchT);
		Rn.times(m, transT, m);
	
		return m;
	}
	
	public static float[] makePerspectiveProjectionMatrix(float[] dst, Rectangle2D viewport, float near, float far)	{
		// assert dim checks
	 	if (dst == null) dst = new float[16];
	 	float an = Math.abs(near);
	 	float l = (float)viewport.getMinX() * an;
	 	float r = (float)viewport.getMaxX() * an;
	 	float b = (float)viewport.getMinY() * an;
	 	float t = (float)viewport.getMaxY() * an;
		Rn.setIdentityMatrix(dst);
		dst[0] = 2*near/(r-l);
		dst[5] = 2*near/(t-b);
		dst[10] = (far+near)/(near-far);
		dst[15] = 0.0f;
		dst[2] = (r+l)/(r-l);
		dst[6] = (t+b)/(t-b);
		dst[11] = 2*near*far/(near-far);
		dst[14] = -1.0f;
		return dst;
	}
	
	/**
	 * Extract a matrix from the <i>src</i> input matrix, such that it fixes the input position <i>point</i>.
	 * @param dst
	 * @param src
	 * @param point
	 * @param metric
	 * @return
	 */
	public static double[] extractOrientationMatrix(double[] dst, double[] src, double[] point, int metric)	{
		if (dst == null) dst = new double[16];

		double[] image = Rn.matrixTimesVector(null, src, point);
		double[] translate = P3.makeTranslationMatrix(null, image, metric);
		Rn.times(dst, Rn.inverse(null, translate), src );
//		System.out.println("The input matrix is "+Rn.matrixToString(src));
//		System.out.println("The orientation matrix is "+Rn.matrixToString(dst));
		return dst;
	}

	// TODO really diagonalize the quadratic form Q instead of assuming it's diagonal
	/**
	 * see Graphics Gems IV, p. 207 for details 
	 * "polar matrix decomposition" yields a factorization of the form
	 * A = TRFUSU'
	 * where	T	translation
	 *		F	flip  (x[i] -> -x[i], i=0,1,2)
	 *		R	rotation
	 *		U	"stretch rotation"
	 *		S	scale
	 *		U'	U transpose
	 *
	 * Additional argument for composeMatrixFromFactors
	 *		centerV	center of rotation
	 *
	 * 
	 *	Currently U is assumed to be the identity, and metric must be EUCLIDEAN.
	 *
	 * @param m
	 * @param transV
	 * @param rotQ
	 * @param stretchRotQ
	 * @param stretchV
	 * @param isFlipped
	 * @param metric
	 * @return
	 */
	public static double[] factorMatrix(double[] m, double[] transV, Quaternion rotQ, Quaternion stretchRotQ, double[] stretchV, boolean isFlipped[], int metric)
	{
		double[] itransT = new double[16], 
			transT = new double[16], 
			tmp = new double[16],
			M3 = new double[9], 
			Q3 = new double[9], 
			S3 = new double[9];
		double det;
	
		//ASSERT( (vT && qR && qSR && vS && isFlipped && A), OE_NULLPTR, OE_DEFAULT, " ", return A;)
	
		/* see if it's a reflection */
		det = Rn.determinant(m);
		//System.out.println("Det ="+det);
		isFlipped[0] = (det < 0); 
	
		/* first extract the translation part */
		Rn.matrixTimesVector(transV, m, P3.originP3);
		if (metric == Pn.EUCLIDEAN && transV[3] == 0.0)	{
			throw new IllegalArgumentException("bad translation vector");
		}
		P3.makeTranslationMatrix(transT, transV, metric);
		Rn.inverse(itransT, transT);
		// undo the translation first
		Rn.times(tmp, itransT, m);
		//LoggingSystem.getLogger().log(Level.FINER,Rn.matrixToString(tmp));
	
		/* next polar decompose M */
		Rn.extractSubmatrix(M3, tmp, 0, 2, 0, 2);
		if (isFlipped[0] == true) Rn.times(M3, -1.0, M3);
		//LoggingSystem.getLogger().log(Level.FINER,Rn.matrixToString(M3));
		Rn.polarDecompose(Q3, S3, M3);
		//LoggingSystem.getLogger().log(Level.FINER,Rn.matrixToString(Q3));
		//LoggingSystem.getLogger().log(Level.FINER,Rn.matrixToString(S3));
	
		/* we pretend that we have a diagonal scale matrix */
		stretchV[0] = S3[0];	stretchV[1] = S3[4];	stretchV[2] = S3[8];
	
		Quaternion.rotationMatrixToQuaternion(rotQ, Q3);
		/* and that the other rotation is the identity */
		stretchRotQ.setValue(1.0, 0.0,  0.0, 0.0);
	
		return m;
	}



	/**
	 * Generate a glide reflection in  <i>plane</i> with translation vector given by the vector from
	 * <i>p1</i> to </i>p2</i>.  Both these points must lie in the plane.
	 * Under construction.
	 * @param m
	 * @param vec
	 * @return
	 * not referenced
	 */
	public static double[] makeGlideReflectionMatrix(double[] m, 
			double[] p1, 
			double[] p2, 
			double[] plane,
			int metric)	{
		
		if (p1.length == 3) p1 = Pn.homogenize(null, p1);
		if (p2.length == 3) p2 = Pn.homogenize(null, p2);
		double d = Rn.innerProduct(p1, plane);
		if ( Math.abs(d) > 10E-8) 
			throw new IllegalStateException("points must lie in plane");
		d = Rn.innerProduct(p2, plane);
		if ( Math.abs(d) > 10E-8) 
			throw new IllegalStateException("points must lie in plane");
		// TODO check that the following always leaves plane invariant
		// i.e., all planes through the line p1-p2 are preserved by the translation (should be!)
		double[] tlate = makeTranslationMatrix(null, p1, p2, metric);
		double[] reflection = makeReflectionMatrix(null, plane, metric);
		m = Rn.times(m, tlate, reflection);
		double[] m2 = Rn.times(null, reflection, tlate);
		double[] xx = Rn.times(null, m, Rn.inverse(null,m2));
		if (!Rn.isIdentityMatrix(xx, 10E-8)) {
			throw new IllegalStateException("they don't commute!");
		}
		return m;
	}


	/**
	 * Creates an isometry that carries the <i>from</i> vector to the origin; and takes the 
	 * normalized <i>to</i> vector to the (homogeneous) vector <i>(0,0,-1,0)</i>. The orthogonal complement of these two
	 * vectors is rotated by <i>roll</i> degrees.  This transformation represents the <i>worldToCamera<i>
	 * transformation for a camera whose position in world coordinates is the point <i>from</i> and which is pointed to look in
	 * the direction of <i>to</i>.
	 * @param m
	 * @param from
	 * @param to
	 * @param roll
	 * @param metric
	 * @return
	 */public static double[] makeLookatMatrix(double[] m, double[] from, double[] to, double roll, int metric)	{
		// assert dim checks
		double[] newto = new double[4];
		double[] tm1 = new double[16];
		double[] tm2 = new double[16];
		if (m == null) m = new double[16];

		P3.makeTranslationMatrix(tm1, from,  metric);
		//LoggingSystem.getLogger().log(Level.FINER,Rn.matrixToString(tm1));
		Rn.inverse(tm1, tm1);		// tm1 brings from to (0,0,0,1)
		//ystem.out.println(Rn.matrixToString(tm1));
		Rn.matrixTimesVector(newto, tm1, to);
//		LoggingSystem.getLogger(P3.class).log(Level.FINER,Rn.toString(newto));
		P3.makeRotationMatrix(tm2, newto, P3.zaxis);
//		System.out.println("rotationMatrix: "+Rn.toString(Rn.matrixTimesVector(null, tm3, newto)));
		// the matrix m takes to -> newto -> zaxis
		Rn.times(m, tm2, tm1);
		if (roll != 0)	{
			P3.makeRotationMatrix(tm1, P3.zaxis, roll);
			Rn.times(m, m, tm1);
		}
//		System.out.println("lookat: "+Rn.toString(Rn.matrixTimesVector(null, m, from)));
//		System.out.println("rotationMatrix: "+Rn.toString(Rn.matrixTimesVector(null, Rn.times(null, tm2, tm1), from)));
//		System.out.println("lookat: "+Rn.toString(Rn.matrixTimesVector(null, m, to)));
//		System.out.println("rotationMatrix: "+Rn.toString(Rn.matrixTimesVector(null, Rn.times(null, tm2, tm1), to)));
		return m;
	}
	/**
	 * 
	 * @param m
	 * @param viewport
	 * @param n
	 * @param f
	 * @return
	 */
	public static double[] makeOrthographicProjectionMatrix(double[] m, Rectangle2D viewport, double near, double far)	{
		// assert dim checks
		double l = viewport.getMinX();
		double r = viewport.getMaxX();
		double b = viewport.getMinY();
		double t = viewport.getMaxY();
		if (m == null) m = new double[16];
		Rn.setIdentityMatrix(m);
		m[0] = 2/(r-l);
		m[5] = 2/(t-b);
		m[10] = -2/(far-near);
		m[3] = -(r+l)/(r-l);
		m[7] = -(t+b)/(t-b);
		m[11] = -(far+near)/(far-near);
		return m;
	}
	/**
	 * Generate a 4x4 perspective projection matrix based on the parameters.
	 * @param dst			matrix to put the result
	 * @param viewport	The viewport of the camera (normalized to lie in z = -1 plane)
	 * @param n			near clipping plane
	 * @param f			far clipping plane
	 * @return
	 */public static double[] makePerspectiveProjectionMatrix(double[] dst, Rectangle2D viewport, double near, double far)	{
		// assert dim checks
	 	if (dst == null) dst = new double[16];
		double an = Math.abs(near);
		double l = viewport.getMinX() * an;
		double r = viewport.getMaxX() * an;
		double b = viewport.getMinY() * an;
		double t = viewport.getMaxY() * an;
		Rn.setIdentityMatrix(dst);
		dst[0] = 2*near/(r-l);
		dst[5] = 2*near/(t-b);
		dst[10] = (far+near)/(near-far);
		dst[15] = 0.0;
		dst[2] = (r+l)/(r-l);
		dst[6] = (t+b)/(t-b);
		dst[11] = 2*near*far/(near-far);
		dst[14] = -1.0;
		return dst;
	}
	/**
	 * Construct a projective reflection that fixes the element <i>plane</i> considered
	 * as a pole/polar point/plane pair.  That is, the fixed elements of the transformation are
	 * the point <i>vec</i> and the polar plane <i>Q.vec</i> where Q is the diagonal
	 * matrix representing the absolute quadric of the given metric.  Such a transformation
	 * is also known as a harmonic involution.
	 * <b>Warning</b> Under construction!
	 * @param m
	 * @param vec
	 * @param metric
	 * @return
	 */
	public static double[] makeReflectionMatrix(double[] m, double[] plane, int metric)	{
		// TODO assert checks m.length == 16 and vec.length = 3 or 4
		if (plane.length != 4)	{
			throw new IllegalArgumentException("makeReflectionMatrix: Invalid argument");
		}
		double[] reflectionMatrix = null;
		double[] fixedPlane = ((double[]) plane.clone());
		double[] polarPoint = null; // = (double[]) fixedPlane.clone();
		if (m == null) 	reflectionMatrix = new double[16];
		else 			reflectionMatrix = m;
		Rn.setIdentityMatrix(reflectionMatrix);
		polarPoint = Pn.polarizePlane(null, fixedPlane, metric);
		Pn.setToLength(polarPoint, polarPoint,1.0, metric);
	
		switch (metric)	{
			case Pn.ELLIPTIC:
			case Pn.HYPERBOLIC:
				Pn.normalize(fixedPlane, fixedPlane,  metric);
				break;
			case Pn.EUCLIDEAN:		// this is not optimal, I think; but it works
				Pn.normalizePlane(fixedPlane, fixedPlane, metric);
				break;
		}
		
		for (int i = 0; i<4; ++i)	{
			for (int j = 0; j<4; ++j)	{
				reflectionMatrix[i*4 + j] = reflectionMatrix[i*4+j] - 2 * fixedPlane[j]*polarPoint[i];
			}
		}
		return reflectionMatrix;
	}

	 /**
	 * Generate a rotation matrix fixing the origin (0,0,0,1) around the given axis with the given angle.
	 * The matrix is a 4x4 matrix, which differs from the identity matrix only in the upper 3x3 corner.
	 * 
	 * @param m		the target matrix
	 * @param axis	double[3]
	 * @param angle
	 * @return
	 */
	public static double[] makeRotationMatrix(double[] m, double[] axis, double angle)	{
		double[] u = new double[3];
		if (m == null) 	m = new double[16];
		if (axis.length < 3)	{
			throw new IllegalArgumentException("Axis is wrong size");
		}
		System.arraycopy(axis,0,u,0,3);
		Rn.normalize(u,u);
		double c = Math.cos(angle);
		double s = Math.sin(angle);
		double v = 1.0 - c;
		
		Rn.setIdentityMatrix(m);
		m[0] = u[0] * u[0] * v + c;
		m[4] = u[0] * u[1] * v + u[2] * s;
		m[8] = u[0] * u[2] * v - u[1] * s;
	
		m[1] = u[1] * u[0] * v - u[2] * s;
		m[5] = u[1] * u[1] * v + c;
		m[9] = u[1] * u[2] * v + u[0] * s;
	
		m[2] = u[2] * u[0] * v + u[1] * s;
		m[6] = u[2] * u[1] * v - u[0] * s;
		m[10] = u[2] * u[2] * v + c;
		
		return m;
	}

	/**
	 * Generate a rotation matrix which fixes the origin (0,0,0,1) and carries the vector <i>from</i> to the vector <i>to</i>.
	 * The output matrix is 4x4.  <i>from</i> and <i>to</i> are 3-vectors.
	 * 
	 * @param m		double[16]
	 * @param from	double[3]
	 * @param to	double[3]
	 * @return
	 */
	public static double[] makeRotationMatrix(double[] m, double[] from, double[] to)	{
		// assert dim checks; only valid for P3
		if (from.length < 3 || to.length < 3)	{
			throw new IllegalArgumentException("Input vectors too short");
		}
		double[][] vecs = new double[3][3];
		System.arraycopy(from,0,vecs[0],0,3);
		System.arraycopy(to,0,vecs[1],0,3);
		Rn.normalize(vecs[0], vecs[0]);
		Rn.normalize(vecs[1], vecs[1]);
		double cosAngle = Rn.innerProduct(vecs[0], vecs[1]);
		double angle = Math.acos(cosAngle);
		if (Double.isNaN(angle)) {
			// inner product is out of [-1,1] for numerical reasons
			angle=cosAngle>0?0:Math.PI;
		}
		Rn.crossProduct(vecs[2], vecs[0], vecs[1]);
		Rn.normalize(vecs[2], vecs[2]);
		return P3.makeRotationMatrix(m, vecs[2], angle);
	}
	
	/**
	  * Calculate a rotation matrix in the given metric which rotates a given <i>angle</i> about the axis
	  * determined by <i>p1</i> and <i>p2</i>.
	  * @param m
	  * @param p1
	  * @param p2
	  * @param angle
	  * @param metric
	  * @return
	  */public static double[] makeRotationMatrix(double[] m, double[] p1, double[] p2, double angle, int metric)	{
		// assert dim checks; only valid for P3
		if (p1.length < 3 || p2.length < 3)	{
			throw new IllegalArgumentException("Points too short");
		}
	 	if (m == null) m = new double[16];
		double[] tmat = P3.makeTranslationMatrix(null, p1, metric);
		double[] invtmat = Rn.inverse(null, tmat);
		double[] ip2 = new double[4];
		Rn.matrixTimesVector(ip2, invtmat, p2);
		double[] foo = P3.makeRotationMatrix(null, ip2, angle);
		Rn.conjugateByMatrix(m,foo,tmat);
		return m;
	}

	/**
	 * @param object
	 * @param earthPhi
	 * @return
	 */
	public static double[] makeRotationMatrixX(double[] mat, double angle) {
		double[] axis = {1.0, 0.0, 0.0};
		return makeRotationMatrix(mat, axis, angle);
	}

	public static double[] makeRotationMatrixY(double[] mat, double angle) {
		double[] axis = {0.0, 1.0, 0.0};
		return makeRotationMatrix(mat, axis, angle);
	}

	public static double[] makeRotationMatrixZ(double[] mat, double angle) {
		double[] axis = {0.0, 0.0, 1.0};
		return makeRotationMatrix(mat, axis, angle);
	}

	/**
	 * Create a diagonal matric with entries <i>(stretch, stretch,stretch, 1)</i>
	 * @param dst
	 * @param stretch
	 * @return
	 */
	public static double[] makeStretchMatrix(double[] dst, double stretch)	{
		// assert dim checks
		if (dst == null) dst = new double[16];
		int n = Rn.mysqrt(dst.length);
		double[] stretchV = new double[n];
		Rn.setToValue(stretchV, stretch, stretch, stretch, 1.0);
		return makeStretchMatrix(dst, stretchV);
	}
	
	/**
	 * Create a diagonal matric with entries <i>(xscale, yscale, zscale, 1)</i>
	 * 
	 * @param dst
	 * @param xscale
	 * @param yscale
	 * @param zscale
	 * @return
	 */
	public static double[] makeStretchMatrix(double[] dst, double xscale, double yscale, double zscale) {
		// TODO Auto-generated method stub
		if (dst == null) dst = new double[16];
		Rn.setIdentityMatrix(dst);
		dst[0] = xscale;
		dst[5] = yscale;
		dst[10] = zscale;
		return dst;
	}

	/**
	 * Construct a diagonal matrix with the given entries.  The length of <i>v</i> can be arbitrary.
	 * @param dst
	 * @param scales
	 * @return	dst
	 */
	public static double[] makeStretchMatrix(double[] dst, double[] v)	{
		// assert dim checks
		if (dst == null) dst = new double[16];
		int n = Rn.mysqrt(dst.length);
		int ll = Math.min(n, v.length);
		Rn.setIdentityMatrix(dst);
		for (int i = 0; i<ll; ++i)	{
			dst[i*n+i] = v[i];
		}
		return dst;
	}
	
	/**
	 * For those who are uncomfortable with the word <i>stretch</i> -- even though
	 * only euclidean geometry supports scaling (changing size without changing shape).
	 * @param dst
	 * @param s
	 * @return
	 */
	public static double[] makeScaleMatrix(double[] dst, double s)	{
		return makeStretchMatrix(dst, s);
	}
	public static double[] makeScaleMatrix(double[] dst, double[] s)	{
		return makeStretchMatrix(dst, s);
	}
	public static double[] makeScaleMatrix(double[] dst, double sx, double sy, double sz)	{
		return makeStretchMatrix(dst, sx, sy, sz);
	}
	
	public static double[] makeSkewMatrix(double[] dst, int i, int j, double val)	{
		if (dst == null) dst = new double[16];
		System.arraycopy(Rn.identityMatrix(4), 0, dst, 0, 16);
		dst[4*i+j] = val;
		return dst;
	}

	public static double[] makeScrewMotionMatrix(double[] dst, double[] p1, double[] p2, double angle, int metric){
		double[] tlate = makeTranslationMatrix(null, p1, p2, metric);
		double[] rot = makeRotationMatrix(null, p1, p2, angle, metric);
		// debug code
//		double[] m1 = Rn.times(null, tlate, rot);
//		double[] m2 = Rn.times(null, rot, tlate);
//		double[] xx = Rn.times(null, m1, Rn.inverse(null,m2));
//		if (!Rn.isIdentityMatrix(xx, 10E-8)) {
//			throw new IllegalStateException("they don't commute!");
//		}
		return Rn.times(dst, tlate, rot);
	}
	/**
	 * Calculate a translation matrix in the given metric 
	 * which carries the point <i>from</i> to the point <i>to</i> 
	 * and maps the line joining  <i>from</i> and <i>to</i> to itself (the axis of the isometry).
	 * @param dst
	 * @param from
	 * @param to
	 * @param metric
	 * @return
	 */
	  public static double[] makeTranslationMatrix(double[] dst, double[] from, double[] to, int metric)	{
		// assert dim checks
		if (dst == null) 	dst = new double[16];
		double[] TP = makeTranslationMatrix(null, from, metric);
		double[] iTP = Rn.inverse(null, TP);
		double[] toPrime = Rn.matrixTimesVector(null, iTP, to );
		makeTranslationMatrix(dst, toPrime, metric);
		Rn.conjugateByMatrix(dst, dst, TP);
		return dst;
	}

	/**
	 * Calculate a translation matrix which carries the origin <i>(0,0,0,1</i> to 
	 * the point <i>to</i>.
	 * @param mat
	 * @param to
	 * @param metric
	 * @return
	 */
	  public static double[] makeTranslationMatrix(double[] mat, double[] to, int metric)	{
		if (mat == null) mat = new double[16];
		if (true) return makeTranslationMatrixOld(mat, to, metric);
		double[] toL1 = null;
		if (to.length == 3)	toL1 = Pn.homogenize(null, to);
		else if (to.length == 4) toL1 = (double[]) to.clone();
		if ((metric == Pn.EUCLIDEAN && toL1[3] == 0.0))	{
			return Rn.identityMatrix(4);
//			throw new IllegalArgumentException("Infinite euclidean translation vector");
		}
		double[] toL = Pn.normalize(null, toL1, metric);
//		LoggingSystem.getLogger(P3.class).finer("Translation vector is "+Rn.toString(toL));
		if (Double.isNaN(toL[0])) {
			Rn.setIdentityMatrix(mat);
			return mat;
//			throw new IllegalStateException("bad translation vector: "+Rn.toString(toL1));
		}
//		if (toL[3] < 0) Rn.times(toL, -1.0, toL);
		double f = (toL[3] <0) ? 1.0/(1-toL[3]) : 1.0/(1+toL[3]);
			for (int i = 0; i<3; ++i)	{
			for (int j = 0; j<3; ++j)	{
				mat[i*4+j] = ((i == j) ? 1.0 : 0.0 ) - metric * f  * toL[i]*toL[j];
			}
		}
		for (int i = 0; i<4; ++i)	mat[4*i+3] = f * toL[i];
		for (int i = 0; i<3; ++i) mat[12+i] = -metric*mat[4*i+3];
		if (debug)	{
			double[] oldm = makeTranslationMatrixOld(null, to, metric);
			if (! Rn.equals(mat, oldm, 10E-8)) {
				Logger log = LoggingSystem.getLogger(P3.class);
				log.log(Level.WARNING,"Incompatible results:");
				log.log(Level.WARNING,"metric is "+metric);
				log.log(Level.WARNING,"To vector is "+Rn.toString(toL));
				log.log(Level.WARNING,"New: \n"+Rn.matrixToString(mat));
				log.log(Level.WARNING,"Old: \n"+Rn.matrixToString(oldm));
				
			}
			
		}
		return mat;
	}
	  
	  public static boolean isValidTranslationVector(double[] vec, int metric)	{
		  if (vec.length < 4) return true;
		  return !((metric == Pn.EUCLIDEAN && vec[3] == 0.0) || Double.isNaN(vec[0]));
	  
	  }

	/**
	 * Calculate a translation in the given geometry which carries the origin of P3 (0,0,0,1) to the input <i>point</i>.
	 * @param mat
	 * @param tvec
	 * @param metric
	 * @return
	 */
	 private static double[] makeTranslationMatrixOld(double[] mat, double[] p, int metric)	{
		// assert dim checks
		double[] tmp = new double[4];		
		double[] foo = new double[3];
		double[] m;
		if (mat == null)		m = new double[16];
		else					m = mat;
		double[] rot = new double[16];
		double[] mtmp = new double[16];
		double[] point = null;
		if (p.length == 3)	{
			point = new double[4];
			System.arraycopy(p,0,point,0,3);
			point[3] = 1.0;
		} else point = p;
		
		switch(metric)		{
			case Pn.EUCLIDEAN:
				Rn.setIdentityMatrix(m);
				if (point.length == 4){
					Pn.dehomogenize(point, point);
					if (point[3] == 0.0) point[3] = 1.0;
				}
				for (int i = 0; i < 3; ++i)	{
					m[i*4 + 3] = point[i];
				}
				break;
				
			case Pn.HYPERBOLIC:
				if (Pn.innerProduct(point, point, Pn.HYPERBOLIC) > 0.0)	{
					double k = (point[3] * point[3] - .0001)/Rn.innerProduct(point,point,3);
					k = Math.sqrt(k);
					for (int i = 0; i<3; ++i) point[i] *= k;
				}
			case Pn.ELLIPTIC:
				Rn.setIdentityMatrix(mtmp);
				Pn.normalize(tmp, point, metric); 
				System.arraycopy(tmp, 0, foo, 0, 3);
				double d = Rn.innerProduct(foo, foo);
				mtmp[11] = Math.sqrt(d);
				if (metric == Pn.ELLIPTIC) 	mtmp[14] = -mtmp[11];
				else					mtmp[14] = mtmp[11];
				mtmp[10] = mtmp[15] = tmp[3];
				P3.makeRotationMatrix(rot, P3.hzaxis, tmp);
				Rn.conjugateByMatrix(m, mtmp, rot);
				break;
	
			default:		// error!
				break;
		}
		return m;
	}

	/**
	 * Generate a direct isometry that carries the frame determined by <i>p0</i>, <i>p1</i> and <i>p2</i> to that determined
	 * by <i>q0</i>, <i>	q1</i> and <i>q2</i>.  See {@link #makeDirectIsometryFromFrame(double[], double[], double[], int)}.
	 * @param dst
	 * @param p0
	 * @param p1
	 * @param q0
	 * @param q1
	 * @param metric
	 * @return 
	 */
	public static double[] makeDirectIsometryFromFrames(double[] dst, double[] p0, double[] p1, double[] p2,
			double[] q0, double[] q1, double[] q2, int metric) {
		double[] toP = makeDirectIsometryFromFrame(null, p0, p1, p2, metric);
		double[] toQ = makeDirectIsometryFromFrame(null, q0, q1, q2, metric);
		double[] iToP = Rn.inverse(null,toP);
		dst = Rn.times(dst, toQ, iToP);
		return dst;
	}

	/**
	 * Generate a direct isometry which maps the frame <i>F</i> determined by <i>point</i>, <i>xdir</i>, and <i>xydir</i> to the 
	 * standard frame represented by the identity matrix.  <i>F</i> is the frame based at <i>point</i>
	 * whose whose tangent space is spanned by a unit tangent vector in the direction of <i>xdir</i>, with 
	 * a second orthonormal tangent vector in the plane spanned by <i>point</i>, <i>xdir</i>, and <i>xydir</i>.
 	 * the last basis vector is orthogonal to the latter plane.
 	 */
	private static double[] makeDirectIsometryFromFrame(double[] dst, double[] point,
			double[] xdir, double[] xydir, int metric) {
		if (dst == null) dst = new double[16];
		Pn.normalize(point, point, metric);
		// calculate the polar plane of the base point
		double[] polarP = Pn.polarizePoint(null, point, metric);
		// and the plane containing all three given points
		double[] xyplane = planeFromPoints(null, point, xdir, xydir);
		// this is the normalized tangent vector in the xdirection
		double[] pxdir = Pn.normalize(null, lineIntersectPlane(null, point, xdir, polarP), metric);
		double[] pzdir = Pn.polarizePlane(null, xyplane, metric);
		Pn.normalize(pzdir, pzdir, metric);
		double[] pydir = lineIntersectPlane(null, point, xydir, polarP);
		pydir = Pn.normalize(pydir, Pn.projectOntoComplement(null, pxdir, pydir, metric), metric);
		makeMatrixFromColumns(dst, pxdir, pydir, pzdir, point);
		return dst;
	}
		
	private static double[] makeMatrixFromColumns(double[] dst, double[] p0, double[] p1, double[] p2, double[] p3) {
		if (dst == null) dst = new double[16];
		double[][] ptrs = {p0, p1, p2, p3};
		for (int i = 0; i<4; ++i)	{
			for (int j = 0; j<4; ++j)	{
				dst[4*i+j] = ptrs[j][i];
			}
		}
		return dst;
	}
 

	/**
	 * Calculate the determinant of the matrix spanned by the three input vectors
	 * and the origin <i>(0,0,0,1)</i>.
	 * @param to
	 * @param up
	 * @param upNoRoll
	 * @return
	 */
	 public static double orientation(double[] to, double[] up, double[] upNoRoll) {
		double[] mat = new double[16];
		System.arraycopy(to, 0, mat,0,4);
		System.arraycopy(up, 0, mat, 4, 4);
		System.arraycopy(upNoRoll,0,mat,8,4);
		mat[15] = 1.0;
		return Rn.determinant(mat);
	}
	
	/**
	 * Attempt to convert a matrix <i>m</i> into an isometry with respect to metric <i>metric</i>.
	 * This is useful if round-off errors have accumulated through a continuous sequence of motions.
	 * @param dst
	 * @param m
	 * @param tolerance
	 * @param metric
	 * @return
	 */
	 public static double[] orthonormalizeMatrix(double[] dst, double[] m, double tolerance, int metric)		{
		if (dst == null) dst = new double[16];
		if (metric == Pn.EUCLIDEAN)	{
			// TODO fix the euclidean case; in the meantime punt
			if (dst == m) return dst;
			System.arraycopy(m, 0, dst, 0, 16);
			return dst;
		}
		double lastentry = m[15];
		double[] diagnosis = getTransformedAbsolute(m, metric);
//		double nn = 1.0;
//		if (diagnosis[0] != 0) nn = 1.0/diagnosis[0];
//		Rn.times(diagnosis, nn, diagnosis);
//		if (Rn.maxNorm(diagnosis) < tolerance)		{
//			return null;
//		}
		boolean mydebug = false;
		if (mydebug)	{
			LoggingSystem.getLogger(P3.class).log(Level.FINER,"m =");
			LoggingSystem.getLogger(P3.class).log(Level.FINER,Rn.matrixToString(m));
			LoggingSystem.getLogger(P3.class).log(Level.FINER,"Diagnosis is");
			LoggingSystem.getLogger(P3.class).log(Level.FINER,Rn.matrixToString(diagnosis));			
		}
		double[][] basis = new double[4][4];
		double[] Q = Q_LIST[metric+1];
		// the columns of m are the basis vectors (image of canonical basis under the isometry)
		for (int i = 0; i<4; ++i)	 for (int j = 0; j<4; ++j)	basis[i][j] = m[j*4+i];
		// first orthogonalize
		for (int i = 0; i<3; ++i)		
			for (int j = i+1; j<4; ++j)	{	
				if (Q[5*j] == 0.0) continue;
				if (Math.abs(diagnosis[4*i+j]) > tolerance)	{
					Pn.projectOntoComplement(basis[j], basis[i], basis[j], metric);
				}
			}
		// then normalize
		for (int i = 0; i<4; ++i)		{
			if (Q[5*i] != 0.0)	Pn.normalizePlane(basis[i], basis[i],  metric);
			for (int j = 0; j<4; ++j)		dst[j*4+i] = basis[i][j];
		}
		// TODO figure out how to avoid this clean-up for euclidean case
		if (metric == Pn.EUCLIDEAN)	{
			for (int i = 0; i<4; ++i)	{dst[12+i] = 0.0;  dst[4*i+3] = m[4*i+3]; }
		}
		// for now just print out the table of inner products
		diagnosis = Rn.subtract(null, Q, 
				Rn.times(null, Rn.transpose(null, dst), Rn.times(null, Q_LIST[metric+1],dst )));
		if (mydebug)	{
			LoggingSystem.getLogger(P3.class).log(Level.FINER,"dst =");
			LoggingSystem.getLogger(P3.class).log(Level.FINER,Rn.matrixToString(dst));
			LoggingSystem.getLogger(P3.class).log(Level.FINER,"Revised is");
			LoggingSystem.getLogger(P3.class).log(Level.FINER,Rn.matrixToString(diagnosis));			
		}
		if ( dst[15] * lastentry < 0) Rn.times(dst, -1, dst);
		return dst;
	}

	public static double[] getTransformedAbsolute(double[] m, int metric) {
		double[] diagnosis = Rn.subtract(null, Q_LIST[metric+1], 
				Rn.times(null, Rn.transpose(null, m), Rn.times(null, Q_LIST[metric+1], m )));
		return diagnosis;
	}
	
	 public static boolean isometryIsUnstable(double[] matrix, int metric)	{
		 if (metric != Pn.HYPERBOLIC) return false;
		 double max = Rn.maxNorm(matrix);
		 return (max > 200);
	 }
	/**
	 * Calculate the plane coordinates for the plane which lies midway between the input
	 * planes <i>p1</i> and <i>p2</i>. Midway in this case means the distances (with respect to
	 * <i>metric</i>) of <i>dst</i> to the two inputs are equal. There are generally two
	 * such planes when the measure on the plane pencil spanned by the two inputs is elliptic;
	 * we try to choose the one closer to both of the planes.
	 * @param dst
	 * @param p1
	 * @param p2
	 * @param metric
	 * @return
	 */public static double[] perpendicularBisector(double[] dst, double[] p1, double[]p2, int metric)	{
		// TODO assert dim checks
		if (p1.length != 4 || p2.length != 4)	{
			throw new IllegalArgumentException("Input points must be homogeneous vectors");
		}
		if (dst == null) dst = new double[4];
		double[] midpoint = new double[4];
		if (metric == Pn.EUCLIDEAN) {
			Rn.add(midpoint,p1,p2);
			Rn.times(midpoint, .5, midpoint);
			Pn.dehomogenize(midpoint, midpoint);
			Rn.subtract(dst, p2, p1);
			dst[3] = -(Rn.innerProduct(dst, midpoint, 3));
			return dst;			
		}
		Pn.linearInterpolation(midpoint,p1,p2, .5, metric);
		double[] polarM = Pn.polarize(null, midpoint, metric);
		double[] pb = P3.lineIntersectPlane(null, p1, p2, polarM);
		Pn.polarize(dst, pb, metric);
		if (Rn.innerProduct(dst,p1) > 0)	Rn.times(dst, -1.0, dst);
		return dst;
	}
	/**
	 * Construct the homogeneous plane coordinates for the plane containing the three points <i>(p1, p2, p3)</i>.
	 * The method does not check for degenerate conditions.
	 * @param plane	double[4]
	 * @param p1	double[3] or double[4]
	 * @param p2	double[3] or double[4]
	 * @param p3	double[3] or double[4]
	 * @return		plane
	 */
	 public static double[] planeFromPoints(double[] planeIn, double[] p1, double[]p2, double[]p3)	{
		if (p1.length < 3 || p2.length <3 || p3.length < 3)	{
			throw new IllegalArgumentException("Input points must be homogeneous vectors");
		}
		double[] plane;
		if (planeIn == null) 	plane = new double[4];
		else					plane = planeIn;
		if (p1.length == 3 || p2.length == 3 || p3.length == 3)	{
			plane[0] = p1[1]*(p2[2]-p3[2]) - p1[2]*(p2[1]-p3[1]) + (p2[1]*p3[2]-p2[2]*p3[1]);
			plane[1] = p1[0]*(p2[2]-p3[2]) - p1[2]*(p2[0]-p3[0]) + (p2[0]*p3[2]-p2[2]*p3[0]);
			plane[2] = p1[0]*(p2[1]-p3[1]) - p1[1]*(p2[0]-p3[0]) + (p2[0]*p3[1]-p2[1]*p3[0]);
			plane[3] = p1[0]*(p2[1]*p3[2]-p2[2]*p3[1]) - p1[1]*(p2[0]*p3[2]-p2[2]*p3[0]) + p1[2]*(p2[0]*p3[1]-p2[1]*p3[0]);			
//			throw new IllegalStateException("Can't work with 3-vectors");
		} else {
			plane[0] = p1[1]*(p2[2]*p3[3]-p2[3]*p3[2]) - p1[2]*(p2[1]*p3[3]-p2[3]*p3[1]) + p1[3]*(p2[1]*p3[2]-p2[2]*p3[1]);
			plane[1] = p1[0]*(p2[2]*p3[3]-p2[3]*p3[2]) - p1[2]*(p2[0]*p3[3]-p2[3]*p3[0]) + p1[3]*(p2[0]*p3[2]-p2[2]*p3[0]);
			plane[2] = p1[0]*(p2[1]*p3[3]-p2[3]*p3[1]) - p1[1]*(p2[0]*p3[3]-p2[3]*p3[0]) + p1[3]*(p2[0]*p3[1]-p2[1]*p3[0]);
			plane[3] = p1[0]*(p2[1]*p3[2]-p2[2]*p3[1]) - p1[1]*(p2[0]*p3[2]-p2[2]*p3[0]) + p1[2]*(p2[0]*p3[1]-p2[1]*p3[0]);			
		}
		plane[0] *= -1;
		plane[2] *= -1;
		return plane;
	}
	
	 
		/**
		 * Calculate the intersection point of the line determined by <i>p1</i> and <i>p2</i> with <i> plane</i>.
		 * @param point
		 * @param p1
		 * @param p2
		 * @param plane
		 * @return The intersection point
		 */
	 public static double[] lineIntersectPlane(double[] dst, double[] p1, double[] p2, double[] plane)	{
			if (plane.length != 4)	{
				throw new IllegalArgumentException("lineIntersectPlane: plane has invalid dimension");
			}
			double[] point1 , point2;
			if (dst == null || dst.length != 4)	dst = new double[4];
			if (p1.length == 3)  point1 = Pn.homogenize(null, p1);
			else point1 = p1;
			if (p2.length == 3)  point2 = Pn.homogenize(null, p2);
			else point2 = p2;
			double k1 = Rn.innerProduct(point1, plane);
			double k2 = Rn.innerProduct(point2, plane);
			// both points lie in the plane!
			if (k1 == 0.0 && k2 == 0.0)	{
//				LoggingSystem.getLogger(P3.class).log(Level.WARNING,"lineIntersectPlane: Line lies in plane");
				System.arraycopy(p1, 0, dst, 0, Math.min(p1.length,dst.length));
			} else {
				double[] tmp = new double[4];
				Rn.linearCombination(tmp, k2, point1, -k1, point2);
				Pn.dehomogenize(dst, tmp);
			}
//			LoggingSystem.getLogger(P3.class).log(Level.FINER,"k1 is "+k1+" and k2 is "+k2);
//			LoggingSystem.getLogger(P3.class).log(Level.FINER,"p1 is "+Rn.toString(p1)+" and p2 is "+Rn.toString(p2));
//			LoggingSystem.getLogger(P3.class).log(Level.FINER,"result is "+Rn.toString(dst));
			return dst;
		}


	 /**
	 * Via duality, an alias for {@link #planeFromPoints(double[], double[], double[], double[])}.
	 * @param point
	 * @param p1
	 * @param p2
	 * @param p3
	 * @return
	 */
	 public static double[] pointFromPlanes(double[] point, double[] p1, double[]p2, double[]p3)	{
		return planeFromPoints(point, p1, p2, p3);
	}

	public static double[] lineJoinPoint (double[] plane, double[] p1, double[] p2, double[] point)	{
		return lineIntersectPlane(plane, p1, p2, point);
	}

	public static boolean areCollinear(double[] p0, double[] p1, double[] p2, double tol) {
		double[] plane = planeFromPoints(null, p0, p1, p2);
		return Rn.equals(plane, P3.zeroVector, tol);
	}
	
	public static double[] barycentricCoordinates(double[] weights, double[] p0,
				double[] p1, double[] p) {
			// handle the case that p0,p1,p are linearly independent
			// project p onto the line through p0 and p1
			double[] plane = planeFromPoints(null, p0, p1, p);
			 if ( !Rn.equals(P3.zeroVector, plane, 10E-8)) {
					plane = Rn.subtract(null, p0, p1);
					plane[3] = -Rn.innerProduct(plane, p, 3);
					p = lineIntersectPlane(null, p0, p1, plane);
			 }
	//			 throw new IllegalArgumentException("Not linearly dependent.");
	         if (weights == null) weights = new double[2];
	         // find two indices which are linearly independent in p0 and p1
	         double det = 0;
	         int index0= 0, index1 = 0;
	         int n = Math.min(p0.length, p1.length);
	         for (; index0<n-1; ++index0)	{
	        	 for (index1 = index0+1; index1 < n; ++index1)	{
	        		 det = p0[index0]*p1[index1] - p0[index1]*p1[index0];
	        		 if (Math.abs(det) > 10E-8) break;
	        	 }
	        	 if (index1 != n) break;
	         }
	         if (index0 == n-1 && index1 == n) return weights;
	//          System.err.println("i:j= "+index0+":"+index1);
	        double a = p0[index0], b = p1[index0], c = p0[index1], d = p1[index1];
	         weights[0] = (d*p[index0] - b*p[index1])/det;
	         weights[1] = (-c*p[index0] + a*p[index1])/det;
	         double[] p2 = Rn.linearCombination(null, weights[0], p0, weights[1], p1);
	//         System.err.println("p0= "+Rn.toString(p0));
	//         System.err.println("p1="+Rn.toString(p1));
	//         System.err.println("p= "+Rn.toString(p));
	//         System.err.println("alpha:beta= "+weights[0]+":"+weights[1]);
	//         System.err.println("p2="+Rn.toString(p2));
	         return weights;
		}

	public static double affineCoordinate(double[] p1 ,double[] p2, double[] pw )	{
	      double[] weights = barycentricCoordinates(null, p1, p2, pw);
		  if (weights[1] == 0) return 0;
	      double affCoord = Double.MAX_VALUE;
	      if (weights[0] != 0.0) affCoord = weights[1]/weights[0];
	      else if (weights[1] < 0) affCoord = -Double.MAX_VALUE;
	      return affCoord;
	  }


}
