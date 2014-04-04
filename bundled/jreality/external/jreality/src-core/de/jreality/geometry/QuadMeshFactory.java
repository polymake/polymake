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

import java.awt.Color;
import java.awt.Dimension;

import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.IndexedLineSet;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.DataList;
import de.jreality.scene.data.DataListSet;

/**
 * This factory class supports creating and editing of quad meshes, that is, regular meshes of quadrilaterals.  In contrast to the
 * {@link IndexedFaceSetFactory} where the face indices must be explicitly set, this factory expects that the data provided to
 * the {@link #setVertexCoordinates(DataList)} method and its variants, is arranged in the special form corresponding to a quad mesh, and
 * does not allow the face indices to be explicitly set.
 * <p>
 * Instead of providing face indices, this factory has two methods {@link #setULineCount(int)} and {@link #setVLineCount(int)} to
 * describe the dimensions of the quad mesh.  If you think of the data arranged in a 2D array, the rows have length u and there are v rows.
 * To support this picture further, there is another method provided for specifying the underlying point set: {@link #setVertexCoordinates(double[][][])}.
 * In this 3D array, the left-most index counts off the rows, the middle index runs through a given row, and the right most index runs through
 * the data for a specific vertex. There are analogous methods for setting the vertex normals, texture coordinates, colors, and relative radii.
 * <p>
 * There are also methods for specifying whether the quad mesh <i>wraps around</i> in the two parameter directions: {@link #setClosedInUDirection(boolean)} and 
 * {@link #setClosedInVDirection(boolean)}. Defaults for both is <code>false</code>. This is used for
 * example in the case that vertex normals are automatically generated, to identify correctly which faces are adjecent
 * to a given vertex.  Note: in case the surface <b>is</b> closed in one or the other direction, the factory does not remove the duplicated vertices.
 * <p>
 * There are also some other new control methods:
 * <ul>
 * <li>{@link #setGenerateTextureCoordinates(boolean)} Generate texture coordinates mapping quad-mesh to unit square in (u,v) space</li>
 * <li>{@link #setEdgeFromQuadMesh(boolean)} If <code>true</code>, generate <i>long</i> edges, one for each row of the mesh. (See {@link IndexedLineSet}).</li>
 * </ul>
 * <p>
 * The underlying geometry managed by this factory is an instance of {@link IndexedFaceSet} -- there is no QuadMesh class.  However,
 * the factory provides the instance with an {@link Attribute}, {@link GeometryUtility#QUAD_MESH_SHAPE} whose value is an instance of {@link Dimension}
 * specifying the (u,v) dimensions of the mesh -- in case a backend can optimize its handling of the geometry.
 * <p>
 *  For an example, see
 * <a href=http://www3.math.tu-berlin.de/jreality/mediawiki/index.php/Use_a_quad_mesh_factory> this tutorial</a>.
 * <p>
 * @author gunn
 * @see QuadMeshFactory
 */
public class QuadMeshFactory extends AbstractQuadMeshFactory {

	public QuadMeshFactory() {
		super();
	}
	
	public IndexedFaceSet getQuadMesh() {
		return getIndexedFaceSet();
	}
	
	/* vertex attributes */
	
	public void setVertexAttribute( Attribute attr, DataList data ) {
		super.setVertexAttribute( attr, data );
	}
	
	public void setVertexAttribute(Attribute attr, double [] data ) {
		super.setVertexAttribute( attr, data );
	}
	
	public void setVertexAttribute(Attribute attr,  double [][] data ) {
		super.setVertexAttribute( attr, data);
	}
	
	public void setVertexAttributes(DataListSet dls ) {
		super.setVertexAttributes( dls );
	}

	public void setVertexCoordinates( DataList data ) {
		super.setVertexCoordinates(data);
	}
	
	public void setVertexCoordinates( double [] data ) {
		super.setVertexCoordinates( data );
	}
	
	public void setVertexCoordinates( double [][] data ) {
		super.setVertexCoordinates( data );
	}
	
	/**
	 * A convenience method to allow users to work with a rectangular 3D array to describe a quad mesh
	 * @param points a 3-dimension, rectangular array; the first two dimensions must equal
	 * the number of v-lines (@link getVLineCount) and u-lines (@link getULineCount).
	 */
	public void setVertexCoordinates(double[][][] points) {
		double[][] npoints = convertDDDtoDD(points);
		setVertexCoordinates(npoints);
	}

	public void setVertexColors(double[][][] cs) {
		double[][] ncs = convertDDDtoDD(cs);
		setVertexColors(ncs);
	}

	public void setVertexColors( Color [] data ) {
		setVertexColors( toDoubleArray( data ) );
	}
	
	/**
	 * @param points
	 * @return
	 */
	private double[][] convertDDDtoDD(double[][][] points) {
		int lengthv = points.length;
		int lengthu = points[0].length;
		int lengthf = points[0][0].length;
		if (lengthv != getVLineCount() || lengthu != getULineCount() ) {
			throw new IllegalArgumentException("Bad dimension for 3D array");
		}
		double[][] npoints = new double[lengthv * lengthu][];
		for (int i = 0; i<lengthv; ++i)	{
			for (int j = 0; j<lengthu; ++j)	{
				//System.arraycopy(points[i][j], 0, npoints[i*lengthu+j],0,lengthf);
				npoints[i*lengthu+j] = points[i][j];
			}
		}
		return npoints;
	}
	
	public void setVertexNormals( DataList data ) {
		super.setVertexNormals(data);
	}
	
	public void setVertexNormals( double [] data ) {
		super.setVertexNormals( data );
	}
	
	public void setVertexNormals( double [][] data ) {
		super.setVertexNormals( data );
	}
	
	public void setVertexNormals(double[][][] data) {
		double[][] ncs = convertDDDtoDD(data);
		setVertexNormals(ncs);
	}
	
	public void setVertexColors( DataList data ) {
		super.setVertexColors( data );
	}
	
	public void setVertexColors( double [] data ) {
		super.setVertexColors( data );
	}
	
	public void setVertexColors( double [][] data ) {
		super.setVertexColors( data );
	}
	
	public void setVertexTextureCoordinates( DataList data ) {
		super.setVertexTextureCoordinates( data );
	}
	
	public void setVertexTextureCoordinates( double [] data ) {
		super.setVertexTextureCoordinates( data );
	}
	
	public void setVertexTextureCoordinates( double [][] data ) {
		super.setVertexTextureCoordinates( data );
	}
	
	public void setVertexTextureCoordinates(double[][][] data) {
		double[][] ncs = convertDDDtoDD(data);
		setVertexTextureCoordinates(ncs);
	}

	public void setVertexLabels( String [] data ) {
		super.setVertexLabels( data );
	}
	/* face attributes */
	
	/**
	 * Superclass methods are protected so we override to make public
	 * Documentation is lacking ...
	 */
	@Override
	public void setFaceAttribute( Attribute attr, DataList data) {
		super.setFaceAttribute( attr, data );
	}
	
	@Override
	public void setFaceAttribute(Attribute attr, double[] data) {
		super.setFaceAttribute(attr, data);
	}

	@Override
	public void setFaceAttribute(Attribute attr, double[][] data) {
		super.setFaceAttribute(attr, data);
	}

	public void setFaceAttributes(DataListSet dls ) {
		super.setFaceAttributes(dls);
	}

	public void setFaceNormals( DataList data ) {
		super.setFaceNormals(data);
	}
	
	public void setFaceNormals( double [] data ) {
		super.setFaceNormals(data);
	}
	
	public void setFaceNormals( double [][] data ) {
		super.setFaceNormals(data);
	}
	
	public void setFaceColors( DataList data ) {
		super.setFaceColors(data);
	}
	
	public void setFaceColors( double [] data ) {
		super.setFaceColors(data);
	}
	
	public void setFaceColors( double [][] data ) {
		super.setFaceColors(data);
	}
	
	public void setFaceLabels( String [] data ) {
		super.setFaceLabels( data );
	}
}
