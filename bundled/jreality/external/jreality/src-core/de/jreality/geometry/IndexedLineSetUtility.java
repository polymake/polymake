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


package de.jreality.geometry;

import de.jreality.math.Rn;
import de.jreality.scene.IndexedLineSet;
import de.jreality.scene.Scene;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.DataList;
import de.jreality.scene.data.DoubleArray;
import de.jreality.scene.data.IntArray;
import de.jreality.scene.data.IntArrayArray;
import de.jreality.scene.data.StorageModel;

/**
 * Static methods for constructing, extracting, and modifying 
 * instances of {@link IndexedLineSet}.
 * @author Charles Gunn
 * {@see IndexedLineSetUtility} for more ways to specify instances of {@link IndexedLineSet}.
 *
 */
public class IndexedLineSetUtility {

	private IndexedLineSetUtility() {
	}

	public static IndexedLineSet refine(IndexedLineSet ils, int n)	{
		int[][] indices = ils.getEdgeAttributes(Attribute.INDICES).toIntArrayArray(null);
		int totalSegments = 0;
		for (int i=0; i<indices.length; ++i)	{
			totalSegments += indices[i].length-1;
//			if (indices[i].length != 2) {
//				throw new IllegalArgumentException("Edge array can have only 2 points per curve");
//			}
		}
		double[][] verts = ils.getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray(null);
		int numEdges = ils.getNumEdges();
		int veclength = verts[0].length;
		double[][] newVerts = new double[n*totalSegments][veclength];
		int[][] newIndices = new int[totalSegments][n];
		int runningCount = 0;
		for (int i = 0; i<numEdges; ++i)	{
			for (int k = 0; k<indices[i].length-1; ++k)	{
				int i0 = indices[i][k];
				int i1 = indices[i][k+1];
				double[] p0 = verts[i0];
				double[] p1 = verts[i1];
				for (int j = 0; j<n; ++j)	{
					double t = (j)/(n-1.0);
					double s = 1.0 - t;
					newVerts[runningCount*n+j] = Rn.linearCombination(null, s, p0, t, p1);
					newIndices[runningCount][j] = runningCount*n+j;
				}		
				runningCount++;
			}
		}
		IndexedLineSetFactory ifsf = new IndexedLineSetFactory();
		ifsf.setVertexCount(newVerts.length);
		ifsf.setVertexCoordinates(newVerts);
		ifsf.setEdgeCount(newIndices.length);
		ifsf.setEdgeIndices(newIndices);
		ifsf.update();
		return ifsf.getIndexedLineSet();
	}


	/**
	 * @param curve
	 * @param ils
	 * @param i
	 * @return
	 */
	public static double[][] extractCurve(double[][] curve, IndexedLineSet ils, int i) {
		DataList verts = ils.getVertexAttributes(Attribute.COORDINATES);
		DataList indices = ils.getEdgeAttributes(Attribute.INDICES);
		IntArray thisEdge = indices.item(i).toIntArray();
		int n = thisEdge.getLength();
		double[][] output = null;
		if (curve == null || curve.length != n) output = new double[n][];
		else output = curve;
		for (int j = 0; j<n; ++j)	{
			int which = thisEdge.getValueAt(j);
			output[j] = verts.item(which).toDoubleArray(null);
		}
		return output;
	}

	
	public static double[][] extractCurveColors(double[][] curve, IndexedLineSet ils, int i) {
		DataList verts = ils.getVertexAttributes(Attribute.COLORS);
		if (verts == null) return null;
		DataList indices = ils.getEdgeAttributes(Attribute.INDICES);
		IntArray thisEdge = indices.item(i).toIntArray();
		int n = thisEdge.getLength();
		double[][] output = null;
		if (curve == null || curve.length != n) output = new double[n][];
		else output = curve;
		for (int j = 0; j<n; ++j)	{
			int which = thisEdge.getValueAt(j);
			output[j] = verts.item(which).toDoubleArray(null);
		}
		return output;
	}

	public static double[] extractRadii(double[] curve, IndexedLineSet ils, int i)	{
		DoubleArray verts = ils.getVertexAttributes(Attribute.RELATIVE_RADII).toDoubleArray();
		if (verts == null) return null;
		DataList indices = ils.getEdgeAttributes(Attribute.INDICES);
		IntArray thisEdge = indices.item(i).toIntArray();
		int n = thisEdge.getLength();
		double[] output = null;
		if (curve == null || curve.length != n) output = new double[n];
		else output = curve;
		for (int j = 0; j<n; ++j)	{
			int which = thisEdge.getValueAt(j);
			output[j] = verts.getValueAt(which);
		}
		return output;
		
	}
	public static IndexedLineSet createCurveFromPoints(double[][] points, boolean closed)	{
		return createCurveFromPoints(null, points,  closed);
	}

	public static IndexedLineSet createCurveFromPoints( IndexedLineSet g, final double[][] points, boolean closed)	{
		return createCurveFactoryFromPoints(points, closed).getIndexedLineSet();
	}
	/**
	 * @param points
	 * @param closed
	 * @return
	 */
	public static IndexedLineSetFactory createCurveFactoryFromPoints( final double[][] points, boolean closed)	{
		return createCurveFactoryFromPoints(null, points, closed);
		
	}
	public static IndexedLineSetFactory createCurveFactoryFromPoints(IndexedLineSetFactory ilsf, final double[][] points, boolean closed)	{
		int n = points.length;
		int size = (closed) ? n+1 : n;
		final int[][] ind = new int[1][size];
		for (int i = 0; i<size ; ++i)	{
			ind[0][i] = (i%n);
		}
		if (ilsf == null) ilsf = new IndexedLineSetFactory();
		ilsf.setVertexCount(points.length);
		ilsf.setVertexCoordinates(points);
		ilsf.setEdgeCount(1);
		ilsf.setEdgeIndices(ind);
		ilsf.update();
		return ilsf;
	}

	public static IndexedLineSet createCurveFromPoints( double[] points, int fiber, boolean closed)	{
		return createCurveFromPoints(null, points, fiber, closed);
	}

	public static IndexedLineSet createCurveFromPoints(IndexedLineSet g, final double[] points, final int fiber, final int[][] indices)	{
		int n = points.length/fiber;
		if (g == null) g = new IndexedLineSet(n,indices.length);
		final IndexedLineSet ils = g;
		Scene.executeWriter(ils, new Runnable () {
			public void run() {
				ils.setEdgeCountAndAttributes(Attribute.INDICES, new IntArrayArray.Array(indices));
				ils.setVertexCountAndAttributes(Attribute.COORDINATES, StorageModel.DOUBLE_ARRAY.inlined(fiber).createWritableDataList(points));
				}
		});
		return g;
		
	}

	public static IndexedLineSet createCurveFromPoints(IndexedLineSet g, final double[] points, int fiber, boolean closed)	{
		int n = points.length/fiber;
		int size = (closed) ? n+1 : n;
		// TODO replace this with different call if IndexedLineSet exists.
		final int[][] ind = new int[1][size];
		for (int i = 0; i<size ; ++i)	{
			ind[0][i] = (i%n);
		}
		return createCurveFromPoints(g, points, fiber, ind);
	}

	public static IndexedLineSet circle(int n, double cx, double cy, double r) {
		return circleFactory(n, cx, cy, r).getIndexedLineSet();
	}

	public static IndexedLineSetFactory circleFactory(int n, double cx, double cy, double r) {
		double[][] verts = new double[n][3];
		double angle = 0, delta = Math.PI * 2 / (n);
		for (int i = 0; i<n; ++i) {
			angle = i * delta;
			verts[i][0] = cx+r*Math.cos(angle);
			verts[i][1] = cy+r*Math.sin(angle);
		}
		return createCurveFactoryFromPoints(verts, true);
	}
	

	public static IndexedLineSet circle(int n) {
		return circle(n, 0, 0, 1);
	}

}
