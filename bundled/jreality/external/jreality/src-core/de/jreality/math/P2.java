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

import java.util.logging.Level;

import de.jreality.util.LoggingSystem;

/**
 * Static methods for geometry of the real projective plane RP2.
 * 
 * {@see de.jreality.math.Rn}  for method conventions and discussion of the ubiquitous <i>metricnature</i> parameter.
 * {@see de.jreality.math.Pn}  for other methods applicable in n-dimensional projective space.
 * @author Charles Gunn
 *
 */
final public class P2 {

  private P2() {}

	/**
	 * Calculate the Euclidean perpendicular bisector of the segment from <i>p1</i> to <i>p2</i>.
	 * @param dst
	 * @param p1
	 * @param p2
	 * @return
	 */
	private static double[] perpendicularBisector(double[] dst, double[] p1, double[]p2)	{
		if (p1.length != 3 || p2.length != 3)	{
			throw new IllegalArgumentException("Input points must be homogeneous vectors");
		}
		if (dst == null) dst = new double[3];
		double[] avg = new double[3];
		Rn.add(avg,p1,p2);
		Rn.times(avg, .5, avg);
		double[] line = new double[3];
		lineFromPoints(line, p1, p2);
		dst[0] = -line[1];
		dst[1] = line[0];
		dst[2] = -(dst[0]*avg[0] + dst[1]*avg[1]);
		return dst;
	}
	
	/**
	 * Calculate the perpendicular bisector of the segment <i>p1</i> and <i>p2</i> with metricnature <i>metricnature</i>
	 * @param dst
	 * @param p1
	 * @param p2
	 * @param metric
	 * @return
	 */
	public static double[] perpendicularBisector(double[] dst, double[] p1, double[]p2, int metric)	{
		if (p1.length != 3 || p2.length != 3)	{
			throw new IllegalArgumentException("Input points must be homogeneous vectors");
		}
		if (metric == Pn.EUCLIDEAN) return perpendicularBisector(dst, p1, p2);
		if (dst == null) dst = new double[3];
		double[] midpoint = new double[3];
		Pn.linearInterpolation(midpoint,p1,p2, .5, metric);
		double[] line = lineFromPoints(null, p1, p2);
		double[] polarM = Pn.polarize(null, midpoint, metric);
		double[] pb = pointFromLines(null, polarM, line);
		Pn.polarize(dst, pb, metric);
		if (Rn.innerProduct(dst,p1) < 0)	Rn.times(dst, -1.0, dst);
		return dst;
	}
	
	/**
	 * Calculate the homogeneous coordinates of the point of intersection  of the two  lines <i>l1</i> and <i>l2</i>.
	 * @param point
	 * @param l1
	 * @param l2
	 * @return
	 */
	public static double[] pointFromLines(double[] point, double[] l1, double[] l2)	{
		if (l1.length < 3 || l2.length < 3)	{
			throw new IllegalArgumentException("Input arrays too short");
		}
		if (point == null) point = new double[3];	
		point[0] = l1[1]*l2[2] - l1[2]*l2[1];
		point[1] = l1[2]*l2[0] - l1[0]*l2[2];
		point[2] = l1[0]*l2[1] - l1[1]*l2[0];
		return point;
	}
	
	/**
	 * Calculate the line coordinates of the line connecting the two points <i>p1</i> and <i>p2</i>.
	 * @param point
	 * @param l1
	 * @param l2
	 * @return
	 */
	public static double[] lineFromPoints(double[] line, double[] p1, double[] p2)	{
		return pointFromLines(line, p1, p2);
	}
		
	public static double[] normalizeLine(double[] dst, double[] src)	{
		if (dst == null) dst = new double[3];
		double nn = Rn.innerProduct(src, src, 2);
		nn = Math.sqrt(nn);
		if (nn == 0) {System.arraycopy(src, 0, dst, 0, 3); return dst; }
		nn = 1.0/nn;
		Rn.times(dst, nn, src);
		return dst;
	}
	/**
	 * Returns true if and only if <i>point</i> is  within the polygon determined by the
	 * points contained in the array <i>polygon</i>.
	 * @param polygon
	 * @param point
	 * @return
	 */
	public static boolean polygonContainsPoint(double[][] polygon, double[] point)	{
		return polygonContainsPoint(polygon, null, point);
	}
	
	public static boolean polygonContainsPoint(double[][] polygon, boolean[] open, double[] point)	{
//		if (point.length != 3)	{
//			throw new IllegalArgumentException("Input point must be homogeneous vector");
//		}
//		double metricn = 0.0;
//		int n = polygon.length, j;
//		double[] p1 = new double[3];
//		double[] p2 = new double[3];
//		double[] tmp;
//		p1[2] = p2[2] = 1.0;
//		p1[0] = polygon[0][0]; p1[1] = polygon[0][1];
//		for (int i = 0; i<n; ++i)	{
//			j = (i+1) % n;
//			p2[0] = polygon[j][0]; p2[1] = polygon[j][1];
//			double[] line = lineFromPoints(null, p1, p2);
//			double ip = Rn.innerProduct(line, point);
//			if (metricn == 0.0) metricn = ip;
//			else if (metricn * ip < 0.0) return false;
//			tmp = p1;
//			p1 = p2;
//			p2 = tmp;
//		}
//		return true;
		return getFirstOutsideEdge(polygon, open, point) == -1;
	}
	
	public static int getFirstOutsideEdge(double[][] polygon, boolean[] open, double[] point)	{
		if (point.length != 3)	{
			throw new IllegalArgumentException("Input point must be homogeneous vector");
		}
		double metricn = 0.0;
		int n = polygon.length, j;
		double[] p1 = new double[3];
		double[] p2 = new double[3];
		double[] tmp;
		p1[2] = p2[2] = 1.0;
		p1[0] = polygon[0][0]; p1[1] = polygon[0][1];
		double min = 10E10;
		int which = -1;
		for (int i = 0; i<n; ++i)	{
			j = (i+1) % n;
			p2[0] = polygon[j][0]; p2[1] = polygon[j][1];
			double[] line = lineFromPoints(null, p1, p2);
			double ip = Rn.innerProduct(line, point);
//			if (metricn == 0.0) metricn = ip;
//			else if (metricn * ip < 0.0) return i;
			if (ip < min) {which = i; min = ip;}
			tmp = p1;
			p1 = p2;
			p2 = tmp;
		}
		if (open != null && open[which]) {		// boundary line NOT included in polygon
			if (min <= 0.0) return which;
		} else if (min < 0.0) return which;		// boundary line included in polygon
		return -1;
	}
	

	/**
	 * Returns true if and only if the polygon described by the point series <i>polygon</i> is convex.
	 * @param polygon
	 * @return
	 */
	 public static boolean isConvex(double[][] polygon)	{
		int n = polygon.length, j;
		double metricn = 0.0;
		double[][] diffs = new double[n][polygon[0].length];
		for (int i = 0; i<n; ++i)	{
			j = (i+1) % n;
			Rn.subtract(diffs[i], polygon[j], polygon[i]);
			Rn.normalize(diffs[i], diffs[i]);
		}
		double[] p1 = new double[3];
		double[] p2 = new double[3];
		double[] tmp = new double[3];
		p1[2] = p2[2] = 1.0;
		p1[0] = polygon[0][0]; p1[1] = polygon[0][1];
		for (int i = 0; i<n; ++i)	{
			j = (i+1) % n;
			Rn.crossProduct(tmp, diffs[i],diffs[j]);
			if (metricn == 0.0)	metricn = tmp[2];
			else if (metricn * tmp[2] < 0.0) return false;
		}
		
		return true;
	}
	/**
	 * The assumption is that the line is specified in such a way that vertices to be cut away
	 * have a negative inner product with the line coordinates.  The result is a new point array that
	 * defines the polygon obtained by cutting off all points with negative inner product with the
	 * given <i>line</i>.  The polygon is assumed  to be convex.
	 * @param polygon
	 * @param line
	 * @return
	 */
	public static double[][] chopConvexPolygonWithLine(double[][] polygon, double[] line)	{
		if (line.length != 3 )	{
			throw new IllegalArgumentException("Input line must be homogeneous vectors");
		}
		if (polygon == null) return null;
		int n = polygon.length;
		
		double[] center = new double[3];
		Rn.average(center, polygon);
		boolean noNegative = true;
		
		double[] vals = new double[n];
		int count = 0;
		for (int i = 0; i<n; ++i)	{
			vals[i] = Rn.innerProduct(line, polygon[i]);
			if (vals[i] >= 0) count++;
			else noNegative = false;	
			}
		if (count == 0)		{
			LoggingSystem.getLogger(P2.class).log(Level.FINE, "chopConvexPolygonWithLine: nothing left");
			return null;
		} else if (count == n || noNegative)	{
			return polygon;
		}
		double[][] newPolygon = new double[count+2][3];
		double[] tmp = new double[3];
		count = 0;
		for (int i = 0; i<n; ++i)	{
			if (vals[i] >= 0) 	System.arraycopy(polygon[i],0,newPolygon[count++],0,3);
			if (count >= newPolygon.length) break;
			if (vals[i] * vals[(i+1)%n] < 0)	{
				double[] edge = new double[3];
				lineFromPoints(edge, polygon[i], polygon[(i+1)%n]);
				pointFromLines(tmp,edge,line);
				Pn.dehomogenize(newPolygon[count],tmp);
				count++;
			} 
			if (count >= newPolygon.length) break;
		}
		if (count != newPolygon.length) {
			double[][] newPolygon2 = new double[count][];
			System.arraycopy(newPolygon, 0, newPolygon2,0,count);
			return newPolygon2;
		}
		return newPolygon;
	}
	
	/**
	 * Generate a direct isometry that carries the frame determined by <i>p0</i> and <i>p1</i> to that determined
	 * by <i>q0</i> and <i>q1</i>.  See {@link #makeDirectIsometryFromFrame(double[], double[], double[], int)}.
	 * @param dst
	 * @param p0
	 * @param p1
	 * @param q0
	 * @param q1
	 * @param signature
	 * @return 
	 */
	public static double[] makeDirectIsometryFromFrames(double[] dst, double[] p0, double[] p1, double[] q0, double[] q1, int signature) {
		double[] toP = makeDirectIsometryFromFrame(null, p0, p1, signature);
		double[] toQ = makeDirectIsometryFromFrame(null, q0, q1, signature);
		double[] iToP = Rn.inverse(null,toP);
		dst = Rn.times(dst, toQ, iToP);
		return dst;
	}

	/**
	 * Generate a direct isometry which maps the frame <i>F</i> determined by <i>point</i> and <i>xdir</i> to the 
	 * standard frame represented by the identity matrix.  <i>F</i> is the frame based at <i>point</i>
	 * whose whose tangent space is spanned by a unit tangent vector in the direction of <i>xdir</i>, and 
	 * a second tangent vector orthogonal to both <i>point</i> and <i>xdir</i>.
	 * @param dst
	 * @param point
	 * @param xdir
	 * @param metric
	 * @return
	 */
	public static double[] makeDirectIsometryFromFrame(double[] dst, double[] point,
			double[] xdir, int metric) {
		if (dst == null) dst = new double[9];
		Pn.normalize(point, point, metric);
		double[] p2 = null, p1n = null;
		if (metric == Pn.EUCLIDEAN)	{ // handle it special since the code is fragile
			p1n = xdir.clone();
			if (p1n[2] != 0)	{	// have to convert to free vector
				Pn.dehomogenize(p1n, p1n);
				Rn.subtract(p1n, p1n, point);	// now it should have w-coordinate = 0
			}
			Rn.normalize(p1n, p1n);		// normalize wrt euclidean VS metric
			p2 = new double[]{-p1n[1], p1n[0], 0};  // rotate clockwise by 90 degrees
		} else {	
			double[] polarP = Pn.polarize(null, point, metric);
			double[] lineP = lineFromPoints(null, point, xdir);
			// the following is not reliable in the euclidean case: sometimes produces flipped result
			p1n = Pn.normalize(null, pointFromLines(null, polarP, lineP), metric);
			p2 = Pn.polarize(null, lineP, metric);
			Pn.normalize(p2, p2, metric);			
		}
		makeMatrixFromColumns(dst, p1n, p2, point);
		return dst;
	}
	
	private static double[] makeMatrixFromColumns(double[] dst, double[] p0, double[] p1, double[] p2) {
		if (dst == null) dst = new double[9];
		double[][] ptrs = {p0, p1, p2};
		for (int i = 0; i<3; ++i)	{
			for (int j = 0; j<3; ++j)	{
				dst[3*i+j] = ptrs[j][i];
			}
		}
		return dst;
	}
	/**
	 * Convert the input (x,y,z,w) into (x,y,w).
	 * @param vec3
	 * @param vec4
	 * @return
	 */public static double[] projectP3ToP2(double[] vec3, double[] vec4)	{
		double[] dst;
		if (vec3 == null)	dst = new double[3];
		else dst = vec3;
		dst[0] = vec4[0];
		dst[1] = vec4[1];
		dst[2] = vec4[3];
		return dst;
	}
	
	/**
	 * Convert (x,y,z) into (x,y,0,z)
	 * @param vec4
	 * @param vec3
	 * @return
	 */
	 public static double[] imbedP2InP3(double[] vec4, double[] vec3)	{
		double[] dst;
		if (vec4 == null)	dst = new double[4];
		else dst = vec4;
		dst[0] = vec3[0];
		dst[1] = vec3[1];
		dst[2] = 0.0;
		dst[3] = vec3[2];
		return dst;
	}

	 private static int[] which = {0,1,3};
	 public static double[] imbedMatrixP2InP3(double[] dst, double[] m3)	{
			if (dst == null)	dst = new double[16];
			for (int i = 0; i<3; ++i)	{
				int i4 = which[i];
				for (int j = 0; j<3; ++j)	{
					int j4 = which[j] + 4 * i4;
					int j3 = i*3 + j;
					dst[j4] = m3[j3];
				}
			}
			dst[2] = dst[6] = dst[8] = dst[9] = dst[11] = 0.0; dst[10] = 1.0;
			return dst;
		}


}
