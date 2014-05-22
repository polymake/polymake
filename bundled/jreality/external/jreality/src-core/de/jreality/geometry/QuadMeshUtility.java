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

import java.awt.Dimension;

import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.DataList;
import de.jreality.scene.data.DoubleArray;
import de.jreality.scene.data.StorageModel;

/**
 * Static methods applicable to quad meshes. In
 * jReality, quad meshes are indentified as instances of {@link IndexedFaceSet} 
 * which have a non-null {@link Geometry Attribute} with 
 * key {@link de.jreality.geometry.GeometryUtility#QUAD_MESH_SHAPE}.
 * <p>
 * These methods support extracting given <i>u</i> or <i>v</i> parameter curves.
 * @author Charles Gunn
 *
 */
public class QuadMeshUtility {

	private QuadMeshUtility() {}


	public static double[][] extractUParameterCurve(double[][] curve, IndexedFaceSet ifs, int which)	{
		return extractParameterCurve(curve, ifs, which, 0);
	}

	public static double[][] extractVParameterCurve(double[][] curve, IndexedFaceSet ifs, int which)	{
		Dimension dim = (Dimension) ifs.getGeometryAttributes(GeometryUtility.QUAD_MESH_SHAPE);
		return extractParameterCurve(curve, ifs,  which, 1);
	}

	/**
	 * Extracts the specified parameter curve from the quad mesh represented by <i>ifs</i>.
	 * @param curve		where to store the output curve; null OK
	 * @param ifs		the quad mesh
	 * @param which		the index of the curve to extract
	 * @param type		0: extract curve for fixed u-value; 1: fixed v-value
	 * @return			the extracted curve
	 */
	public static double[][] extractParameterCurve(double[][] curve, IndexedFaceSet ifs,  int which, int type)	{
		Object foo = ifs.getGeometryAttributes(GeometryUtility.QUAD_MESH_SHAPE);
		if (foo == null)	
			throw new IllegalArgumentException("Not a quad mesh");
		Dimension dim = (Dimension) foo;
		int uSize = dim.width;
		int vSize = dim.height;
		DataList verts = ifs.getVertexAttributes(Attribute.COORDINATES);
//		int u = qms.getMaxU();
//		int v = qms.getMaxV();
		boolean closedU = false; //qms.isClosedInUDirection();
		boolean closedV = false; //qms.isClosedInVDirection();
		int numverts = uSize*vSize;
		int lim = 0, begin = 0, stride = 0, modulo;
		if (type == 0)	{
			lim = (closedV) ? vSize+1 : vSize;
			begin = which;
			stride = uSize;
			modulo = numverts;
		} else {
			lim = (closedU) ? uSize+1 : uSize;
			begin = which * uSize;
			stride = 1;
			modulo = uSize;
		}
		int n = GeometryUtility.getVectorLength(verts);
		if (curve == null || curve.length != lim || curve[0].length != n)	 curve = new double[lim][n];
		int m, i;
		for (i = 0, m = 0; i<lim; ++i, m += stride)	{
			int xx = begin + (m % modulo);
			DoubleArray da = verts.item(xx).toDoubleArray();
			for (int j = 0; j < n; ++j)	{
				curve[i][j] = da.getValueAt(j);				
			}
		}
		return curve;
	}

	public static void generateAndSetEdgesFromQuadMesh(IndexedFaceSet qm)	{
	{
		Object obj = qm.getGeometryAttributes(GeometryUtility.QUAD_MESH_SHAPE);
		if (obj == null || !( obj instanceof Dimension))
			throw new IllegalStateException("Not a quad mesh");
		int uLineCount = ((Dimension)obj).width;
		int vLineCount = ((Dimension)obj).height;
		
		int sizeUCurve = vLineCount; 
		int sizeVCurve = uLineCount; 
		int numVerts = uLineCount*vLineCount;
		int[][] indices = new int[uLineCount +vLineCount][];
		for (int i = 0; i<uLineCount; ++i)	{
			indices[i] = new int[sizeUCurve];
			for (int j = 0; j< sizeUCurve; ++j)	  indices[i][j] = ((j)*uLineCount + (i%uLineCount))%numVerts;
		}
		for (int i = 0; i<vLineCount; ++i)	{
			indices[i+uLineCount] = new int[sizeVCurve];
			for (int j = 0; j< sizeVCurve; ++j)	  indices[i+uLineCount][j] = (i*uLineCount + (j%uLineCount))%numVerts;
		}	
		qm.setEdgeCountAndAttributes(Attribute.INDICES,StorageModel.INT_ARRAY_ARRAY.createReadOnly(indices));
	}
		
	}

}
