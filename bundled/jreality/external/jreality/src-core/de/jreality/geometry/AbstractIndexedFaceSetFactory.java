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
import java.lang.reflect.Array;

import de.jreality.geometry.OoNode.UpdateMethod;
import de.jreality.math.Pn;
import de.jreality.scene.Geometry;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.DataList;
import de.jreality.scene.data.DataListSet;
import de.jreality.scene.data.DoubleArrayArray;
import de.jreality.scene.data.IntArrayArray;
import de.jreality.scene.data.StorageModel;
import de.jreality.scene.data.StringArray;
import de.jreality.scene.pick.AABBTree;
import de.jreality.util.PickUtility;

/** 
 * This class implements the functionality of the API class {@link IndexedFaceSetFactory}. 
 * Should only be accessed via IndexedFaceSetFactory.  
 */
public class AbstractIndexedFaceSetFactory extends AbstractIndexedLineSetFactory {
	
	/** The IndexedFaceSet to be generated. Reference will not change any more.**/
	final IndexedFaceSet ifs;
	
	/** List of geometry attributes specific to faces. 
	 **/
	GeometryAttributeListSet face = new GeometryAttributeListSet( this, Geometry.CATEGORY_FACE );
	
	/* Standard generated attributes and their ingredients*/
	AttributeGenerator vertexCoordinates = attributeGeneratorNode( vertex, double[][].class, Attribute.COORDINATES);
	AttributeGenerator vertexNormals     = attributeGeneratorNode( vertex, double[][].class, Attribute.NORMALS );
	
	AttributeGenerator edgeIndices       = attributeGeneratorNode( edge, int[][].class,      Attribute.INDICES );
	
	AttributeGenerator faceIndices       = attributeGeneratorNode( face, int[][].class,      Attribute.INDICES );
	AttributeGenerator faceLabels        = attributeGeneratorNode( face, String[].class,     Attribute.LABELS );
	AttributeGenerator faceNormals       = attributeGeneratorNode( face, double[][].class,   Attribute.NORMALS );
	
	/* Ingredients that affect the geometry attributes in a non standard way, or have non standard */
	OoNode faceCount = node( "faceCount", Integer.class, 0 );
	OoNode aabbTree  = node( "aabbTree", AABBTree.class, null );
	boolean generateAABBTree = false;
	OoNode unwrapFaceIndices = node( "unwrapFaceIndices", int[][].class, null);
	OoNode actualVertexOfUnwrapVertex = node ( "actualVertexOfUnwrapVertex", int[].class, null);
	
	/* constructors */
	AbstractIndexedFaceSetFactory( IndexedFaceSet ifs, int metric, boolean generateEdgesFromFaces, boolean generateVertexNormals, boolean generateFaceNormals ) {
		super( ifs, metric );	

		this.ifs = ifs;
		
		setGenerateEdgesFromFaces( generateEdgesFromFaces);
		setGenerateFaceNormals( generateVertexNormals );
		setGenerateFaceNormals( generateFaceNormals );
	}
	
	AbstractIndexedFaceSetFactory( int metric, boolean generateEdgesFromFaces, boolean generateVertexNormals, boolean generateFaceNormals ) {
		this( new IndexedFaceSet(0,0), metric, generateEdgesFromFaces, generateVertexNormals, generateFaceNormals );	
	}
	
	AbstractIndexedFaceSetFactory( IndexedFaceSet existing, int metric ) {
		this(existing,  metric, false, false, false );
	}
	
	AbstractIndexedFaceSetFactory( int metric ) {
		this( metric, false, false, false );
	}
	
	AbstractIndexedFaceSetFactory(IndexedFaceSet existing) {
		this(  existing, Pn.EUCLIDEAN, false, false, false );
	}
	
	public AbstractIndexedFaceSetFactory() {
		this( Pn.EUCLIDEAN );
	}
	
	/* Getters and setters. */
	protected int nof(){
		return (Integer)faceCount.getObject();
	}
	
	protected int getFaceCount() {
		return nof();
	}
	
	void setFaceCount( int count ) {
		face.setCount( count );
		faceCount.setObject(new Integer(count));
	}
	
	protected void setFaceAttribute( Attribute attr, DataList data ) {
		face.setAttribute(attr, data);
	}
	
	protected void setFaceAttribute(Attribute attr, double [] data ) {
		if( data != null && (nof() == 0 && data.length != 0 || data.length % nof() != 0) )
			throw new IllegalArgumentException( "array has wrong length" );
		setFaceAttribute( attr, data==null ? null : new DoubleArrayArray.Inlined( data, data.length / nof() ) );
	}
	
	protected void setFaceAttribute(Attribute attr,  double [][] data ) {
		setFaceAttribute( attr,
				StorageModel.DOUBLE_ARRAY.array(data[0].length).createReadOnly(data));
	}
	
	protected void setFaceAttributes(DataListSet dls ) {
		face.setAttributes(dls);
	}

	protected void setFaceIndices(DataList data) {
		setFaceAttribute(Attribute.INDICES, data);
	}

	protected void setFaceIndices(int[][] data) {
		setFaceAttribute(Attribute.INDICES, new IntArrayArray.Array(data));
	}

	protected void setFaceIndices(int[] data, int pointCountPerFace) {
		if (data != null && data.length != pointCountPerFace * nof())
			throw new IllegalArgumentException("array has wrong length");
		setFaceAttribute(Attribute.INDICES, data == null ? null
				: new IntArrayArray.Inlined(data, pointCountPerFace));
	}

	protected void setFaceIndices(int[] data) {
		setFaceIndices(data, 3);
	}
	
	protected void setUnwrapFaceIndices(DataList data) {
		if (data!=null && data.size()!=nof())
			throw new IllegalArgumentException("Data list of face indices for unwrap faces has wrong length.");
		setUnwrapFaceIndices(data==null?null:data.toIntArrayArray(null));
	}

	protected void setUnwrapFaceIndices(int[][] data) {
		if (data!=null && data.length!=nof())
			throw new IllegalArgumentException("Array of face indices for unwrap faces has wrong length.");
		unwrapFaceIndices.setObject(data);
	}

	protected void setUnwrapFaceIndices(int[] data, int pointCountPerFace) {
		if (data != null && data.length != pointCountPerFace * nof())
			throw new IllegalArgumentException("Array of indices for unwrap faces has wrong length.");
		setUnwrapFaceIndices(data == null ? null
				: new IntArrayArray.Inlined(data, pointCountPerFace));
	}

	protected void setUnwrapFaceIndices(int[] data) {
		setUnwrapFaceIndices(data, 3);
	}
	
	protected void setFaceNormals( DataList data ) {
		setFaceAttribute( Attribute.NORMALS, data );
	}
	
	protected void setFaceNormals( double [] data ) {
		if( data != null && data.length % nof() != 0 )
			throw new IllegalArgumentException( "array has wrong length" );	
		setFaceAttribute( Attribute.NORMALS, data != null ? new DoubleArrayArray.Inlined( data, data.length / nof() ) : null );
	}
	
	protected void setFaceNormals( double [][] data ) {
		setFaceAttribute( Attribute.NORMALS, data == null ? null : new DoubleArrayArray.Array( data ) );
	}
	
	protected void setFaceColors( DataList data ) {
		setFaceAttribute( Attribute.COLORS, data );
	}
	
	protected void setFaceColors( double [] data ) {
		if( data != null && data.length % nof() != 0 )
			throw new IllegalArgumentException( "array has wrong length" );	
		setFaceAttribute( Attribute.COLORS, data==null ? null : new DoubleArrayArray.Inlined( data, data.length / nof() ) );
	}
	
	protected void setFaceColors( Color [] colors ) {
		double[] data = new double[colors.length*4];
		float[] col = new float[4];
		for (int i = 0; i < colors.length; i++) {
			colors[i].getRGBComponents(col);
			data[4*i  ] = col[0];
			data[4*i+1] = col[1];
			data[4*i+2] = col[2];
			data[4*i+3] = col[3];
		}
		setFaceAttribute( Attribute.COLORS, new DoubleArrayArray.Inlined( data, 4 ) );
	}
	
	protected void setFaceColors( double [][] data ) {
		setFaceAttribute( Attribute.COLORS, new DoubleArrayArray.Array( data ) );
	}

	protected void setFaceLabels( DataList data ) {
		if( data != null && data.size() != nof() )
			throw new IllegalArgumentException( "array has wrong length" );
		if( data instanceof StringArray)
			throw new IllegalArgumentException( "argument is not a de.jreality.scene.data.StringArray" );
		setVertexAttribute( Attribute.LABELS, data );
	}
	
	protected void setFaceLabels( String[] data ) {
		if( data != null && data.length != nof() )
			throw new IllegalArgumentException( "array has wrong length" );
		setFaceAttribute( Attribute.LABELS, data==null ? null : new StringArray(data));
	}
	
	public IndexedFaceSet getIndexedFaceSet() {
		return ifs;
	}
	
	public boolean isGenerateEdgesFromFaces() {
		// edgeIndices.outdate(); Why should this be here? 
		return edgeIndices.isGenerate();
	}

	public void setGenerateEdgesFromFaces(boolean generateEdgesFromFaces) {
		if( generateEdgesFromFaces && edge.hasEntries() )
			throw new UnsupportedOperationException( 
					"You cannot generate edges form faces " +
					"while edge attributes are set." +
					"Set them to null before.");
			
		edgeIndices.setGenerate(generateEdgesFromFaces);
		
		if( isGenerateEdgesFromFaces() ) {
			edgeIndices.addDeps( edgeCount );
		} else {
			edgeIndices.removeDeps( edgeCount );
		}
		edge.blockAttributeCount = generateEdgesFromFaces;
	}
	
	public boolean isGenerateVertexNormals() {
		return vertexNormals.isGenerate();
	}

	public void setGenerateVertexNormals(boolean generateVertexNormals) {
		vertexNormals.setGenerate(generateVertexNormals);
	}
	
	public boolean isGenerateFaceNormals() {
		return faceNormals.isGenerate();
	}

	public void setGenerateFaceNormals(boolean generateFaceNormals) {
		faceNormals.setGenerate(generateFaceNormals);
	}

	public boolean isGenerateFaceLabels() {
		return faceLabels.isGenerate();
	}

	public void setGenerateFaceLabels(boolean generateFaceLabels) {
		faceLabels.setGenerate(generateFaceLabels);
	}

	public boolean isGenerateAABBTree() {
		return generateAABBTree;
	}
	
	public void setGenerateAABBTree( boolean generate ) {
		if( generateAABBTree==generate)
			return;
		
		aabbTree.outdate();
		
		generateAABBTree = generate;
		
		//TODO:
	}
    	
    /* IMPLEMENTATIONS OF GENERATED ATTRIBUTES according to the following dependency tree:
     * 
     *  faceCount                metric   vertexCoordinates    faceIndices    
     *     |                        |\        /  ____\________/  /   \        
     *  faceLables                  | \      /  /     \         /     \ 
     *                              |  faceNormals     aabbTree     edgeIndices   
     *  unwrapFaceIndices           |     /                            |?
     *        |                     |    /               edgeCount (see AbstractIndexedLineSet)
     * actualVertexOfUnwrapVertex   |   /
     *                          \   |  /
     *                         vertexNormals
     * 
     */
	
	/* actualVertexOfUnwrapVertex
	 * generate the translation table between actual vertices and unwrap vertices 
	 * from faceIndices and unwrapFaceIndices
	 */
	{
		actualVertexOfUnwrapVertex.addIngr(unwrapFaceIndices);
		actualVertexOfUnwrapVertex.addIngr(faceIndices);
		actualVertexOfUnwrapVertex.setUpdateMethod(
				new OoNode.UpdateMethod() {
					public Object update( Object object) {
						return determineActualVertexOfUnwrapVertex((int[]) object);
					}
				}
		);
	}
	
	int[] determineActualVertexOfUnwrapVertex(int[] actualVOUV) {
		int[][] fi = (int[][]) faceIndices.getObject();
		int[][] unwrapFI = (int[][]) unwrapFaceIndices.getObject();

		return determineActualVertexOfUnwrapVertexImpl(fi, unwrapFI,actualVOUV, nov());
	}

	// determine translation table between actual and unwrap vertex indices
	static int[] determineActualVertexOfUnwrapVertexImpl(
			int[][] fi, int[][] unwrapFI, int[] actualVOUV, int nv) {

		if (fi == null || unwrapFI == null)
			return null;
		if (actualVOUV==null || actualVOUV.length != nv)
			actualVOUV = new int[nv];
		
		for (int f=0; f<fi.length; f++) 
			for (int v=0; v<fi[f].length; v++) 
				actualVOUV[unwrapFI[f][v]]=fi[f][v];
		return actualVOUV;
	}					
	
	/* faceLabels
	 * The faces are labeled by their indices.
	 */
	{
		faceLabels.addIngr(faceCount);
		faceLabels.setUpdateMethod(
				new OoNode.UpdateMethod() {
					public Object update( Object object) {					
						return indexString(nof());
					}					
				}
		);
	}

	/* aabbTree
	 * generate aabbTree according to AABBTree.construct()
	 */
	{
		aabbTree.addIngr(vertexCoordinates);
		aabbTree.addIngr(faceIndices);
		aabbTree.setUpdateMethod(
				new OoNode.UpdateMethod() {
					public Object update( Object object) {					
						return isGenerateAABBTree() ? AABBTree.construct((double[][])vertexCoordinates.getObject(), (int[][]) faceIndices.getObject()): null;
					}					
				});
	}

	/* EdgeIndices
	 * generate EdgeIndices according to IndexedFaceSetUtility.edgesFromFaces()
	 */
	{
		edgeIndices.addIngr( faceIndices );
		if( isGenerateEdgesFromFaces() )
			edgeIndices.addDeps( edgeCount );
		edgeIndices.setUpdateMethod(
				new OoNode.UpdateMethod() {
					public Object update( Object object) {					
						return generateEdgeIndices();		
					}					
				}
		);
				
	}
	/* overwrite in subclass. */
	int [][] generateEdgeIndices() {
		int [][] fi = (int[][])faceIndices.getObject();
		if( fi==null)
			return null;
		return IndexedFaceSetUtility.edgesFromFaces( fi ).toIntArrayArray(null);
	}
	
	/* edgeCount
	 * update edgeCount (see AbstractIndexedLineSet). 
	 */
	{
		edgeCount.setUpdateMethod( new UpdateMethod() {
			public Object update(Object object) {
				int count;
				if(edgeIndices.isGenerate())
					count = ((int[][])edgeIndices.getObject()).length;
				else // Never happens? (see setGenerateEdgesFromFaces() )
					count = edge.DLS.containsAttribute(Attribute.INDICES) ? edge.DLS.getListLength() : 0;
				return new Integer(count);	
			}			
		});
	}

    /* faceNormals
     * generate faceNormals according to IndexedFaceSetUtility.calculateFaceNormals()
     */
	{
		faceNormals.addIngr(metric);
		faceNormals.addIngr(faceIndices);
		faceNormals.addIngr(vertexCoordinates);
		faceNormals.addIngr(actualVertexOfUnwrapVertex);
		faceNormals.setUpdateMethod(
				new OoNode.UpdateMethod() {
					public Object update( Object object) {					
						return generateFaceNormals( (double[][])object);		
					}					
				}
		);
	}	
	/* overwrite in subclass. */
	double [][] generateFaceNormals( double [][] faceNormals ) {
		int    [][] fi = (int   [][])faceIndices.      getObject();
		double [][] vc = (double[][])vertexCoordinates.getObject();

		if( fi==null || vc==null )
			return null;
		
		log( "compute", Attribute.NORMALS, "face");
	
		return IndexedFaceSetUtility.calculateFaceNormals( fi, vc, getMetric() );
		
	}
	
    /* vertexNormals
     * generate vertexNormals according to IndexedFaceSetUtility.calculateVertexNormals()
     */
	{
		vertexNormals.addIngr(metric);
		vertexNormals.addIngr(faceNormals);
		
		vertexNormals.setUpdateMethod(
				new OoNode.UpdateMethod() {
					public Object update( Object object) {					
						return generateVertexNormals( (double[][])object);		
					}					
				}
		);
	}
	/* overwrite in subclass. */
	double [][] generateVertexNormals( double [][] vertexNormals ) {
		int    [][] fi = (int   [][])faceIndices.      getObject();
		double [][] vc = (double[][])vertexCoordinates.getObject();
		double [][] fn = (double[][])faceNormals.      getObject();
		
		if( fi==null || vc==null  )
			return null;
		
		if( fn==null ) { 
			fn = IndexedFaceSetUtility.calculateFaceNormals( fi, vc, getMetric() );
		}
		
		vertexNormals=IndexedFaceSetUtility.calculateVertexNormals( fi, vc, fn, getMetric() );
		
		/* add vertex normals to corresponding unwrap vertices */
		int[] translation=(int[]) actualVertexOfUnwrapVertex.getObject();
		if ( translation != null ) {
			for (int i=0; i<vertexNormals.length && i<translation.length; i++)
				if (i!=translation[i])
					System.arraycopy(vertexNormals[translation[i]], 0, vertexNormals[i], 0, vertexNormals[translation[i]].length);
		}

		return vertexNormals;

	}

	
	void recompute() {		
			
		super.recompute();
			
		aabbTree.update();
		
		actualVertexOfUnwrapVertex.update();
		
		faceLabels.update();
		edgeIndices.update();
		faceNormals.update();
		
		if (unwrapFaceIndices.getObject()==null  && nodeWasUpdated(unwrapFaceIndices)) 
				//unwrapFaceIndices where just set to null, 
				//so the usual vertexNormals have to be restored
				vertexNormals.outdate();		
		
		vertexNormals.update();
	}

	protected void updateImpl() {
	 		
		super.updateImpl();
		
		if( ifs.getNumFaces() != getFaceCount() )
			ifs.setNumFaces( getFaceCount() );
		
		updateGeometryAttributeCathegory( face );
		
		if( nodeWasUpdated(aabbTree))
			ifs.setGeometryAttributes(PickUtility.AABB_TREE, aabbTree.getObject());
		
		/*face indices */
		if (unwrapFaceIndices.getObject()!=null)
			// overwrite face indices with unwrap face indices 
			ifs.setFaceAttributes(
					Attribute.INDICES, 
					StorageModel.INT_ARRAY_ARRAY.createReadOnly( (int[][]) unwrapFaceIndices.getObject())
					);
		else if (faceIndices.getObject()!=null && nodeWasUpdated(unwrapFaceIndices)) 
			//unwrapFaceIndices was set to null since last update, so 
			//faceIndices have to be restored, even if they have not been updated. If faceIndices where updated
			//they where already set/restored by "updateGeometryAttributeCathegory( face )" above. 
			ifs.setFaceAttributes(
					Attribute.INDICES, 
					StorageModel.INT_ARRAY_ARRAY.createReadOnly( (int[][]) faceIndices.getObject())
					);
			
		
		edgeIndices.updateArray();

		faceLabels.updateArray();
		faceNormals.updateArray();

		vertexNormals.updateArray();
	}
	
	@SuppressWarnings("unchecked")
	protected <E> E[] unwrapVertexAttributes(E[] data) {
		return (E[]) unwrapVertexAttributes(data, data[0].getClass(), 1, this);
	}
	
	@SuppressWarnings("unchecked")
	protected static <E> E[] unwrapVertexAttributes(E[] data,
			int[][] faceIndices, int[][] unwrapFaceIndices, int numberOfUnwrapVertices) {
		return (E[]) unwrapVertexAttributes(data, data[0].getClass(), 
				1, faceIndices, unwrapFaceIndices, numberOfUnwrapVertices);
	}

	protected int[] unwrapVertexAttributes(int[] data, int entriesPerVertex) {
		return (int[]) unwrapVertexAttributes(data, int.class, entriesPerVertex, this);
	}
	
	protected static int[] unwrapVertexAttributes(int[] data, int entriesPerVertex,
			int[][] faceIndices, int[][] unwrapFaceIndices, int numberOfUnwrapVertices) {
		return (int[]) unwrapVertexAttributes(data, int.class, 
				entriesPerVertex, faceIndices, unwrapFaceIndices, numberOfUnwrapVertices);
	}

	protected boolean[] unwrapVertexAttributes(boolean[] data, int entriesPerVertex) {
		return (boolean[]) unwrapVertexAttributes(data, boolean.class, entriesPerVertex, this);
	}
	
	protected static boolean[] unwrapVertexAttributes(boolean[] data, int entriesPerVertex,
			int[][] faceIndices, int[][] unwrapFaceIndices, int numberOfUnwrapVertices) {
		return (boolean[]) unwrapVertexAttributes(data, boolean.class, 
				entriesPerVertex, faceIndices, unwrapFaceIndices, numberOfUnwrapVertices);
	}

	protected long[] unwrapVertexAttributes(long[] data, int entriesPerVertex) {
		if( data == null )
			throw new NullPointerException();
		return (long[]) unwrapVertexAttributes(data, long.class, entriesPerVertex, this);
	}
	
	protected static long[] unwrapVertexAttributes(long[] data, int entriesPerVertex,
			int[][] faceIndices, int[][] unwrapFaceIndices, int numberOfUnwrapVertices) {
		return (long[]) unwrapVertexAttributes(data, long.class, 
				entriesPerVertex, faceIndices, unwrapFaceIndices, numberOfUnwrapVertices);
	}

	protected float[] unwrapVertexAttributes(float[] data, int entriesPerVertex) {
		return (float[]) unwrapVertexAttributes(data, float.class, entriesPerVertex, this);
	}
	
	protected static float[] unwrapVertexAttributes(float[] data, int entriesPerVertex,
			int[][] faceIndices, int[][] unwrapFaceIndices, int numberOfUnwrapVertices) {
		return (float[]) unwrapVertexAttributes(data, float.class, 
				entriesPerVertex, faceIndices, unwrapFaceIndices, numberOfUnwrapVertices);
	}

	protected char[] unwrapVertexAttributes(char[] data, int entriesPerVertex) {
		return (char[]) unwrapVertexAttributes(data, char.class, entriesPerVertex, this);
	}
	
	protected static char[] unwrapVertexAttributes(char[] data, int entriesPerVertex,
			int[][] faceIndices, int[][] unwrapFaceIndices, int numberOfUnwrapVertices) {
		return (char[]) unwrapVertexAttributes(data, char.class, 
				entriesPerVertex, faceIndices, unwrapFaceIndices, numberOfUnwrapVertices);
	}

	protected double[] unwrapVertexAttributes(double[] data, int entriesPerVertex) {
		return (double[]) unwrapVertexAttributes(data, double.class, entriesPerVertex, this);
	}
	
	protected static double[] unwrapVertexAttributes(double[] data, int entriesPerVertex,
			int[][] faceIndices, int[][] unwrapFaceIndices, int numberOfUnwrapVertices) {
		return (double[]) unwrapVertexAttributes(data, double.class, 
				entriesPerVertex, faceIndices, unwrapFaceIndices, numberOfUnwrapVertices);
	}

	private static Object unwrapVertexAttributes(Object data, Class<?> clazz, 
			int entriesPerVertex, AbstractIndexedFaceSetFactory ifsf) {
		if (ifsf.faceIndices.getObject()==null || ifsf.unwrapFaceIndices.getObject() == null) 
			throw new IllegalStateException("set faceIndices and unwrapFaceIndices first, or use the static method instead");

		return unwrapVertexAttributes(
				data, clazz, 
				entriesPerVertex,  (int[][]) ifsf.faceIndices.getObject(), 
				(int[][]) ifsf.unwrapFaceIndices.getObject(), 
				ifsf.nov());
	}
	
	/* this method is a little complicated, because it is designed to be applicable to 
	 * arrays of Objects (which would be easy) AND primitives */
	private static Object unwrapVertexAttributes(Object data, Class<?> clazz, 
			final int entriesPerVertex, int[][] faceIndices, int[][] unwrapFaceIndices, 
			final int numberOfUnwrapVertices) {
		if (data == null || faceIndices == null || unwrapFaceIndices == null) 
			throw new NullPointerException();
		if (entriesPerVertex<1)
			throw new IllegalArgumentException();

		
		Object unwrapData = Array.newInstance(clazz, numberOfUnwrapVertices*entriesPerVertex);
		int[] translation = determineActualVertexOfUnwrapVertexImpl(
				faceIndices, unwrapFaceIndices, null, numberOfUnwrapVertices);
		
		for (int i=0; i<numberOfUnwrapVertices; i++) 
			for (int j=0; j<entriesPerVertex; j++)
			Array.set(unwrapData,i*entriesPerVertex + j,Array.get(data,translation[i]*entriesPerVertex + j));
		
		return unwrapData;
	}
	

}
