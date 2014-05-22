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

import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.DataList;
import de.jreality.scene.data.DataListSet;

/**
 * This factory classes can be used to create and edit instances of {@link IndexedFaceSet}.  See {@link IndexedLineSetFactory} and
 * {@link PointSetFactory} for functionality inherited from the superclasses.
 * </p>
 * <h4>Specifying the geometry</h4>
 * <p>
 *  To specify faces, first set the number of faces using {@link #setFaceCount(int)}. The faces themselves
 *  are specified using the method {@link #setFaceIndices(int[][])} and its variants. Each element of this array is a list of
 *  indices into the coordinate array of the underlying point set (See {@link PointSetFactory}).
 * <p>
 * <h4>Standard attributes</h4>
 * <p>
 * There are methods for setting the built-in face attributes normals, colors, and labels.
 * Texture coordinates can have fiber length 2, 3, or 4.  Normals in euclidean case must have fiber length 3; otherwise they should have 
 * length 4. Labels are represented by an array of type <code>String[]</code>, and are displayed at the center of the face.
 * <p>
 * <h4>Generated attributes</h4>
 * There are a number of boolean methods to control whether the factory generates various types of derivative information:
  <ul>
  <li>{@link #setGenerateAABBTree(boolean)}: 
  	The AABBTree is stored as an {@link Attribute} within the geometry and is used to optimize picking.</li>
  <li>{@link #setGenerateFaceLabels(boolean)}:
   The face labels show the index of the face within the face array and are displayed at the midpoint of the face.</li>
  <li>{@link #setGenerateFaceNormals(boolean)}: 
   The face normals are generated using the cross product of the first two edges of the face, with respect to the specified metric (see {@link #setMetric(int)}.</li>
  <li>{@link #setGenerateVertexNormals(boolean)}:
    The vertex normals are generated based on the face normals, by averaging using the combinatorial information in the face index array.</li>
  <li>{@link #setGenerateEdgesFromFaces(boolean)}:
  By default there are no edges attached to the indexed face set.  Either set them explicitly using {@link #setEdgeIndices(int[])} and
  its variants, or call this method with <code>true</code> as argument; then the edge indices will be generated as the edges of the faces.
 </li>
  </ul>
 * <p>
 * By default, all these values are <code>false</code>.
 * 
 * <h4>More Attributes</h4>
 * <p>
 * For attributes not included in the built-in set, use the methods
 * <ul>
 * <li>{@link #setFaceAttribute(Attribute, DataList)}</li>
 * <li>{@link #setFaceAttribute(Attribute, double[])}</li>
 * <li>{@link #setFaceAttribute(Attribute, double[][])}</li>
 * </ul>
 * <p>
 * <h4>Unwrapping</h4>
 * <p>
 * A geometry in the jReality scene graph does not allow per face vertex attributes. So to unwrap a geometry e.g. to put a texture on it, one has
 * to introduce multiple vertices. This factory supports this as follows: set vertex coordinates for all vertices (<code>setVertexCount()</code>, 
 * <code>setVertexCoordinates()</code>), 
 * set the face indices to the wrapped faces (leave out the extra vertices) (<code>setFaceCount()</code>, <code>setFaceIndices</code>), and use 
 * {@link #setUnwrapFaceIndices(int[][])}  or its variants to introduce the unwrapped faces (same number of faces). 
 * 
 * <p>The <code>unwrapFaceIndices</code> 
 * are written into the created <code>IndexedFaceSet</code>. The <code>faceIndices</code> are used to generate attributes, in particular to 
 * generate correct vertex normals. The vertices that represent the same vertex are detected from correspondence in <code>faceIndices</code>
 * and <code>unwrapFaceIndices</code>, and not from the vertex coordinates. There is also a
 * <a href=http://www3.math.tu-berlin.de/jreality/mediawiki/index.php/unwrap_a_geometry_using_a_factory>tutorial that explains the use of unwrapping</a>.
 * 
 * <p>Since the vertex count includes the extra vertices for unwrapping, one needs to multiply vertex attributes for the extra vertices. 
 * More often than not, one needs to copy the attributes to the corresponding vertices. The correspondence may be determined from the 
 * two sets of face indices. The method <code>unwrapVertexAttributes()</code> does this for you. It has a static version (which needs 
 * more parameters) and an instance version (which reads the corresponding parameters from the instance). It comes in many versions in
 * order to be directly applicable to those vertex attributes that are flat arrays of primitives.
 
 * @see QuadMeshFactory
 * @see ParametricSurfaceFactory
 * 
 *  <p>
 *  For an example, see
 *  <a href=http://www3.math.tu-berlin.de/jreality/mediawiki/index.php/Use_an_indexed_face_set_factory> this tutorial</a>
 * 
 * @author gunn, peters
 *
 */
public class IndexedFaceSetFactory extends AbstractIndexedFaceSetFactory {

	public IndexedFaceSetFactory() {
		super();
	}

	/* vertex attributes */
	
	public void setVertexCount( int count ) {
		super.setVertexCount(count);
	}
	
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
	
	public void setVertexNormals( DataList data ) {
		super.setVertexNormals(data);
	}
	
	public void setVertexNormals( double [] data ) {
		super.setVertexNormals( data );
	}
	
	public void setVertexNormals( double [][] data ) {
		super.setVertexNormals( data );
	}
	
	public void setVertexColors( DataList data ) {
		super.setVertexColors( data );
	}
	
	public void setVertexColors( double [] data ) {
		super.setVertexColors( data );
	}
	
	public void setVertexColors( Color [] data ) {
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

	public void setVertexLabels( String [] data ) {
		super.setVertexLabels( data );
	}

	public void setVertexRelativeRadii( double [] data ) {
		super.setVertexRelativeRadii( data );
	}
	
	/* edge attributes */

	public void setEdgeCount( int count ) {
		super.setEdgeCount(count);
	}
	
	@Override
	public void setEdgeAttribute(Attribute attr, DataList data)	{
		super.setEdgeAttribute(attr, data);
	}
	
	@Override
	public void setEdgeAttribute(Attribute attr, double[] data) {
		super.setEdgeAttribute(attr, data);
	}

	@Override
	public void setEdgeAttribute(Attribute attr, double[][] data) {
		super.setEdgeAttribute(attr, data);
	}

	public void setEdgeIndices( int[][] data ) {
		super.setEdgeIndices(data);
	}
	
	public void setEdgeIndices( int[] data, int pointCountPerLine ) {
		super.setEdgeIndices(data, pointCountPerLine );
	}
	
	public void setEdgeIndices( int[] data ) {
		super.setEdgeIndices( data );
	}
	
	
	public void setEdgeColors( DataList data ) {
		super.setEdgeColors( data );
	}
	
	public void setEdgeColors( double [] data ) {
		super.setEdgeColors(data);
	}
	
	public void setEdgeColors( Color [] data ) {
		super.setEdgeColors( data );
	}
	
	public void setEdgeColors( double [][] data ) {
		super.setEdgeColors(data);
	}

	public void setEdgeLabels( String[] data ) {
		super.setEdgeLabels( data );
	}
	
	public void setEdgeRelativeRadii( double [] data ) {
		super.setEdgeRelativeRadii( data );
	}

	/* face attributes */
	
	public void setFaceCount( int count ) {
		super.setFaceCount( count );
	}
	
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
	
	public void setFaceIndices( DataList data ) {
		super.setFaceIndices(data);
	}
	
	public void setFaceIndices( int[][] data ) {
		super.setFaceIndices(data);
	}
	
	public void setFaceIndices( int[] data, int pointCountPerFace ) {
		super.setFaceIndices(data, pointCountPerFace );
	}
	
	public void setFaceIndices( int[] data ) {
		super.setFaceIndices(data);
	}
	
	public void setUnwrapFaceIndices( DataList data ) {
		super.setUnwrapFaceIndices(data);
	}
	
	public void setUnwrapFaceIndices( int[][] data ) {
		super.setUnwrapFaceIndices(data);
	}
	
	public void setUnwrapFaceIndices( int[] data, int pointCountPerFace ) {
		super.setUnwrapFaceIndices(data, pointCountPerFace );
	}
	
	public void setUnwrapFaceIndices( int[] data ) {
		super.setUnwrapFaceIndices(data);
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
	
	public void setFaceColors( Color [] data ) {
		super.setFaceColors( data );
	}
	
	public void setFaceColors( double [][] data ) {
		super.setFaceColors(data);
	}

	public void setFaceLabels( String [] data ) {
		super.setFaceLabels( data );
	}

	/* unwrap convenience methods */
	public boolean[] unwrapVertexAttributes(boolean[] data,
			int entriesPerVertex) {
		return super.unwrapVertexAttributes(data, entriesPerVertex);
	}

	public static boolean[] unwrapVertexAttributes(boolean[] data, int entriesPerVertex,
			int[][] faceIndices, int[][] unwrapFaceIndices, int numberOfUnwrapVertices) {
		return AbstractIndexedFaceSetFactory.unwrapVertexAttributes(
				data, entriesPerVertex, faceIndices, unwrapFaceIndices, numberOfUnwrapVertices);
	}

	public char[] unwrapVertexAttributes(char[] data, int entriesPerVertex) {
		return super.unwrapVertexAttributes(data, entriesPerVertex);
	}

	public static char[] unwrapVertexAttributes(char[] data, int entriesPerVertex,
			int[][] faceIndices, int[][] unwrapFaceIndices, int numberOfUnwrapVertices) {
		return AbstractIndexedFaceSetFactory.unwrapVertexAttributes(
				data, entriesPerVertex, faceIndices, unwrapFaceIndices, numberOfUnwrapVertices);
	}

	public double[] unwrapVertexAttributes(double[] data,
			int entriesPerVertex) {
		return super.unwrapVertexAttributes(data, entriesPerVertex);
	}

	public static double[] unwrapVertexAttributes(double[] data, int entriesPerVertex,
			int[][] faceIndices, int[][] unwrapFaceIndices, int numberOfUnwrapVertices) {
		return AbstractIndexedFaceSetFactory.unwrapVertexAttributes(
				data, entriesPerVertex, faceIndices, unwrapFaceIndices, numberOfUnwrapVertices);
	}

	public <E> E[] unwrapVertexAttributes(E[] data) {
		return super.unwrapVertexAttributes(data);
	}

	public static <E> E[] unwrapVertexAttributes(E[] data, 
			int[][] faceIndices, int[][] unwrapFaceIndices, int numberOfUnwrapVertices) {
		return AbstractIndexedFaceSetFactory.unwrapVertexAttributes(
				data, faceIndices, unwrapFaceIndices, numberOfUnwrapVertices);
	}

	public float[] unwrapVertexAttributes(float[] data, int entriesPerVertex) {
		return super.unwrapVertexAttributes(data, entriesPerVertex);
	}

	public static float[] unwrapVertexAttributes(float[] data, int entriesPerVertex,
			int[][] faceIndices, int[][] unwrapFaceIndices, int numberOfUnwrapVertices) {
		return AbstractIndexedFaceSetFactory.unwrapVertexAttributes(
				data, entriesPerVertex, faceIndices, unwrapFaceIndices, numberOfUnwrapVertices);
	}

	public int[] unwrapVertexAttributes(int[] data, int entriesPerVertex) {
		return super.unwrapVertexAttributes(data, entriesPerVertex);
	}

	public static int[] unwrapVertexAttributes(int[] data, int entriesPerVertex,
			int[][] faceIndices, int[][] unwrapFaceIndices, int numberOfUnwrapVertices) {
		return AbstractIndexedFaceSetFactory.unwrapVertexAttributes(
				data, entriesPerVertex, faceIndices, unwrapFaceIndices, numberOfUnwrapVertices);
	}

	public long[] unwrapVertexAttributes(long[] data, int entriesPerVertex) {
		return super.unwrapVertexAttributes(data, entriesPerVertex);
	}

	public static long[] unwrapVertexAttributes(long[] data, int entriesPerVertex,
			int[][] faceIndices, int[][] unwrapFaceIndices, int numberOfUnwrapVertices) {
		return AbstractIndexedFaceSetFactory.unwrapVertexAttributes(
				data, entriesPerVertex, faceIndices, unwrapFaceIndices, numberOfUnwrapVertices);
	}

}
