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

import java.util.ArrayList;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;

import de.jreality.math.Matrix;
import de.jreality.math.P3;
import de.jreality.math.Pn;
import de.jreality.math.Rn;
import de.jreality.scene.Cylinder;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.IndexedLineSet;
import de.jreality.scene.PointSet;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.Sphere;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.DataList;
import de.jreality.scene.data.DoubleArray;
import de.jreality.scene.data.DoubleArrayArray;
import de.jreality.scene.data.IntArray;
import de.jreality.scene.data.IntArrayArray;

// TODO: currently 3vectors (from, to) are NOT supported

/**
 * TODO: get rid of the maxDist parameter, use the from and to vectors as the
 * endpoints of the segment of valid pick-points
 */
public class BruteForcePicking {

	public static void intersectPolygons(IndexedFaceSet ifs, int metric,
			SceneGraphPath path, Matrix m, Matrix mInv, double[] from,
			double[] to, ArrayList<Hit> hits) {

		double[] fromLocal = mInv.multiplyVector(from);
		double[] toLocal = mInv.multiplyVector(to);
		double[] bary = new double[3];
		double[] p1 = new double[4], p2 = new double[4], p3 = new double[4], pobj = new double[4];
		p1[3] = p2[3] = p3[3] = 1;
		IntArrayArray faces = getFaces(ifs);
		DoubleArrayArray points = getPoints(ifs);
		if (faces == null || points == null || faces.getLength() == 0
				|| points.getLength() == 0)
			return;

		for (int i = 0, n = faces.getLength(); i < n; i++) {
			IntArray face = faces.getValueAt(i);
			// simple triangulation:
			for (int j = 0, dd = face.getLength() - 2; j < dd; j++) {
				p1 = points.getValueAt(face.getValueAt(0)).toDoubleArray(p1);
				p2 = points.getValueAt(face.getValueAt(1 + j))
						.toDoubleArray(p2);
				p3 = points.getValueAt(face.getValueAt(2 + j))
						.toDoubleArray(p3);

				if (intersects(pobj, fromLocal, toLocal, p1, p2, p3, bary)) {
					double[] pw = m.multiplyVector(pobj);
					double d = Pn.distanceBetween(from, pw, metric);
					double affCoord = P3.affineCoordinate(from, to, pw);
					hits.add(new Hit(path.pushNew(ifs), pw, d, affCoord, bary,
							PickResult.PICK_TYPE_FACE, i, j));
					// System.err.println("polygon hit");
				}

			}
		}

	}

	public static boolean intersects(double[] pobj, double[] fromLocal,
			double[] toLocal, double[] p1, double[] p2, double[] p3, double[] bary) {
		double[] plane = P3.planeFromPoints(null, p1, p2, p3);
		pobj = P3.lineIntersectPlane(pobj, fromLocal, toLocal, plane);
		// if(pobj[3]*pobj[3]<Rn.TOLERANCE) return false; // parallel

		Pn.dehomogenize(p1, p1);
		Pn.dehomogenize(p2, p2);
		Pn.dehomogenize(p3, p3);

		if (bary == null) bary = new double[3];
		if (!Hit.convertToBary(bary, p1, p2, p3, pobj))
			return false;
		if (((bary[0] < 0 || bary[0] > 1) || (bary[1] < 0 || bary[1] > 1) || (bary[2] < 0 || bary[2] > 1))
				|| ((bary[0] + bary[1] + bary[2] - 1)
						* (bary[0] + bary[1] + bary[2] - 1) > Rn.TOLERANCE)) {
			return false;
		}
		// if (Rn.innerProduct(pobj, p1) < 0) {
		// Rn.times(pobj,-1,pobj);
		// System.err.println("Reversing pick");
		// }
		return true;
	}

	public static void intersectEdges(IndexedLineSet ils, int metric,
			SceneGraphPath path, Matrix m, Matrix mInv, double[] from,
			double[] to, double tubeRadius, ArrayList<Hit> localHits) {

		double[] fromOb = mInv.multiplyVector(from);
		double[] toOb = mInv.multiplyVector(to);

		double[] fromOb3 = new double[3];
		double[] toOb3 = new double[3];
		double[] dirOb3 = new double[3];
		if (from.length > 3) {
			Pn.dehomogenize(fromOb3, fromOb);
			Pn.dehomogenize(toOb3, toOb);
			if (toOb[3] == 0) {
				dirOb3 = toOb3;
			} else {
				Rn.subtract(dirOb3, toOb3, fromOb3);
			}
		} else {
			Rn.subtract(dirOb3, toOb3, fromOb3);
		}

		IntArrayArray edges = getEdges(ils);
		DoubleArrayArray points = getPoints(ils);
		if (points == null || edges == null || edges.getLength() == 0
				|| points.getLength() == 0)
			return;

		IntArray edge = edges.getValueAt(0);

		DoubleArray point = points.getValueAt(0);
		boolean vec3 = point.getLength() == 3;
		double[] vertex1 = new double[3];
		double[] vertex2 = new double[3];
		double[] vecRaw1 = vec3 ? null : new double[4];
		double[] vecRaw2 = vec3 ? null : new double[4];

		LinkedList<double[]> MY_HITS = new LinkedList<double[]>();
		DoubleArray edgeRadii = getRadii(ils);
		int mm = edges.getLength();
		for (int i = 0; i < mm; i++) {
			edge = edges.getValueAt(i);
			double realRad = tubeRadius;
			if (edgeRadii != null)
				realRad = realRad * edgeRadii.getValueAt(i);
			for (int j = 0, n = edge.getLength() - 1; j < n; j++) {
				if (vec3) {
					points.getValueAt(edge.getValueAt(j))
							.toDoubleArray(vertex1);
					points.getValueAt(edge.getValueAt(j + 1)).toDoubleArray(
							vertex2);
				} else {
					points.getValueAt(edge.getValueAt(j))
							.toDoubleArray(vecRaw1);
					points.getValueAt(edge.getValueAt(j + 1)).toDoubleArray(
							vecRaw2);
					if (vecRaw1[3] == 0) {
						Rn.linearCombination(vecRaw1, .99, vecRaw1, .01,
								vecRaw2);
					} else if (vecRaw2[3] == 0) {
						Rn.linearCombination(vecRaw2, .99, vecRaw2, .01,
								vecRaw1);
					}
					Pn.dehomogenize(vertex1, vecRaw1);
					Pn.dehomogenize(vertex2, vecRaw2);
				}
				intersectCylinder(MY_HITS, m, fromOb3, dirOb3, vertex1,
						vertex2, tubeRadius);
				for (Iterator it = MY_HITS.iterator(); it.hasNext();) {
					double[] hitPoint = (double[]) it.next();
					it.remove();
					double dist = Rn.euclideanNorm(Rn.subtract(null, hitPoint,
							from));
					// TODO: pass index j (index in the edge) to the Hit?
					double affCoord = P3.affineCoordinate(from, to, hitPoint);
					Hit h = new Hit(path.pushNew(ils), hitPoint, dist,
							affCoord, null, PickResult.PICK_TYPE_LINE, i, j);
					localHits.add(h);
				}
			}
		}
	}

	private static DoubleArrayArray getPoints(PointSet ils) {
		DataList dl = ils.getVertexAttributes(Attribute.COORDINATES);
		if (dl == null)
			return null;
		return dl.toDoubleArrayArray();
	}

	private static DoubleArray getRadii(PointSet ps) {
		DataList dl = ps.getVertexAttributes(Attribute.RELATIVE_RADII);
		if (dl == null)
			return null;
		return dl.toDoubleArray();
	}

	private static DoubleArray getRadii(IndexedLineSet ls) {
		DataList dl = ls.getEdgeAttributes(Attribute.RELATIVE_RADII);
		if (dl == null)
			return null;
		return dl.toDoubleArray();
	}

	private static IntArrayArray getEdges(IndexedLineSet ils) {
		DataList dl = ils.getEdgeAttributes(Attribute.INDICES);
		if (dl == null)
			return null;
		return dl.toIntArrayArray();
	}

	private static IntArrayArray getFaces(IndexedFaceSet ils) {
		DataList dl = ils.getFaceAttributes(Attribute.INDICES);
		if (dl == null)
			return null;
		return dl.toIntArrayArray();
	}

	public static void intersectPoints(PointSet ps, int metric,
			SceneGraphPath path, Matrix m, Matrix mInv, double[] from,
			double[] to, double pointRadius, ArrayList<Hit> localHits) {
		// path.getMatrix(m.getArray());
		// path.getInverseMatrix(mInv.getArray());

		double[] fromOb = mInv.multiplyVector(from);
		double[] toOb = mInv.multiplyVector(to);

		double[] fromOb3 = new double[3];
		double[] toOb3 = new double[3];
		double[] dirOb3 = new double[3];
		if (from.length > 3) {
			Pn.dehomogenize(fromOb3, fromOb);
			Pn.dehomogenize(toOb3, toOb);
			if (toOb[3] == 0) {
				dirOb3 = toOb3;
			} else {
				Rn.subtract(dirOb3, toOb3, fromOb3);
			}
		} else {
			Rn.subtract(dirOb3, toOb3, fromOb3);
		}

		DoubleArrayArray points = getPoints(ps);
		if (points == null || points.getLength() == 0)
			return;
		DoubleArray point = points.getValueAt(0);
		boolean vec3 = point.getLength() == 3;
		double[] vertexRaw = vec3 ? new double[3] : new double[4];
		double[] vertex = new double[3];

		LinkedList<double[]> MY_HITS = new LinkedList<double[]>();
		DoubleArray pointRadii = getRadii(ps);
		for (int j = 0, n = points.getLength(); j < n; j++) {
			if (!vec3) {
				points.getValueAt(j).toDoubleArray(vertexRaw);
				if (vertexRaw[3] == 0)
					continue;
				Pn.dehomogenize(vertex, vertexRaw);
			} else {
				points.getValueAt(j).toDoubleArray(vertex);
			}
			double realRad =  pointRadius;
			if (pointRadii != null)
				realRad = pointRadius * pointRadii.getValueAt(j);
			intersectSphere(MY_HITS, vertex, fromOb3, dirOb3, realRad);
			for (Iterator i = MY_HITS.iterator(); i.hasNext();) {
				double[] hitPoint = (double[]) i.next();
				hitPoint = m.multiplyVector(hitPoint);
				i.remove();
				double dist = Rn.euclideanNorm(Rn
						.subtract(null, hitPoint, from));
				double affCoord = P3.affineCoordinate(from, to, hitPoint);
				Hit h = new Hit(path.pushNew(ps), hitPoint, dist, affCoord, null,
						PickResult.PICK_TYPE_POINT, j, -1);
				localHits.add(h);
			}
		}

	}

	private static final List<double[]> SPHERE_HIT_LIST = new LinkedList<double[]>();

	public static void intersectSphere(Sphere sphere, int metric,
			SceneGraphPath path, Matrix m, Matrix mInv, double[] from,
			double[] to, ArrayList<Hit> localHits) {

		// path.getMatrix(m.getArray());
		// path.getInverseMatrix(mInv.getArray());
		double[] fromOb = mInv.multiplyVector(from);
		double[] toOb = mInv.multiplyVector(to);

		double[] fromOb3 = new double[3];
		double[] toOb3 = new double[3];
		double[] dirOb3 = new double[3];
		if (from.length > 3) {
			Pn.dehomogenize(fromOb3, fromOb);
			Pn.dehomogenize(toOb3, toOb);
			if (toOb[3] == 0) {
				dirOb3 = toOb3;
			} else {
				Rn.subtract(dirOb3, toOb3, fromOb3);
			}
		} else {
			Rn.subtract(dirOb3, toOb3, fromOb3);
		}

		intersectSphere(SPHERE_HIT_LIST, null, fromOb3, dirOb3, 1);

		for (Iterator i = SPHERE_HIT_LIST.iterator(); i.hasNext();) {
			double[] hitPoint = (double[]) i.next();
			hitPoint = m.multiplyVector(hitPoint);
			i.remove();
			double dist = Rn.euclideanNorm(Rn.subtract(null, hitPoint, from));
			double affCoord = P3.affineCoordinate(from, to, hitPoint);
			Hit h = new Hit(path.pushNew(sphere), hitPoint, dist, affCoord, null,
					PickResult.PICK_TYPE_OBJECT, -1, -1);
			localHits.add(h);
		}
	}

	/**
	 * 
	 * @param hits
	 * @param vertex
	 *            null or translation in object coordinates
	 * @param from
	 *            in local coordinates
	 * @param dir
	 *            in local coordinates !!!!!MUST BE NORMALIZED !!!!
	 * @param r
	 *            in local coordinates
	 */
	private static void intersectSphere(List<double[]> hits,
			final double[] vertex, final double[] f, final double[] dir,
			double r) {
		double[] from = f;
		Rn.normalize(dir, dir);
		if (vertex != null)
			from = Rn.subtract(null, from, vertex);

		double b = 2 * Rn.innerProduct(dir, from);
		double c = Rn.euclideanNormSquared(from) - r * r;
		double dis = Math.pow(b, 2) - 4 * c;
		if (dis >= 0) {
			dis = Math.sqrt(dis);
			double t = (-b - dis) / 2;
			for (int i = 0; i < 2; i++) { // avoid duplicate code
				t += i * dis;
				if (t >= 0) {
					double[] hitPointOb3 = new double[3];
					Rn.times(hitPointOb3, t, dir);
					Rn.add(hitPointOb3, hitPointOb3, from); // from+t*dir
					if (vertex != null)
						Rn.add(hitPointOb3, hitPointOb3, vertex);
					double[] hitPoint = new double[4];
					Pn.homogenize(hitPoint, hitPointOb3);
					// hitPoint=m.multiplyVector(hitPoint);
					hits.add(hitPoint);
				}
			}
		}
	}

	// TODO this is not thread safe
	private static final List<double[]> CYLINDER_HIT_LIST = new LinkedList<double[]>();

	public static void intersectCylinder(Cylinder cylinder, int metric,
			SceneGraphPath path, Matrix m, Matrix mInv, double[] from,
			double[] to, ArrayList<Hit> localHits) {
		// path.getMatrix(m.getArray());
		// path.getInverseMatrix(mInv.getArray());

		double[] fromOb = mInv.multiplyVector(from);
		double[] toOb = mInv.multiplyVector(to);

		double[] fromOb3 = new double[3];
		double[] toOb3 = new double[3];
		double[] dirOb3 = new double[3];
		if (from.length > 3) {
			Pn.dehomogenize(fromOb3, fromOb);
			Pn.dehomogenize(toOb3, toOb);
			if (toOb[3] == 0) {
				dirOb3 = toOb3;
			} else {
				Rn.subtract(dirOb3, toOb3, fromOb3);
			}
		} else {
			Rn.subtract(dirOb3, toOb3, fromOb3);
		}

		intersectCylinder(CYLINDER_HIT_LIST, m, fromOb3, dirOb3, new double[] {
				0, 0, 1 }, new double[] { 0, 0, -1 }, 1);
		double[] tmp = new double[from.length];
		for (Iterator i = CYLINDER_HIT_LIST.iterator(); i.hasNext();) {
			double[] hitPoint = (double[]) i.next();
			i.remove();
			double affCoord = P3.affineCoordinate(from, to, hitPoint);
			double dist = Rn.euclideanNorm(Rn.subtract(tmp, hitPoint, from));
			Hit h = new Hit(path.pushNew(cylinder), hitPoint, dist, affCoord, null,
					PickResult.PICK_TYPE_OBJECT, -1, -1);
			localHits.add(h);
		}
	}

	/**
	 * 
	 * @param hits
	 * @param f
	 *            =from in local coordinates
	 * @param d
	 *            =dir in local coordinates !!!!!MUST BE NORMALIZED !!!!
	 * @param v1
	 *            upper point of cylinder-axis
	 * @param v2
	 *            lower point of cylinder-axis
	 * @param r
	 *            in local coordinates, radius
	 */
	private static void intersectCylinder(List<double[]> hits, Matrix m,
			final double[] f, final double[] d, final double[] v1,
			final double[] v2, double r) {

		boolean debug = false;
		long time = System.currentTimeMillis();
		// Methode2:

		double[] from = f;
		double[] dir = d;
		// from=f;
		// dir=d;
		Rn.normalize(dir, dir);
		double[] dir_cyl = Rn.subtract(null, v2, v1);
		Rn.normalize(dir_cyl, dir_cyl);

		double[] dir_cyl_x_dir = Rn.crossProduct(null, dir_cyl, dir);
		double[] from_min_v1 = Rn.subtract(null, from, v1);
		double lambda = Rn.innerProduct(dir_cyl_x_dir,
				Rn.crossProduct(null, from_min_v1, dir_cyl))
				/ Rn.euclideanNormSquared(dir_cyl_x_dir);
		double[] nearest = Rn.add(null, from, Rn.times(null, lambda, dir));
		double dist = Math.abs(Rn.innerProduct(dir_cyl_x_dir, from_min_v1))
				/ Rn.euclideanNorm(dir_cyl_x_dir);

		if (dist <= r) { // && (nearest[0]-from[0])/dir[0]>=0){
			double angle = Math.abs(Rn.euclideanAngle(dir, dir_cyl));
			if (Math.cos(angle) != 0) { // sonst Sehstrahl parallel zu
										// Zylinder-Achse
				if (angle > Math.PI)
					angle = 2 * Math.PI - angle;
				if (angle > Math.PI / 2)
					angle = Math.PI - angle;
				double factor = Math.sqrt(Math.pow(r, 2) - Math.pow(dist, 2))
						/ Math.cos(Math.PI / 2 - angle);
				double maxDist = Math.sqrt(Math.pow(
						Rn.euclideanDistance(v1, v2), 2)
						+ Math.pow(r, 2)); // mx Abstand hit von v1,v2: hitPoint
											// zwischen v1 und v2 auf Cylinder?

				double[] hitPointOb3 = new double[3];
				Rn.times(hitPointOb3, -factor, dir);
				Rn.add(hitPointOb3, hitPointOb3, nearest); // nearest-factor*dir
				// if((hitPointOb3[0]-from[0])/dir[0]>=0 &&
				// (Rn.euclideanDistance(hitPointOb3,v1)<maxDist &&
				// Rn.euclideanDistance(hitPointOb3,v2)<maxDist)){ //vor oder
				// hinter from && hitPoint zwischen v1 und v2 auf Cylinder
				if ((Rn.euclideanDistance(hitPointOb3, v1) < maxDist && Rn
						.euclideanDistance(hitPointOb3, v2) < maxDist)) { // vor
																			// oder
																			// hinter
																			// from
																			// &&
																			// hitPoint
																			// zwischen
																			// v1
																			// und
																			// v2
																			// auf
																			// Cylinder

					if (debug) {
						System.out.println("Methode2: cylinder matched_1");
						System.out.println("Methode2: hitPointOb: "
								+ hitPointOb3[0] + ", " + hitPointOb3[1] + ", "
								+ hitPointOb3[2]);
					}

					double[] hitPoint = new double[4];
					Pn.homogenize(hitPoint, hitPointOb3);
					hitPoint = m.multiplyVector(hitPoint);
					hits.add(hitPoint);
				}

				Rn.times(hitPointOb3, factor, dir);
				Rn.add(hitPointOb3, hitPointOb3, nearest); // nearest+factor*dir
				// if((hitPointOb3[0]-from[0])/dir[0]>=0 &&
				// (Rn.euclideanDistance(hitPointOb3,v1)<maxDist &&
				// Rn.euclideanDistance(hitPointOb3,v2)<maxDist)){ //vor oder
				// hinter from && hitPoint zwischen v1 und v2 auf Cylinder
				if ((Rn.euclideanDistance(hitPointOb3, v1) < maxDist && Rn
						.euclideanDistance(hitPointOb3, v2) < maxDist)) { // vor
																			// oder
																			// hinter
																			// from
																			// &&
																			// hitPoint
																			// zwischen
																			// v1
																			// und
																			// v2
																			// auf
																			// Cylinder

					if (debug) {
						System.out.println("Methode2: cylinder matched_2");
						System.out.println("Methode2: hitPointOb: "
								+ hitPointOb3[0] + ", " + hitPointOb3[1] + ", "
								+ hitPointOb3[2]);
					}

					double[] hitPoint = new double[4];
					Pn.homogenize(hitPoint, hitPointOb3);
					hitPoint = m.multiplyVector(hitPoint);
					hits.add(hitPoint);
				}
			}
		}

		if (debug) {
			System.out.println("Methode2: "
					+ (System.currentTimeMillis() - time) + " ms");
			System.out.println("");
		}

	}
}
