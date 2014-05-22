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

import java.awt.geom.Rectangle2D;
import java.util.logging.Level;

import de.jreality.math.Pn;
import de.jreality.math.Rn;
import de.jreality.scene.Geometry;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.PointSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.Sphere;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.DataList;
import de.jreality.util.LoggingSystem;
import de.jreality.util.Rectangle3D;
import de.jreality.util.SceneGraphUtility;

/**
 * Static methods for various geometric operations.
 * <p>
 * There are a few basic categories:
 * <ul>
 * <li>static fields for specific geometry attributes, </li>
 * <li>methods for calculating normal vectors for {@link IndexedFaceSet} instances,</li>
 * <li>methods for calculating bounding boxes for {@link PointSet} instances, </li>
 * <li>analogous methods which traverse scene graphs, and </li>
 * <li>methods for transforming the geometry in a scene graph in some way.</li>
 * </ul>
 * @author Charles Gunn and others
 *
 */
public class GeometryUtility {
 
	/**
	 * For setting the bounding box of the geometry;
	 * Value: {@link Rectangle3D}.
	 * @see Geometry#setGeometryAttributes(Attribute, Object).
	 */
	 public static String BOUNDING_BOX = "boundingBox";		// value:	de.jreality.util.Rectangle3D
	/**
	 * For identifying this IndexedFaceSet as a QuadMesh;
	 * Value: {@link java.awt.Dimension}.
	 * @see Geometry#setGeometryAttributes(Attribute, Object).
	 * @see QuadMeshFactory
	 */
	 public static String QUAD_MESH_SHAPE = "quadMesh";	// value:	java.awt.Dimension
	/**
	 * For identifying this IndexedFaceSet as a QuadMesh with a single
	 * value at each point (z-value on a regular x-y 2D domain);
	 * Value: {@link Rectangle2D} identifies the 2D domain.
	 * @see Geometry#setGeometryAttributes(Attribute, Object).
	 * @see HeightFieldFactory
	 */
	 public static String HEIGHT_FIELD_SHAPE = "heightField";	// value:	java.awt.Rectangle2D
	/**
	 * For setting the metric ({@link Pn}) of the geometry; 
	 * Value: {@link Integer}
	 * @see Geometry#setGeometryAttributes(Attribute, Object).
	 */
	 public static String METRIC = "metric";		// value:	Integer
	 
	 public static String FACTORY = "factory";

	private GeometryUtility() {}
	
	public static int getMetric(Geometry g ) {
		Object sigO = g.getGeometryAttributes(METRIC);
		int sig = Pn.EUCLIDEAN;
		if (sigO != null && sigO instanceof Integer)	{
			sig = ((Integer) sigO).intValue();
		}
		return sig;
	}

	public static AbstractGeometryFactory getFactory(Geometry g ) {
		Object factory = g.getGeometryAttributes(FACTORY);
		if (factory != null && factory instanceof AbstractGeometryFactory)	{
			LoggingSystem.getLogger(GeometryUtility.class).log(Level.FINER,"Factory found");
			return (AbstractGeometryFactory) factory;
		}
		return null;
	}
	/**
	 * Set the geometry attribute {@link GeometryUtility#FACTORY} to the given factory,
	 * which may be <i>null</i>.
	 * @param g
	 * @param fac
	 */
	public static void setFactory(Geometry g, AbstractGeometryFactory fac)	{
		g.setGeometryAttributes(GeometryUtility.FACTORY, fac);
	}
	
	/**
	 * Set the metric ({@link Pn}) associated to this geometry.
	 * @param g
	 * @param s
	 */
    public static void setMetric(Geometry g, int s)	{
		Object o = g.getGeometryAttributes(METRIC);
		if (o != null && o instanceof Integer)		{
			if (((Integer) o).intValue() == s) return;			//unchanged
		}
		g.setGeometryAttributes(METRIC, new Integer(s));
	}
    
	/**
	 * Find out the length of the first element of this {@link DataList}.
	 * 
	 * @param ps
	 * @return
	 */
	 public static int getVectorLength(DataList ps)		{
		int[] dims = ps.getStorageModel().getDimensions(ps);
		int vl = dims[dims.length-1];
		if (vl == -1)		// not set; assume uniform
		{
		   ps=(DataList) ps.item(0);
		   vl = ps.size();
		}
		return vl;
	}
	
	/**
	 * Find out the length of the coordinates for a single vertex of this {@link PointSet}.
	 * @param ps
	 * @return
	 */
	 public static int getVectorLength(PointSet ps)		{
		DataList vv = ps.getVertexAttributes(Attribute.COORDINATES);
		return getVectorLength(vv);
	}
	
	/**
	 * @deprecated Use {@link BoundingBoxUtility#calculateBoundingBox(double[],SceneGraphComponent)} instead
	 */
	public static Rectangle3D calculateBoundingBox(double[] initialMatrix, SceneGraphComponent sgc) {
		return BoundingBoxUtility.calculateBoundingBox(initialMatrix, sgc);
	}
	
	/**
	 * @deprecated Use {@link BoundingBoxUtility#calculateBoundingBox(double[][])} instead
	 */
	 public static Rectangle3D calculateBoundingBox(double[][] verts)	{
		return BoundingBoxUtility.calculateBoundingBox(verts);
	}
	
    /**
	 * @deprecated Use {@link BoundingBoxUtility#calculateBoundingBox(PointSet)} instead
	 */
	public static Rectangle3D calculateBoundingBox(PointSet ps)	{
		return BoundingBoxUtility.calculateBoundingBox(ps);
	}
    
    /**
	 * @deprecated Use {@link BoundingBoxUtility#calculateBoundingBox(SceneGraphComponent)} instead
	 */
	public static Rectangle3D calculateBoundingBox(SceneGraphComponent sgc)	{
		return BoundingBoxUtility.calculateBoundingBox(sgc);
	}

  	/**
	 * @deprecated Use {@link BoundingBoxUtility#calculateBoundingBox(Sphere)} instead
	 */
	public static Rectangle3D calculateBoundingBox(Sphere sph)	{
		return BoundingBoxUtility.calculateBoundingBox(sph);
	}
   
	/**
	 * @deprecated Use {@link BoundingBoxUtility#calculateChildrenBoundingBox(SceneGraphComponent)} instead
	 */
	public static Rectangle3D calculateChildrenBoundingBox(SceneGraphComponent sgc)	{
		return BoundingBoxUtility.calculateChildrenBoundingBox(sgc);
	}
	
	/**
	 * @deprecated Use {@link IndexedFaceSetUtility#calculateAndSetFaceNormals(IndexedFaceSet)} instead
	 */
	public static void calculateAndSetFaceNormals(IndexedFaceSet ifs)   {
		IndexedFaceSetUtility.calculateAndSetFaceNormals(ifs);
	}

	/**
	 * @deprecated Use {@link IndexedFaceSetUtility#calculateAndSetNormals(IndexedFaceSet)} instead
	 */
	public static void calculateAndSetNormals(IndexedFaceSet ifs)	{
		IndexedFaceSetUtility.calculateAndSetNormals(ifs);
	}

	/**
	 * @deprecated Use {@link IndexedFaceSetUtility#calculateAndSetVertexNormals(IndexedFaceSet)} instead
	 */
	public static void calculateAndSetVertexNormals(IndexedFaceSet ifs) {
		IndexedFaceSetUtility.calculateAndSetVertexNormals(ifs);
	}
	
	/**
	 * @deprecated Use {@link IndexedFaceSetUtility#calculateFaceNormals(IndexedFaceSet)} instead
	 */
	public static double[][] calculateFaceNormals(IndexedFaceSet ifs)	{
		return IndexedFaceSetUtility.calculateFaceNormals(ifs);
	}
    
	
	/**
	 * @deprecated Use {@link IndexedFaceSetUtility#calculateFaceNormals(IndexedFaceSet,int)} instead
	 */
	public static double[][] calculateFaceNormals(IndexedFaceSet ifs, int metric) {
		return IndexedFaceSetUtility.calculateFaceNormals(ifs, metric);
	}
   
	/**
	 * @deprecated Use {@link IndexedFaceSetUtility#calculateFaceNormals(int[][],double[][],int)} instead
	 */
	public static double[][] calculateFaceNormals(int[][] indices, double[][] verts, int metric)	{
		return IndexedFaceSetUtility.calculateFaceNormals(indices, verts,
				metric);
	}
	
	  /**
	 * @deprecated Use {@link IndexedFaceSetUtility#calculateFaceNormals(SceneGraphComponent)} instead
	  */
	 public static void calculateFaceNormals(SceneGraphComponent c) {
		IndexedFaceSetUtility.calculateFaceNormals(c);
	}
	
    /**
	 * @deprecated Use {@link IndexedFaceSetUtility#calculateVertexNormals(IndexedFaceSet)} instead
	 */
	public static double[][] calculateVertexNormals(IndexedFaceSet ifs)	{
		return IndexedFaceSetUtility.calculateVertexNormals(ifs);
	}
	
     /**
	 * @deprecated Use {@link IndexedFaceSetUtility#calculateVertexNormals(IndexedFaceSet,int)} instead
	 */
	public static double[][] calculateVertexNormals(IndexedFaceSet ifs,
				int metric) {
					return IndexedFaceSetUtility.calculateVertexNormals(ifs,
							metric);
				}


	/**
	 * @deprecated Use {@link IndexedFaceSetUtility#calculateVertexNormals(int[][],double[][],double[][],int)} instead
	   */
	 public static double[][] calculateVertexNormals(int[][] indices,
				double[][] vertsAs2D, double[][] fn, int metric) {
					return IndexedFaceSetUtility.calculateVertexNormals(
							indices, vertsAs2D, fn, metric);
				}

	/**
	 * @deprecated Use {@link IndexedFaceSetUtility#calculateVertexNormals(SceneGraphComponent)} instead
	 */
	 public static void calculateVertexNormals(SceneGraphComponent c) {
		IndexedFaceSetUtility.calculateVertexNormals(c);
	}
	
	 /**
	 * @deprecated Use {@link SceneGraphUtility#flatten(SceneGraphComponent)} instead
	 */
	public static SceneGraphComponent flatten(SceneGraphComponent sgc)		{
		return SceneGraphUtility.flatten(sgc);
	}

     /**
	 * @deprecated Use {@link SceneGraphUtility#flatten(SceneGraphComponent,boolean)} instead
	 */
	 public static SceneGraphComponent flatten(SceneGraphComponent sgc, final boolean rejectInvis)		{
		return SceneGraphUtility.flatten(sgc, rejectInvis);
	}

	/**
	 * @deprecated Use {@link Rn#convertArray2DToArray1D(double[],double[][])} instead
	 */
	static double[] convert2dArrayTo1dArray(double[] target, double[][] src) {
		return Rn.convertArray2DToArray1D(target, src);
	}
	
}

