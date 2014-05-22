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



/**
 * <p>
 * A simple quaternion class for support for {@link de.jreality.math.FactoredMatrix} and isometry generation
 * in {@link P3}.  Although the bulk of the methods are static, there are also some instance methods.
 * </p><p>
 * The generic calling convention is <code>public static Quaternion method(Quaternion result, Quaternion q1, Quaternion q2)</code>
 * where if <i>result</i> is null, a new instance is created and the result is returned in it. 
 * <p>
 * Consult also {@link Rn} for a description of conventions used in method design.
 * @author Charles Gunn
  */
final public class Quaternion implements Cloneable, Serializable {
	static Quaternion INFINITE_QUATERNION = new Quaternion(Double.POSITIVE_INFINITY, 0.0, 0.0, 0.0);

	public double re, x, y, z;
	/**
	 * The default quaternion is 1.
	 */
	public Quaternion() {
		this(1d, 0d, 0d, 0d);
	}
	
	/**
	 * A copy constructor.
	 * @param nq
	 */
	public Quaternion(Quaternion nq) {
		this(nq.re, nq.x, nq.y, nq.z);
	}
	
	public Quaternion(double r, double dx, double dy, double dz)	{
		super();
		re = r;
		x = dx;
		y = dy;
		z = dz;
	}
	
	public String toString()	{
		return "re: "+String.format("%8.6f\t",re)+
		"i: "+String.format("%8.6f\t",x)+
		"j: "+String.format("%8.6f\t",y)+
		"k: "+String.format("%8.6f",z);
	}
		
	public static double[] asDouble(double[] dst, Quaternion q)	{
		if (dst == null) dst = new double[4];
		dst[0] = q.re; dst[1] = q.x; dst[2] = q.y; dst[3] = q.z;
		return dst;
	}
	
	public void setValue(double r, double dx, double dy, double dz)	{
		re = r;
		x = dx;
		y = dy;
		z = dz;
	}
	
	// static methods start here
	public static Quaternion copy(Quaternion dst, Quaternion src)	{
		if (dst == null) return new Quaternion(src);
		dst.re = src.re;
		dst.x = src.x;
		dst.y = src.y;
		dst.z = src.z;
		return dst;
	}
	
	/**
	 * return imaginary part as a double array.
	 * @param dst
	 * @param q
	 * @return
	 */
	public static double[] IJK(double[] dst, Quaternion q)	{
		// assert dim checks
		if (dst == null) dst = new double[3];
		dst[0] = q.x;
		dst[1] = q.y;
		dst[2] = q.z;
		return dst;
	}
	
	/**
	 * Check for numerical equality.
	 * @param a
	 * @param b
	 * @param tol
	 * @return
	 */
	public static boolean equals(Quaternion a, Quaternion b, double tol)	{
		Quaternion tmp = new Quaternion();
		subtract(tmp, a, b);
		double ll = length(tmp);
		return ll < tol;
	}

	/**
	 * Check if the rotations represented by the two quaternions are equal
	 * @param a
	 * @param b
	 * @param tol
	 * @return
	 */
	public static boolean equalsRotation(Quaternion a, Quaternion b, double tol)	{
		Quaternion tmp = new Quaternion();
		return (equals(a,b,tol) || equals(times(tmp,-1.0,a),b,tol));
	}

	public static Quaternion exp(Quaternion dst, double t, Quaternion src)	{
		if (dst == null) dst = new Quaternion();
		dst.re = Math.cos(t);
		double s = Math.sin(t);
		dst.x = s*src.x;
		dst.y = s*src.y;
		dst.z = s*src.z;
		return dst;
	}
	public static Quaternion add(Quaternion dst, Quaternion a, Quaternion b)	{
		if (dst == null) dst = new Quaternion();
		if (a == null || b == null) {
			return dst;} 
		dst.re = a.re + b.re;
		dst.x = a.x + b.x;
		dst.y = a.y + b.y;
		dst.z = a.z + b.z;
		return dst;
	}
	
	public static Quaternion negate(Quaternion dst, Quaternion src)	{
		if (dst == null) dst = new Quaternion();
		if (src == null) {
			return dst;} 
		dst.re = -src.re;
		dst.x = -src.x;
		dst.y = -src.y;
		dst.z = -src.z;
		return dst;
	}
	
	public static Quaternion conjugate(Quaternion dst, Quaternion src)	{
		if (dst == null) dst = new Quaternion();
		if (src == null) {
			return dst;} 
		dst.re = src.re;
		dst.x = -src.x;
		dst.y = -src.y;
		dst.z = -src.z;
		return dst;
	}
	
	public static Quaternion subtract(Quaternion dst, Quaternion a, Quaternion b)	{
		if (dst == null) dst = new Quaternion();
		if (a == null || b == null) {
			return dst;} 
		dst.re = a.re - b.re;
		dst.x = a.x - b.x;
		dst.y = a.y - b.y;
		dst.z = a.z - b.z;
		return dst;
	}

	public static Quaternion times(Quaternion dst, double s, Quaternion src)	{
		if (dst == null) dst = new Quaternion();
		dst.re = s * src.re;
		dst.x = s * src.x;
		dst.y = s * src.y;
		dst.z = s * src.z;
		return dst;
	}

	public static Quaternion times(Quaternion dst, Quaternion a, Quaternion b)	{
		// check to see if dst = a or dst = b
		if (dst == null) dst = new Quaternion();
		if (a == null || b == null) {
			return dst;} 
		dst.re = a.re * b.re - a.x*b.x - a.y * b.y - a.z*b.z;
		dst.x =  a.re * b.x + b.re*a.x + a.y * b.z - a.z*b.y;
		dst.y =  a.re * b.y - a.x *b.z + b.re* a.y + a.z*b.x;
		dst.z =  a.re * b.z + a.x * b.y - a.y* b.x + b.re*a.z ;
		return dst;
	}
	
	public static double innerProduct(Quaternion a, Quaternion b)	{
		return (a.re*b.re + a.x*b.x + a.y*b.y + a.z*b.z);
	}
	
	public static double lengthSquared(Quaternion q)	{
		return innerProduct(q,q);
	}

	public static double length(Quaternion q)	{
		return Math.sqrt(lengthSquared(q));
	}
	
	public static Quaternion invert(Quaternion dst, Quaternion src)	{
		Quaternion tmp = new Quaternion();
		double ll = lengthSquared(src);
		if (ll == 0.0)	{
			dst = new Quaternion(INFINITE_QUATERNION);
		} else {		// q^-1 = q * (q bar)/<q,q>
			ll = 1.0/ll;
			conjugate(tmp, src);
			times(dst, ll, tmp);
			//times(dst, tmp, src);
		}
		return dst;
	}
	
	public static Quaternion divide(Quaternion dst, Quaternion a, Quaternion b) {
		Quaternion tmp = new Quaternion();
		invert(tmp, b);
		return times(dst, a, tmp);
	}

	/**
	 * The conjugate of the inverse, or do I mean the inverse of the conjugate???
	 * @param dst
	 * @param src
	 * @return
	 */
	public static Quaternion star(Quaternion dst, Quaternion src)	{
		Quaternion tmp = new Quaternion();
		return conjugate(dst, invert(tmp, src));
	}
	
	public static Quaternion normalize(Quaternion dst, Quaternion src)	{
		double ll = length(src);
		if (ll == 0) dst = new Quaternion(src);
		else {
			ll = 1.0/ll;
			times(dst, ll, src);
		}
		return dst;
	}
	
	public static Quaternion makeRotationQuaternionAngle(Quaternion q, double angle, double[] axis)	{
		double [] tmp = (double[] ) axis.clone();
		double cos = Math.cos(angle/2.0);
		double sin = Math.sin(angle/2.0);
		Rn.normalize(tmp, axis);
		Rn.times(tmp, sin, tmp);
		q.setValue(cos, tmp[0], tmp[1], tmp[2]);
		normalize(q, q);
		return q;
	}
	
	public static Quaternion makeRotationQuaternionCos(Quaternion q, double cos, double[] axis)	{
		return makeRotationQuaternionAngle(q, 2*Math.acos(cos), axis);
	}
	
	private static double[] convert44To33(double[] d) {
		double[] d33 = new double[9];
		d33[0] = d[0];
		d33[1] = d[1];
		d33[2] = d[2];
		d33[3] = d[4];
		d33[4] = d[5];
		d33[5] = d[6];
		d33[6] = d[8];
		d33[7] = d[9];
		d33[8] = d[10];
		return d33;
	}

	/**
	 * Convert the 3x3 rotation matrix <i>mat</i> into a quaternion.
	 * @param q
	 * @param mat
	 * @return
	 */
	public static Quaternion rotationMatrixToQuaternion(Quaternion q, double[] mat)		{
		// assert dim checks
		int n = Rn.mysqrt(mat.length);
//		double[] mat = null;
//		if (n == 4)	{
//			mat = convert44To33(mmm);
//		} else if (n == 3)
//			mat = mmm;
//		else 
//			throw new IllegalArgumentException("Invalid matrix");
		
		double d = Rn.determinant(mat);
		double[] m = null;
		// HACK!
		if (d < 0)	{
			double[] mtmp = new double[9];
			Rn.times(mtmp, -1.0, mat);
			m = mtmp;
		} else
			m = mat;
		if (q == null) q = new Quaternion();
		q.x = Math.sqrt(1 - m[2*n+2] - m[n+1] + m[0])/2;
		if ( q.x > .001 ) {
			q.y = (m[1] + m[n]) / (4 * q.x);
			q.z = (m[2] + m[2*n]) / (4 * q.x);
			q.re = (m[2*n+1] - m[n+2]) / (4 * q.x);
		} else {
			q.y = Math.sqrt(1 - m[2*n+2] + m[n+1] - m[0])/2;
			if ( q.y  > .001) {
				q.x = (m[1] + m[n]) / (4 * q.y);
				q.z = (m[n+2] + m[2*n+1]) / (4 * q.y);
				q.re = (m[2] - m[2*n]) / (4 * q.y);
			} else {
				q.z = Math.sqrt(1 + m[2*n+2] - m[n+1] - m[0])/2;
				if ( q.z  > .001) {
					q.x = (m[2] + m[2*n]) / (4 * q.z);
					q.y = (m[n+2] + m[2*n+1]) / (4 * q.z);
					q.re = (m[n] - m[1]) / (4 * q.z);
				} else {
					q.setValue(1.0, 0.0, 0.0, 0.0);
				}
			}
		}
		normalize(q, q);
//		// try new method (from Blaschke) and compare
//      does NOT WORK for the identit, see QuaternionTest - paul peters, Thu Jul  2 18:24:16 CEST 2009
//		Quaternion r[] = {
//				new Quaternion(0, m[0], m[n], m[2*n]),
//				new Quaternion(0, m[1], m[n+1], m[2*n+1]),
//				new Quaternion(0, m[2], m[n+2], m[2*n+2])
//		};
////		System.err.println("rot = "+Rn.matrixToString(m));
//		Quaternion ret = null;
//		for (int i = 0; i<3; ++i)	{
//			int index0 = i, index1 = (i+1)%3, index2 = (i+2)%3;
//			// e2r3-e3r2-e1-r1
//			ret = subtract(null, 
//				subtract(null, 
//					times(null, eq[index1], r[index2]),
//					times(null, eq[index2], r[index1])),
//				add(null, eq[index0], r[index0]));
//			ret = conjugate(ret, ret);
////			System.err.println("ret = "+ret.toString());
//			if (lengthSquared(ret) > .001) break;
//		}
//		normalize(ret, ret);
////		if (!equals(ret, q, 10E-8)  && !equals(ret, negate(null, q), 10E-8)) {
////			System.err.println("ret = "+ret.toString());
////			System.err.println("q = "+q.toString());
////			throw new IllegalStateException("not equal");			
////		}
		return q;
	}
	static Quaternion eq[] = {
		new Quaternion(0,1,0,0),
		new Quaternion(0,0,1,0),
		new Quaternion(0,0,0,1)
	};
	
	/**
	 * Convert the quaternion <i>qt</i> into a 3x3 rotation matrix.
	 * @param rot
	 * @param qt
	 * @return
	 */
	public static double[] quaternionToRotationMatrixOld( double[] rot, Quaternion qt)	{		
		if (rot == null) rot = new double[16];
		double[] axis = new double[3];
		Quaternion q = new Quaternion();
		normalize(q, qt);
		if (1.0 - Math.abs(q.re) < 10E-16)	{
			Rn.setIdentityMatrix(rot);
			return rot;
		}
		IJK(axis, q);
		Rn.normalize(axis, axis);
		double angle = 2 *  Math.acos(q.re);
		/* fprintf(stderr,"angle is %f\n",angle); */
		return P3.makeRotationMatrix(rot, axis, angle);
	}

	public static double[] quaternionToRotationMatrix( double[] rot, Quaternion qt)	{	
		if (rot == null) rot = new double[16];
		Rn.setToValue(rot, 0.0);
		Quaternion.normalize(qt, qt);
		rot[4] =  2*( qt.re*qt.z + qt.y*qt.x);
		rot[1] =  2*(-qt.re*qt.z + qt.x*qt.y);

		rot[8] = 2*(-qt.re*qt.y + qt.x*qt.z);
		rot[2] =  2*( qt.re*qt.y + qt.x*qt.z);

		rot[9] = 2*( qt.re*qt.x + qt.y*qt.z);
		rot[6] = 2*(-qt.re*qt.x + qt.y*qt.z);
		rot[0] = qt.re*qt.re + qt.x*qt.x - qt.y*qt.y - qt.z*qt.z;
		rot[5] = qt.re*qt.re - qt.x*qt.x + qt.y*qt.y - qt.z*qt.z;
		rot[10]= qt.re*qt.re - qt.x*qt.x - qt.y*qt.y + qt.z*qt.z;
		rot[15] = 1.0;
		return rot;
		
	}
	/**
	 * @param object
	 * @param rot1
	 * @param rot2
	 * @param s
	 * @return
	 */
	public static Quaternion linearInterpolation(Quaternion dst, Quaternion rot1, Quaternion rot2, double s) {
		if (dst ==null) dst = new Quaternion();
		double[] r1 = rot1.asDouble();
		double[] r2 = rot2.asDouble();
		if (Rn.innerProduct(r1, r2) < 0) Rn.times(r2, -1.0, r2);
		double[] val = Pn.linearInterpolation(null, r1, r2, s, Pn.ELLIPTIC);
		dst.setValue(val[0], val[1], val[2], val[3]);
		return dst;
	}

	public double[] asDouble() { return asDouble(null); }
	
	/**
	 * Convert the quaternion into a 4-vector
	 * @return
	 */
	static int[] chan = {0,1,2,3};
	public double[] asDouble(double[] val) {
		return asDouble(val, chan);
	}
  
	public double[] asDouble(double[] val, int[] channels) {
		if (val == null) val = new double[4];
		val[channels[0]] = re;
		val[channels[1]] = x;
		val[channels[2]] = y;
		val[channels[3]] = z;
		return val;
	}
  



}
