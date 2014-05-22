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
import java.util.logging.Level;

import de.jreality.math.Pn;
import de.jreality.math.Rn;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.DataList;
import de.jreality.util.LoggingSystem;

/**
 * 
 * @author schmies
 *
 */
public class AbstractQuadMeshFactory extends AbstractIndexedFaceSetFactory {
	
	AttributeGenerator textureCoordinates = attributeGeneratorNode( vertex, double[][].class, Attribute.TEXTURE_COORDINATES);
	
	final OoNode closedInVDirection = node( new Boolean(false),"closed in v" );
	final OoNode closedInUDirection = node( new Boolean(false),"closed in u" );

	final OoNode vLineCount = node( new Integer(-1), "v-line count" );  //unfortunately we need the -1
	final OoNode uLineCount = node( new Integer(-1), "u-line count" );
	
	private int uLineCount_;
		
	double uTextureScale = 1;
	double vTextureScale = 1;
	
	double uTextureShift = 0;
	double vTextureShift = 0;
	
	final OoNode edgeFromQuadMesh = node( new Boolean(false),"one edge per parametric curve" );
	
	AbstractQuadMeshFactory() {
		this( Pn.EUCLIDEAN, 10, 10, false, false );
	}
	
	AbstractQuadMeshFactory(IndexedFaceSet existing) {
		this( existing, Pn.EUCLIDEAN, 10, 10, false, false );
	}
	
	// don't scare away the customers with the metric thing...
	AbstractQuadMeshFactory( int mMaxU, int mMaxV, boolean closeU, boolean closeV ) {
		this(Pn.EUCLIDEAN, mMaxU, mMaxV, closeU, closeV);
	}
	
	AbstractQuadMeshFactory( int metric, int mMaxU, int mMaxV, boolean closeU, boolean closeV ) {
		this(new IndexedFaceSet(), metric, mMaxU, mMaxV, closeU, closeV);
	}
	
	AbstractQuadMeshFactory( IndexedFaceSet existing, int metric, int mMaxU, int mMaxV, boolean closeU, boolean closeV ) {
		super(existing == null ? new IndexedFaceSet() : existing, metric );
		
		setMeshSize( mMaxU, mMaxV );

		setClosedInUDirection(closeU);
		setClosedInVDirection(closeV);
		
		faceIndices.setGenerate(true);
		setGenerateTextureCoordinates(true);

		edgeIndices.addIngr( faceIndices );
		edgeIndices.addIngr( edgeFromQuadMesh );
		edgeIndices.setUpdateMethod(
				new OoNode.UpdateMethod() {
					public Object update( Object object) {		
						return generateEdgeIndices( );					
					}					
				}
		);

		faceIndices.addIngr( uLineCount );
		faceIndices.addIngr( vLineCount );
		faceIndices.setUpdateMethod(
				new OoNode.UpdateMethod() {
					public Object update( Object object) {		
						return generateFaceIndices( (int[][])object);					
					}					
				}
		);

		textureCoordinates.addIngr( uLineCount );
		textureCoordinates.addIngr( vLineCount );
		textureCoordinates.setUpdateMethod(
				new OoNode.UpdateMethod() {
					public Object update( Object object) {					
						return generateTextureCoordinates( (double[][])object);					
					}					
				}
		);
	}
	
	void setMeshSize(int maxU2, int maxV2) {
		if( maxU2 < 2 || maxV2 < 2 )
			throw new IllegalArgumentException( "line count must be bigger then 1" );
		
		if( maxU2 == getULineCount() && maxV2 == getVLineCount() )
			return;
		
		uLineCount.setObject(new Integer(maxU2));  uLineCount_ = maxU2; // ugly, but we need this for fast access
		vLineCount.setObject(new Integer(maxV2));
					
		super.setVertexCount( getULineCount()*getVLineCount());
		super.setFaceCount( (getULineCount()-1)*(getVLineCount()-1) );
		
	}

	public void setVertexCount( int count ) {
		throw new UnsupportedOperationException();
	}
	
	public void setFaceCount( int count ) {
		throw new UnsupportedOperationException();
	}
	
	public void setFaceAttribute( Attribute attr, DataList data ) {

		if( attr == Attribute.INDICES ) {			
			throw new UnsupportedOperationException( "cannot set indices of a quad mesh");
		}
		
		super.setFaceAttribute( attr, data );
	}
	
	int [][] generateEdgeIndices( ) {
		if (!((Boolean) edgeFromQuadMesh.getObject()).booleanValue()) 
			return super.generateEdgeIndices();
		
		int uLineCount = getULineCount();
		int vLineCount = getVLineCount();
		
		int sizeUCurve = vLineCount;
		int sizeVCurve = uLineCount;
		int numVerts = getULineCount()*getVLineCount();
		int nbOfEdges = uLineCount +vLineCount;
		
		int[][] indices = new int[nbOfEdges][];
		for (int i = 0; i<uLineCount; ++i)	{
			indices[i] = new int[sizeUCurve];
			for (int j = 0; j< sizeUCurve; ++j)	  indices[i][j] = ((j)*uLineCount + (i%uLineCount))%numVerts;
		}
		for (int i = 0; i<vLineCount; ++i)	{
			indices[i+uLineCount] = new int[sizeVCurve];
			for (int j = 0; j< sizeVCurve; ++j)	  indices[i+uLineCount][j] = (i*uLineCount + (j%uLineCount))%numVerts;
		}	
		return indices;
	}
	
	int [][] generateFaceIndices( int [][] faceIndices ) {
			
		final int uLineCount = getULineCount();
		final int vLineCount = getVLineCount();
	
		log("compute", Attribute.INDICES, "face");
		if( faceIndices == null || nof() != faceIndices.length )
			faceIndices = new int[nof()][4];
		
		final int numUFaces = uLineCount-1;
		final int numVFaces = vLineCount-1;
		
		final int numPoints = nov();
		
		for (int i = 0, k=0; i<numVFaces; ++i) {
			for (int j = 0; j< numUFaces; ++j, k++)	{
				final int [] face = faceIndices[k];
				face[0] = (i * uLineCount + j);
				face[1] = (((i+1) * uLineCount) + j) % numPoints;
				face[2] = (((i+1) * uLineCount) + (j+1)%uLineCount) % numPoints;
				face[3] = ((i* uLineCount) + (j+1)%uLineCount) % numPoints;				
			}
		}

		return faceIndices;
	}
	
	public int getULineCount() {
		return ((Integer)uLineCount.getObject()).intValue();
	}
	
	public int getVLineCount() {
		return ((Integer)vLineCount.getObject()).intValue();
	}
	
	public void setULineCount(int newU) {
		setMeshSize( newU, getVLineCount() );
	}
	
	public void setVLineCount(int newV) {
		setMeshSize( getULineCount(), newV );
	}
	
	public boolean isClosedInUDirection() {
		return ((Boolean)closedInUDirection.getObject()).booleanValue();
	}
	
	public boolean isClosedInVDirection() {
		return ((Boolean)closedInVDirection.getObject()).booleanValue();
	}
	
	public void setClosedInUDirection(boolean close) {
		closedInUDirection.setObject( new Boolean(close));
	}
	
	public void setClosedInVDirection(boolean close) {
		closedInVDirection.setObject( new Boolean(close));
	}

	double [][] generateTextureCoordinates( double [][] textureCoordinates ) {
		
		if( vertex.DLS.containsAttribute(Attribute.TEXTURE_COORDINATES))
			return null;
		
		if( textureCoordinates == null || textureCoordinates.length != nov() )
			textureCoordinates = new double[nov()][2];
		
		final int vLineCount = getVLineCount();
		final int uLineCount = getULineCount();
		
		final double dv= 1.0 / (vLineCount - 1) * vTextureScale;
		final double du= 1.0 / (uLineCount - 1) * uTextureScale;
		
		double v=0;
		for(int iv=0, firstIndexInULine=0;
		iv < vLineCount;
		iv++, v+=dv, firstIndexInULine+=uLineCount) {
			double u=0;
			for(int iu=0; iu < uLineCount; iu++, u+=du) {
				final double [] xy = textureCoordinates[firstIndexInULine + iu];
				xy[0] = u + uTextureShift;
				xy[1] = v + vTextureShift;
			}
		}
				
		return textureCoordinates;
	}

	void recompute() {
		
		super.recompute();
	
		faceIndices.update();
		textureCoordinates.update();
		
	}
	
	protected void updateImpl() {
	
		super.updateImpl();
	
		log( "set", GeometryUtility.QUAD_MESH_SHAPE, "vertex");
			
		ifs.setGeometryAttributes(GeometryUtility.QUAD_MESH_SHAPE, new Dimension( getULineCount(), getVLineCount() ));

		edgeIndices.updateArray();
		faceIndices.updateArray();
		textureCoordinates.updateArray();
	}

	public boolean isGenerateTextureCoordinates() {
		return textureCoordinates.isGenerate();
	}

	public void setGenerateTextureCoordinates(boolean generateTextureCoordinates) {
		textureCoordinates.setGenerate(generateTextureCoordinates);
	}

	final private int index( int u, int v ) {
		return u + uLineCount_ * v;
	}
	
	void average( double [] x, double [] y ) {
		Rn.add(x,x,y);
		Rn.times(x,2,x); //unnecessary because we normalize later
		Rn.copy(y,x);
	}
	
	void normalize( double [] x ) {
		Rn.normalize(x,x);
	}
	
	{
		vertexNormals.addIngr(closedInUDirection);
		vertexNormals.addIngr(closedInVDirection);
	}
	
	double [][] generateVertexNormals( double [][] vertexNormals ) {
		vertexNormals = super.generateVertexNormals( vertexNormals );
		
		if( !isClosedInUDirection() && !isClosedInVDirection() )
			return vertexNormals;
		
		if( getMetric() != Pn.EUCLIDEAN )
			LoggingSystem.getLogger(this).log(Level.WARNING, 
					"currently only eucledian normals used for smoothing");
		
		if( isClosedInUDirection() ) {		
			final int last = getULineCount()-1;		
			for( int i=0; i<getVLineCount(); i++ ) {
				average( vertexNormals[index(0,i)], vertexNormals[index(last,i)]);
			}
		}
		
		if( isClosedInVDirection() ) {		
			final int last = getVLineCount()-1;		
			for( int i=0; i<getULineCount(); i++ ) {
				average( vertexNormals[index(i,0)], vertexNormals[index(i,last)]);
				normalize(vertexNormals[index(i,0)]);
				normalize(vertexNormals[index(i,last)]);
			}
		}
		
		if( isClosedInUDirection() ) {		
			final int last = getULineCount()-1;		
			for( int i=0; i<getVLineCount(); i++ ) {
				normalize( vertexNormals[index(0,i)]);
				normalize( vertexNormals[index(last,i)]);
			}
		}
		
		return vertexNormals;
	}

	public double getUTextureScale() {
		return uTextureScale;
	}

	public void setUTextureScale(double textureScale) {
		if( this.uTextureScale == textureScale )
			return;
		
		uTextureScale = textureScale;
		
		textureCoordinates.outdate();
	}

	public double getVTextureScale() {
		return vTextureScale;
	}

	public void setVTextureScale(double textureScale) {
		if( this.vTextureScale == textureScale )
			return;
		
		vTextureScale = textureScale;
		
		textureCoordinates.outdate();
	}

	public double getUTextureShift() {
		return uTextureShift;
	}

	public void setUTextureShift(double textureShift) {
		if( this.uTextureShift == textureShift )
			return;
		
		uTextureShift = textureShift;
		
		textureCoordinates.outdate();
	}

	public double getVTextureShift() {
		return vTextureShift;
	}

	public void setVTextureShift(double textureShift) {
		if( this.vTextureShift == textureShift )
			return;
		
		vTextureShift = textureShift;
		
		textureCoordinates.outdate();
	}

	public boolean isEdgeFromQuadMesh() {
		return ((Boolean)edgeFromQuadMesh.getObject()).booleanValue();

	}

	public void setEdgeFromQuadMesh(boolean b) {
		edgeFromQuadMesh.setObject( new Boolean(b));
	}

}
