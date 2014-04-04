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

import de.jreality.util.LoggingSystem;




/**
 * 	A set of static methods related to real n-dimensional real projective space RP<sup>n</sup>.
 *  In general, points and vectors are represented in homogeneous coordinates by arrays of length <i>n+1</i>.  
 *  By duality, hyperplanes are represented in the same fashion. The last coordinate is considered 
 *  to be the <i>homogeneous</i> coordinate (see for example {@link #dehomogenize(double[], double[])}).
 *  <p>
 *  In addition to purely projective methods,
 *  this includes a number of methods related to the classical 
 *  homogeneous metric spaces (euclidean, hyperbolic, and elliptic)
 *  which are based on a projective model for these spaces.
*  <p>
 *	The methods related to metric geometries generally have a final argument
 * which identifies the geometry.  This can be one of the 3 pre-defined values
 * {@link #HYPERBOLIC}, {@link #EUCLIDEAN}   or {@link #ELLIPTIC}. These correspond to 
 * spaces of constant negative, null, and positive curvature, resp.
 *  <p>
 *  (For the mathematical basis of the derivation of metric geometries from projective geometry 
 *  -- which forms the foundation the functionality of this class -- see
 *  H.M.S. Coxeter, Non-Euclidean Geometry, 1965.)
 *	<p>
 *  <b>Note</b>: there may appear to be some duplication of functionality with methods in {@link de.jreality.math.Rn Rn}. However,
 *  the methods here expect points to be specified with homogeneous coordinates, while those in Rn expect dehomogenized
 *  coordinates (in general -- but see {@link Rn} for exceptions), so there is 
 *  usually only one correct choice for which method to use. But see {@link #normalize(double[], double[], int)}
 *  for an example of some finer points.
  *  <p>
 * See also {@link de.jreality.math.Rn Rn} for more on method conventions.
 * See also {@link de.jreality.math.P2 P2} and {@link de.jreality.math.P3 P3} for 
 * methods specific to two- and three-dimensional real projective geometry.
 * @author Charles Gunn
 */
public class Pn {

  
	public static final int ELLIPTIC	= 1;
	public static final int EUCLIDEAN	= 0;
	public static final int HYPERBOLIC = -1;
	public static final int PROJECTIVE	= 2;
	public static double[] zDirectionP3 = {0.0, 0.0, 1.0, 0.0};
	
	private Pn() {}

	/**
	 * These hyperbolic trig functions fill in a gap in the Java math library!
	 * 
	 * @param arg
	 * @return inverse sinh of arg
	 */
	public static double cosh(double x)	{
		return .5*(Math.exp(x) + Math.exp(-x));
	}
	
	public static double sinh(double x)	{
		return .5*(Math.exp(x) - Math.exp(-x));
	}
	public static double tanh(double x)	{
		return sinh(x)/cosh(x);
	}

	public static double acosh(double x)	{
		return Math.log((x > 0? x : -x)+Math.sqrt(x*x-1));
	}

	public static double asinh(double x)	{
		return Math.log(x+Math.sqrt(x*x+1));
	}
	
	public static double atanh(double x)	{
		return .5*(Math.log((1+x)/(1-x)));
	}
	
	 
	/**
	 * Calculate the angle between the points <i>u</i> and <i>v</i> with respect to
	 * the metric <i>metric</i>.
	 * @param u
	 * @param v
	 * @param metric
	 * @return
	 */
	public static double angleBetween(double[] u, double[]v, int metric)	{
		double uu = innerProductPlanes(u, u, metric);
		double vv = innerProductPlanes(v, v, metric);
		double uv = innerProductPlanes(u, v, metric);
		if (uu == 0 || vv == 0) 	// error: infinite distance
			return (Double.MAX_VALUE);
		double f =  uv/Math.sqrt(Math.abs(uu*vv));
		if (f > 1.0) f = 1.0;
		if (f < -1.0) f = -1.0;
		double d = Math.acos(f);
		return d;
	}
	 /**
	 * Like the method {@link Rn#calculateBounds(double[][], double[][]) calculateBounds} in class Rn, 
	 * but dehomogenizes the points before computing the bound. 
	 * @param bounds	double[2][3]
	 * @param vlist		double[][3]	or double[][4]
	 * @return bounds
	 */
	public static double[][] calculateBounds(double[][] bounds, double[][] vlist) {
		// TODO Auto-generated method stub
		int vl = vlist[0].length;
		int bl = bounds[0].length;
		double[] tmp = new double[vl-1];
		// assert dim check
		//if (bounds.length != 2 && bounds[0].length != n)	bounds = new double[2][n];
		if (vl - 1 > bl) return null;		// TODO error
	
		for (int i =0; i<vl-1; ++i)	{
			bounds[0][i] = Double.MAX_VALUE;
			bounds[1][i] = -Double.MAX_VALUE;
		}
		for (int i=vl-1; i<bl; ++i) {
			bounds[0][i] = bounds[1][i] = 0.0;
		}
		for (int i=0; i< vlist.length; ++i)		{
			if (vlist[i][vl-1] == 0.0 ) continue;		// skip over points at infinity
			Pn.dehomogenize(tmp, vlist[i]);
			Rn.max(bounds[1], bounds[1], tmp);
			Rn.min(bounds[0], bounds[0], tmp);
		
		}
		return bounds;
	}
	 
	/**
	 * Calculate the centroid of <i>points</i> with respect to <i>metric</i>. This basically involves
	 * normalizing the points to have unit length with respect to the given <i>metric</i>
	 * before averaging them.
	 * 
	 * @param average
	 * @param points
	 * @param metric
	 * @return
	 */
	public static double[] centroid(double[] average, double[][] points,int metric) {
		if (average == null)
			average = new double[points[0].length];
		double[] tmp = new double[average.length];
		for (int i = 0; i < points.length; ++i) {
			normalize(tmp, points[i], metric);
			Rn.add(average, tmp, average);
		}
		Rn.times(average, 1.0 / points.length, average);
		normalize(average, average, metric);
		return average;
	}
		
	/**
	 * Dehomogenize the src array into the dst array.  Both must be the same length.
	 * The src array is copied to the destination array if the last element of src is 0.0 or 1.0;
	 * otherwise each element of src is divided by the last element of src and written to the destination.
	 * @param dst
	 * @param src
	 * @return	dst
	 */
	public static double[] dehomogenize(double[] dst, double[] src)	{
		// assert dim checks
		int sl = src.length;
		if (dst == null) dst = new double[src.length];
		int dl = dst.length;
		// allow dst array same length or one shorter than source
		if (! (dl == sl || dl +1 == sl))	{
			throw new IllegalArgumentException("Invalid dimensions");
		}
		double last = src[sl-1];
		if (last == 1.0 || last == 0.0) 	{
			if (src != dst) System.arraycopy(src,0,dst,0,dl);
			return dst;
		}
		last = 1.0/last;
		for (int i = 0; i<dl; ++i)		dst[i] = last * src[i];
		if (dl == sl) dst[dl-1] = 1.0;
		return dst;
	}
	
	/**
	 * Return the value <i>z</i> so that the point <i>(0,0,z,1)</i> lies a distance <i>d</i>
	 * from the origin <i>(0,0,0,1)</i> in the given metric.
	 * @param d
	 * @param metric
	 * @return
	 */public static double coordForDistance(double d, int metric)	{
		switch(metric)	{
		case HYPERBOLIC: return Math.tanh(d); 
		case EUCLIDEAN: default: return d;
		case ELLIPTIC: return Math.tan(d);
		}
	}
	/**
	 * A vectorized version of {@link #dehomogenize(double[], double[])}.
	 * @param dst
	 * @param src
	 * @return	dst
	 */
	public static double[][] dehomogenize(double[][] dst, double[][] src)	{
		// assert dim checks
		int sl = src.length;
		if (dst == null) dst = new double[sl][src[0].length-1];
		int dl = dst.length;
		if (dl != sl)	{
			throw new IllegalArgumentException("Invalid dimensions");
		}
		for (int i = 0; i<sl; ++i)	dehomogenize(dst[i], src[i]);
		return dst;
	}
	

	
	/**
	 * Calculate the distance between the two points <i>u</i> and <i>v</i>. In hyperbolic
	 * geometry distances may be imaginary; here we only return the absolute value of the distance.
	 * <b>Note:</b> This method does not attempt to handle all possible special cases correctly,
	 * as when for example the input points lie on the Absolute Quadric, etc.  It does however handle correctly
	 * various cases in the hyperbolic case, when one or both of the points lie strictly outside the hyperbolic
	 * disk.  For example, when the first lies inside and the second outside, the method returns the metricned 
	 * distance of the first point to the polar line of the second (which is a hyperbolic line).
	 * @param u
	 * @param v
	 * @param metric
	 * @return	the distance
	 */
	public static double distanceBetween(double[] u, double[] v, int metric)	{
		// assert dim checks
		double d = 0;
		int n = u.length;
		switch(metric)	{
			default:
				// error: no such metric.  fall through to euclidean case
			case EUCLIDEAN:
				double ul, vl, ulvl, tmp;
				ul = u[n-1]; vl = v[n-1]; ulvl = ul*vl;
				for (int i = 0; i<n-1; ++i)		{
					tmp = ul*v[i] - vl * u[i];
					d += tmp*tmp;
				}
				d = Math.sqrt(d);
				if ( !(d==0 || d == 1.0))	d /= Math.abs(ulvl);
				if (ul == 0 || vl == 0) d = Double.MAX_VALUE;
				break;
			case HYPERBOLIC:
				double uu, uv, vv;
				uu = innerProduct(u, u, metric);
				vv = innerProduct(v, v, metric);
				uv = innerProduct(u, v, metric);
				if (uu == 0 || vv == 0) 	// error: infinite distance
//					throw new IllegalArgumentException("Points cannot lie on the hyperbolic absolute");
					return (Double.MAX_VALUE);
				double k =  (uv)/Math.sqrt(Math.abs(uu*vv));
				if (uu < 0 && vv < 0) d = acosh(k);
				else if ((uu < 0 && vv > 0) || (uu > 0 && vv < 0)) d = asinh(k);
				else if (uu > 0 && vv > 0) d = Math.acos(k);
				break;
			case ELLIPTIC:
				uu = innerProduct(u, u, metric);
				vv = innerProduct(v, v, metric);
				uv = innerProduct(u, v, metric);
				double ip  =  (uv)/Math.sqrt(Math.abs(uu*vv));
				if (ip>1) ip = 1;
				if (ip < -1) ip = -1;
				d = Math.acos(ip);
				break;
			}
		return d;
	}
	
	/**
	 * Drag a tangent vector <i>sdir</i> based at point <i>src</i> with initial direction given by <i>sdir</i>, a distance
	 * of <i>length</i> in the given metric.  
	 * The resulting point and tangent vector is returned in <i>ddir</i> and <i>dst</i>,
	 * respectively.  
	 * <b>Warning</b>: the source point and tangent vector are assumed to be normalized.  
	 * Use {@link #normalize(double[], double[], double[], double[], int)} 
	 * to perform this normalization before calling this method.
	 * @param dst
	 * @param ddir
	 * @param src
	 * @param sdir
	 * @param length
	 * @param metric
	 * @return
	 */
	public static double[] dragTangentVector(double[] dst, double[] ddir, double[] src, double[] sdir, double  length, int metric)
	{
	    double  c, s;
	    double[] tdir;

	    //ASSERT( (dst && ddir && src && sdir), OE_NULLPTR, OE_DEFAULT, "null pointer argument",
	        //return dst);
	    int n = src.length;
	    if (n != sdir.length)	{
			throw new IllegalArgumentException("Invalid dimensions");
	    }
	    
	    if (ddir != null && ddir.length != n)	{
			throw new IllegalArgumentException("Invalid dimensions");
	    }
	    
	    if (dst == null)	{
	    		dst = new double[n];
	    }
	    switch (metric)     {
	      case  Pn.EUCLIDEAN:
	      	tdir = setToLength(null, sdir, length, metric); 
	      	tdir[n-1] = 0.0;
	        Rn.add(dst, src, tdir);
	        if (ddir != null) Rn.copy(ddir, sdir);
	        break;

	      case Pn.HYPERBOLIC:
			c = cosh( length);
			s = sinh( length);
			Rn.linearCombination(dst, c, src, s, sdir);
			if (ddir != null) Rn.linearCombination(ddir,s, src, c, sdir);
			break;
	
	      case Pn.ELLIPTIC:
			c = Math.cos( length);
			s = Math.sin( length);
			Rn.linearCombination(dst, c, src, s, sdir);
			if (ddir != null) Rn.linearCombination(ddir,-s, src, c, sdir);
			break;
	    }
	    return dst;
	}

	/**
	 * Drag the tangent vector sourceTangent based at sourcePoint to a tangent vector based at dstPoint.
	 * @param ds
	 * @param ds2
	 * @param ds3
	 * @param ds4
	 * @param metric
	 * @see #dragTangentVector(double[], double[], double[], double, int)
	 */
	public static double[] dragTangentVector(double[] dstTangent, double[] sourcePoint, double[] sourceTangent, double[] dstPoint, int metric) {
		double d = distanceBetween(sourcePoint, dstPoint, metric);
		if (dstTangent == null) dstTangent = new double[sourcePoint.length];
		dragTangentVector(null, dstTangent, sourcePoint, sourceTangent, d, metric);
		return dstTangent;
		}
	
	/**
	 * Calculate the point lying a distance length from p0 in the direction p1. 
	 * p1 is first converted into a tangent vector (by projection) and then
	 * {@link #dragTangentVector(double[], double[], double[], double[], double, int) is then called.
	 * 
	 * @param result
	 * @param p0
	 * @param p1
	 * @param length
	 * @param metric
	 * @return
	 */
	public static double[] dragTowards(double[] result, double[] p0, double[] p1, double  length, int metric)
	{
		double[] np0 = new double[p0.length]; //normalize(null, p0, metric);
		double[] np1 = new double[p1.length];
		if (metric == EUCLIDEAN)	{
			int last = p0.length-1;
			normalize(np0, p0, metric);
			normalize(np1, p1, metric);
			if (np1[last] == 1 && np0[last] == 1) Rn.subtract(np1, np1, np0);
			double norm = Rn.euclideanNorm(np1);
			return Rn.add(result, Rn.times(np1, length/norm, np1), np0);
		}
//		System.out.println("Dragging 1"+Rn.toString(p0)+"to "+Rn.toString(p1));
		normalize(np0, p0, metric);
		projectToTangentSpace(np1, np0, p1, metric);
		normalize(np1, np1, metric);
//		System.out.println("Dragging 2"+Rn.toString(np0)+"to "+Rn.toString(np1));
		double[] res = dragTangentVector(result, null, np0, np1, length, metric);
//		System.out.println("Result is"+Rn.toString(res));
		return res;
	}

	/**
	 * Extend the coordinates of <i>src</i> by appending 1.0
	 * @param dst
	 * @param src
	 * @return
	 */
	public static double[] homogenize(double[] dst, double[] src )	{
	 	int n = src.length;
	 	double[] to;
	 	if (dst != null)	{
	 		if (dst.length != n+1)	{
	 			throw new IllegalArgumentException("dst must be length (n+1)");
	 		} 
	 		to = dst;
	 	} else 
	 		to = new double[n+1];
		System.arraycopy(src,0,to,0,n);
		to[n] = 1.0;
		return to;
	 }

	/**
	 * A vector version of {@link #homogenize(double[], double[])}.
	 * @param dst
	 * @param src
	 * @return
	 */
	public static double[][] homogenize(double[][] dst, double[][] src)	{
	 	  if (dst == null)	dst = new double[src.length][src[0].length+1];
	 	  int n = Math.min(dst.length, src.length);
	 	  for (int i = 0; i<n; ++i)	{
	 	  	homogenize(dst[i], src[i]);
	 	  }
	 	  return dst;
	 }

	 /**
	 * Returns the inner product of the two vectors for the given metric.
	 * @param dst
	 * @param src
	 * @param metric
	 * @return	the inner product
	 */
	public static double innerProduct(double dst[], double[] src, int metric)	{
		// assert dim checks
		double sum = 0;
		if (src.length != dst.length)		{
			throw new IllegalArgumentException("Incompatible lengths");
		}
		int n = dst.length;
		
		for (int i = 0; i< n-1; ++i)	sum += dst[i] * src[i];
		double ff = dst[n-1] * src[n-1];
		
		switch(metric)	{
			case HYPERBOLIC:		// metric (n-1,1)
				sum -= ff;
				break;
			case EUCLIDEAN:		// metric (n-1, 0)				
				if (!(ff == 1.0 || ff == 0.0))	sum /= ff;
				break;
			case ELLIPTIC:		// metric (n, 0)
				sum += ff;	
		}
		return sum;
	}
	
	/**
	 * The euclidean metric is not completely self-dual so we need a special method to calculate
	 * inner product of planes
	 * @param dst
	 * @param src
	 * @param metric
	 * @return
	 */
	public static double innerProductPlanes(double dst[], double[] src, int metric)	{
		if (metric != Pn.EUCLIDEAN) return innerProduct(dst, src, metric);
		int n = src.length;
		double sum = 0;
		for (int i = 0; i< n-1; ++i)	sum += dst[i] * src[i];
		return sum;
	}
	
	/**
	 * An alias for {@link #innerProduct(double[], double[], int)}.
	 * @param dst
	 * @param src
	 * @param metric
	 * @return
	 */
	public static double innerProductPoints(double[] dst, double[] src, int metric)	{
		return innerProduct(dst, src, metric);
	}
	 
	public static boolean isValidCoordinate(double[] transVec, int dim, int metric) {
		boolean ret = true;
		if (transVec.length < dim) return false;
		if (metric == Pn.EUCLIDEAN && transVec.length == (dim+1) && transVec[dim] == 0.0) {
			ret = false;
		}
		else if (metric == Pn.HYPERBOLIC)	{
			if (transVec.length == (dim+1) && !(Pn.innerProduct(transVec, transVec, metric) < 0)) ret = false;
			else if (transVec.length == dim && !(Rn.innerProduct(transVec, transVec) < 1)) ret = false;
		}
		if (!ret) {
			LoggingSystem.getLogger(Pn.class).warning("Invalid coordinate: "+Rn.toString(transVec)+" metric: "+metric);
		}
		return ret;
	}

		
	 /**
	 * Linear interpolate respecting the given metric <i>metric</i>.  
	 * That is, find the point <i>p</i> in the linear
	 * span of <i>u</i> and <i>v</i> such that the distance(u,p):distance(u,v) = t.  
	 * For the euclidean case
	 * this is equivalent to an ordinary linear interpolation: <i>dst = (1-t)u + tv </i>.
	 * @param dst
	 * @param u
	 * @param v
	 * @param t
	 * @param metric
	 * @return	dst
	 * @see Rn#linearCombination(double[], double, double[], double, double[]).
	 */
	 public static double[] linearInterpolation(double[] dst, double[] u, double[] v, double t, int metric)	{
		// assert dim check
		double dot = 0.0, angle, s0 = 0.0, s1= 0.0, s2;
		if (dst == null) dst = new double[u.length];
		double[] uu = new double[dst.length];
		double[] vv = new double[dst.length];
		int realmetric = metric;		// points in hyperbolic metric can behave like elliptic points

		if (metric != EUCLIDEAN)	{
			normalize(uu, u, metric);
			normalize(vv, v, metric);			
		} else {
			uu = u;  vv = v;
		}
		
		// find the proper factors s0 and s1 to perform a traditional linear combination
		switch(metric)	{
			case PROJECTIVE:
			case EUCLIDEAN:
				s0 = 1-t;
				s1 = t;
				break;
			case HYPERBOLIC:
				dot = innerProduct(uu,vv,metric);
				if (Math.abs(dot) <= 1.0)	realmetric = Pn.ELLIPTIC;
				break;
			case ELLIPTIC:
				dot = innerProduct(uu,vv,metric);
				if (dot > 1.0) dot = 1.0;
				if (dot < -1.0) dot = -1.0;
				break;
		}
		if (realmetric == Pn.ELLIPTIC)	{
			angle = Math.acos(dot);
			s2 = Math.sin(angle);
			if (s2 != 0.0)	{
				s0 = Math.sin((1-t) * angle)/s2;
				s1 = Math.sin(t*angle) /s2;
			} else { s0 = 1.0; s1 = 0.0; }			
		} else if (realmetric == Pn.HYPERBOLIC)	{
			angle = acosh(dot);
			s2 =  sinh(angle);
			if (s2 != 0.0)	{
				s0 = sinh((1-t) * angle)/s2;
				s1 = sinh(t*angle) /s2;
			}	else { s0 = 1.0; s1 = 0.0; }				
			
		}
		if (uu == null || vv == null) 
			throw new NullPointerException();
		return Rn.linearCombination(dst, s0, uu, s1, vv);
	}
	 
	/**
	 * Construct a central projectivity with fixed point center and fixed plane axis.
	 * @param dst
	 * @param center
	 * @param axis
	 * @param metric
	 * @return
	 */
	public static double[] makeHarmonicHarmology(double[] dst, double[] center, double[] axis){ 
		return makeGeneralizedProjection(dst, center, axis, -1);
	}
	
	/**
	 * Similar to {@link P3#makeHarmonicHarmology(double[], double[], double[])} but maps all points
	 * onto the <i>axis</i> plane.
	 * @param dst
	 * @param center
	 * @param axis
	 * @return
	 */
	public static double[] makeFlattenProjection(double[] dst, double[] center, double[] axis)	{
		return makeGeneralizedProjection(dst, center, axis, 0 );
	}
	
/**
 * Create a projectivity that leaves center <i>C</i> invariant (planewise), axis <i>A</i> invariant (point-wise) and
 * otherwise moves a general point P along the line <i>l</i> through P and the center depending on  
 * <i>val</i>. To be exact, let Q be the intersection of <i>l</i> with the axis.  Define projective
 * coordinates on <i>l</i> by setting <i>Q,P,C</i> to 0,1,infinity  respectively. Then the projective coordinate
 * of <i>f(P)</i> is <i>val</i>.  A value of 1 gives the identity, of 0 gives the projection onto the axis,
 * and of infinity gives projection onto the center.
 * @param dst
 * @param center
 * @param axis
 * @param val
 * @return
 */
	public static double[] makeGeneralizedProjection(double[] dst, double[] center, double[] axis, double val) {
		if (center.length != axis.length)
			throw new IllegalArgumentException("center and axis must have same length");
		int n = center.length;
		if (dst == null) dst = new double[n*n];
	     double f = 1.0/Rn.innerProduct(center, axis); 
	     double val1 = 0;
	     //if (val!=0) 
	     	val1 = val-1;// (1-val)/val; 
	     for (int i = 0; i<n; ++i)  {    
	         for (int j = 0; j<n; ++j) {
	              //if (val != 0) 
	            	  	dst[n*i+j] = (i==j? 1 : 0) +  val1 * f * center[i] * axis[j];
	              //else dst[n*i+j] = f*center[i] * axis[j];
	         }
	     }
	     return dst; 		
	}
	 /**
	  * Find the plane which lies, metrically, half-way between the two given planes.
	  * @param midp
	  * @param pl1
	  * @param pl2
	  * @param metric
	  * @return
	  */
	 public static double[] midPlane(double[] midp, double[] pl1, double[] pl2, int metric)	{
		if (midp == null)	midp = new double[4];
		double[] pt1, pt2;
		pt1 = normalizePlane(null,pl1, metric);
		pt2 = normalizePlane(null,pl2, metric);
		linearInterpolation(midp, pt1, pt2, .5, metric);
		return midp;
	}
	 
	 /**
	 * Calculates the norm of the vector in the given metric <i>metric</i>.  Actually returns the absolute 
	 * value of the norm, since hyperbolic points can have imaginary norm.
	 * @param src
	 * @param metric
	 * @return	the norm
	 */
	 public static double norm(double[] src, int metric)	{
		return Math.sqrt(Math.abs(innerProduct(src,src,metric)));
	}
	
	/**
	 * Normalize a point-tangent pair <i>(src, svec)</i> so that both members have unit length
	 * and, the tangent vector lies in the polar plane of the point.
	 * @param dst
	 * @param dvec
	 * @param src
	 * @param svec
	 * @param metric
	 * @return
	 */
	 public static double[] normalize(double[] dst, double[] dvec, double[] src, double[] svec, int metric)	{
	 	if (dst == null)		dst = new double[src.length];
	 	if (metric == EUCLIDEAN)	{
	 		dehomogenize(dst, src);
	 		dehomogenize(dvec, svec);
	 		dvec[dvec.length-1] = 0.0;
	 		return dst;
	 	}
	 	Pn.normalize(dst, src, metric);
	 	dvec = projectToTangentSpace(dvec, dst, svec, metric);
	 	normalize(dvec, dvec, metric);
	 	return dst;
	 }
	
	/**
	 * Normalizes the vector <i>src</i> to have unit length (either 1 or <i>i</i>), or, 
	 * if <i>metric</i> 
	 * is EUCLIDEAN, then the input vector is dehomogenized.  
	 * To set a Euclidean vector to length 1, use
	 * {@link #setToLength(double[], double[], double, int)}.  
	 * This method is only
	 * valid for homogeneous coordinates. Use {@link de.jreality.math.Rn#normalize(double[], double[]) 
	 * if you are using dehomogenous coordinates (e.g., representing points in R3 using 3-vectors). \
	 * 
	 * @param dst
	 * @param src
	 * @param metric
	 * @return	dst
	 */
	 public static double[] normalize(double[] dst, double[] src, int metric)	{
	 	if (dst == null) dst = new double[src.length];
	 	if (metric == EUCLIDEAN) return dehomogenize(dst, src);
	 	return setToLength(dst, src, 1.0, metric);
	 }

	 /**
	  * A vectorized version of {@link #normalize(double[], double[], int)}.
	  * @param dst
	  * @param src
	  * @param metric
	  * @return
	  */
	 public static double[][] normalize(double[][] dst, double[][] src, int metric)	{
	 	if (dst == null)	dst = new double[src.length][src[0].length];
		if (dst.length != src.length)	{
			throw new IllegalArgumentException("Incompatible lengths");
		}
	 	int n = dst.length;
	 	for (int i = 0; i<n; ++i)	{
	 	  	normalize(dst[i], src[i], metric);
	 	}
	 	return dst;
	 }
	 

	/**
	 * For the euclidean special case:
	 * Normalize a hyper-plane (represented as a vector of length n) so that the direction vector 
	 * (the first n-1 coordinates) has 
	 * euclidean length 1 but represents the same projective hyper-plane. Otherwise normalize to have unit length;
	 * @param dst
	 * @param src
	 * @return
	 */
	public static double[]  normalizePlane(double[]  dst, double[]  src, int metric)	{
		if (metric != EUCLIDEAN)	{
			return normalize(dst, src, metric);
		}
		int n = src.length;
		if (dst == null) dst = new double[n];
		double[] foo = new double[n-1];
		System.arraycopy(src, 0, foo, 0, n-1);
		double ll = Rn.euclideanNorm(foo);
		if (ll == 0) return null;
		Rn.times(dst,1.0/ll, src);
		
		return dst;
	}
	
	/**
	 * An alias for {@link #normalize(double[], double[], int)}.
	 * @param dst
	 * @param src
	 * @param metric
	 * @return
	 */
	public static double[] normalizePoint(double[] dst, double[] src, int metric){
	 	return normalize(dst, src,metric);
	 }
	
	/**
	 * @param src
	 * @param metric
	 * @return	square of norm
	 */
	public static double normSquared(double[] src, int metric)	{
		return innerProduct(src, src, metric);
	}

	/**
	 * Polarize the input element <i>p</i> with respect to the quadradic form associated to
	 *  <i>metric</i>.  Note: in the euclidean case, use {@link #polarizePlane(double[], double[], int)} or
	 *  {@link #polarizePlane(double[], double[], int)}.  
	 * @param polar
	 * @param p
	 * @param metric
	 * @return
	 */
	public static double[] polarize(double[] polar, double[] p, int metric)	{
		if (polar == null)	polar = (double[]) p.clone();
		else System.arraycopy(p,0,polar, 0, p.length);
		// last element is multiplied by the metric!
		switch (metric)	{
			case Pn.ELLIPTIC:
				// self-polar!
				break;
			case Pn.EUCLIDEAN:
				polar[polar.length - 1]  = 0d;
				break;
			case Pn.HYPERBOLIC:
				polar[polar.length-1] *= -1;
				break;
		}
		return polar;
	}
	
	/**
	 * A vectorized version of {@link #polarize(double[], double[], int)}.
	 * @param polar
	 * @param p
	 * @param metric
	 * @return
	 */public static double[][] polarize(double[][] polar, double[][] p, int metric)	{
		if (polar == null)	polar = new double[p.length][];
		if (polar.length != p.length)	 throw new IllegalArgumentException("dst has invalid length");
		int n = p.length;
		for (int i = 0; i<n; ++i)	{
			polar[i] = polarize(polar[i], p[i], metric);
		}
		return polar;
	}

	/**
	 * This just calls {@link #polarize(double[], double[], int)} since the polar plane of
	 * a point is just Q.point, where Q is the absolute quadric
	 * 
	 * @param dst
	 * @param ds
	 * @param metric
	 * @return
	 */public static double[] polarizePlane(double[] dst, double[] plane, int metric) {
		return polarize(dst, plane, metric);
	}

	/**
	 * This has to handle the exception euclidean case, that the polar of any point is
	 * the ideal plane. Otherwise it just returns {@link #polarize(double[], double[], int)}.
	 * @param object
	 * @param ds
	 * @param metric
	 * @return
	 */
	public static double[] polarizePoint(double[] dst, double[] point, int metric) {
		if (metric == Pn.EUCLIDEAN)	{
			// there is only one polar plane: the plane at infinity
			if (dst == null) dst = new double[point.length];
			for (int i = 0; i< (dst.length-1); ++i) dst[i] = 0.0;
			dst[dst.length-1] = -1.0;
			return dst;
		}
		return polarize(dst, point, metric);
	}
	
	/**
	 * Determine the projection of the point <i>victim</i> onto the point <i>master</i>.  
	 * This is orthogonal projection with respect to the metric associated to <i>metric</i>.
	 * @param result
	 * @param master
	 * @param victim
	 * @param metric
	 * @return
	 */
	public static double[] projectOnto(double[] result, double[] master, double[] victim, int metric)	{
		if (master.length != victim.length)	{
			throw new IllegalArgumentException("Arguments must be same dimension");
		}
		int n = master.length;
		if (result == null) result = new double[n];
//		if (metric == EUCLIDEAN)	{
//			// set the last coordinate to 0: becomes a direction
//			polarizePlane(result, victim, metric);
//			return result;
//		}
		double factor = innerProductPlanes(master, victim, metric);
		double pp = innerProductPlanes(master, master, metric);
		if (pp != 0.0)	factor = factor / pp;
		Rn.times(result, factor, master );
		return result;
	}
	
	/**
	 * Project <i>victim</i> onto the othogonal complement of <i>master</i> (all with respect to the
	 * given metric). See {@link #projectOnto(double[], double[], double[], int)}.
	 * @param result
	 * @param master
	 * @param victim
	 * @param metric
	 * @return
	 */
	public static double[] projectOntoComplement(double[] result, double[] master, double[] victim, int metric)	{
		return Rn.subtract(result, victim, projectOnto(null, master, victim, metric));
	}
	
	/**
	 * @param result
	 * @param point
	 * @param tangentToBe
	 * @param metric
	 * @return
	 */
	public static double[] projectToTangentSpace(double[] result, double[] point, double[] tangentToBe, int metric)	{
		 // TODO rephrase this method in terms of the following two
		int n = point.length;
		if (result == null) result = new double[n];
		if (metric == EUCLIDEAN)	{
			polarizePlane(result, tangentToBe, metric);
			return result;
		}
		double factor = innerProduct(point, tangentToBe, metric);
		double pp = innerProduct(point, point, metric);
		if (pp != 0.0)	factor = factor / pp;
		Rn.subtract(result, tangentToBe, Rn.times(null, factor, point ));
		return result;
	} 
	
	/**
	 * Create a multiple of <i>src</i> with has length <i>length</i>.
	 * @param dst
	 * @param src
	 * @param length
	 * @param metric
	 * @return
	 */public static double[] setToLength(double[] dst, double[] src, double length, int metric)	{
		// assert dim checks
		if (dst == null) dst = new double[src.length];
	 	if (dst.length != src.length)	
	 		throw new IllegalArgumentException("Incompatible lengths");
		if (metric == EUCLIDEAN)	dehomogenize(dst, src);
		else					System.arraycopy(src,0,dst,0,dst.length);
		
		double ll = norm(dst, metric);
		if (ll == 0.0)	return dst;
		ll = length/ll;
		Rn.times(dst, ll, dst);
		if (metric == EUCLIDEAN && dst[dst.length-1] != 0.0) 	dst[dst.length-1] = 1.0;
		return dst;
	}

	/**
	 * A vectorized version of {@link #setToLength(double[], double[], double, int)}.
	 * @param verts
	 * @param verts2
	 * @param d
	 * @param euclidean2
	 */
	public static double[][] setToLength(double[][] dst, double[][] src, double d, int metric) {
		// assert dim checks
		int sl = src.length;
		if (dst == null) dst = new double[sl][src[0].length-1];
		int dl = dst.length;
		if (dl != sl)	{
			throw new IllegalArgumentException("Incompatible lengths");
		}
		for (int i = 0; i<sl; ++i)	setToLength(dst[i], src[i], d, metric);
		return dst;
	}
	
	// calculate the weights required to write unit as a linear combination of tri
	public static double[] barycentricCoordinates(double[] dst, double[][] tri, double[] unit)	{
		int n = tri[0].length;
		double[] mat = new double[n*n];
		for (int i = 0; i<n; ++i)	System.arraycopy(tri[i], 0, mat, n*i, n);
		double[] imat = Rn.inverse(null, Rn.transpose(null, mat));
		return Rn.matrixTimesVector(dst, imat, unit);
	}


}		
