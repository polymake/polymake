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

// TODO:  no support for setting edge attributes

import java.awt.Color;

import de.jreality.math.Pn;
import de.jreality.scene.Geometry;
import de.jreality.scene.IndexedLineSet;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.DataList;
import de.jreality.scene.data.DataListSet;
import de.jreality.scene.data.DoubleArray;
import de.jreality.scene.data.DoubleArrayArray;
import de.jreality.scene.data.IntArrayArray;
import de.jreality.scene.data.StorageModel;
import de.jreality.scene.data.StringArray;

public class AbstractIndexedLineSetFactory extends AbstractPointSetFactory {
	
	final IndexedLineSet ils;
	
	GeometryAttributeListSet edge = new GeometryAttributeListSet( this, Geometry.CATEGORY_EDGE );
	
	OoNode edgeCount = node( "edgeCount", Integer.class, 0 );
		
	AttributeGenerator edgeLabels = attributeGeneratorNode( edge, String[].class, Attribute.LABELS );
	
	AbstractIndexedLineSetFactory( IndexedLineSet ils, int metric ) {
		super( ils, metric );	

		this.ils = ils;
	}
	
	AbstractIndexedLineSetFactory( int metric ) {
		this( new IndexedLineSet(0,0), metric );
	}
	
	AbstractIndexedLineSetFactory(IndexedLineSet existing) {
		this(  existing, Pn.EUCLIDEAN );
	}
	
	public AbstractIndexedLineSetFactory() {
		this( new IndexedLineSet(0,0), Pn.EUCLIDEAN );
	}
	
	protected int noe(){
		return (Integer)edgeCount.getObject();
	}
	
	public int getEdgeCount() {
		return noe();
	}
	
	void setEdgeCount( int count ) {
		edge.setCount(count);
		edgeCount.setObject(new Integer(count));
	}
	
	/** 
	 * @deprecated Use {@link #setEdgeCount(int)}.
	 */
	public int getLineCount() { 
		return getEdgeCount();
	}
	
	/**
	 * @deprecated Use {@link #setEdgeCount(int)}.
	 */
	public void setLineCount(int count) {
		setEdgeCount(count);
	}
	
	protected void setEdgeAttribute( Attribute attr, DataList data ) {
		edge.setAttribute(attr,data);
	}
	
	protected void setEdgeAttribute(Attribute attr, double [] data ) {
		if( data != null && (noe() == 0 && data.length != 0 || data.length % noe() != 0) )
			throw new IllegalArgumentException( "array has wrong length" );
		setEdgeAttribute( attr, data==null ? null : new DoubleArrayArray.Inlined( data, data.length / nov() ) );
	}
	
	protected void setEdgeAttribute(Attribute attr,  double [][] data ) {
		setEdgeAttribute( attr,
				StorageModel.DOUBLE_ARRAY.array(data[0].length).createReadOnly(data));
	}
	
	protected void setEdgeAttributes(DataListSet dls ) {
		edge.setAttributes(dls);
	}
	
	protected void setEdgeIndices( DataList data ) {
		setEdgeAttribute( Attribute.INDICES, data );
	}
	
	protected void setEdgeIndices( int[][] data ) {
		setEdgeAttribute( Attribute.INDICES, new IntArrayArray.Array( data ) );
	}
	
	protected void setEdgeIndices( int[] data, int pointCountPerLine ) {
		if( data != null && data.length != pointCountPerLine * noe() )
			throw new IllegalArgumentException( "array has wrong length" );
		setEdgeAttribute( Attribute.INDICES, data == null ? null : new IntArrayArray.Inlined( data, pointCountPerLine ) );
	}
	
	protected void setEdgeIndices( int[] data ) {
		setEdgeIndices( data, 2 );
	}
	
	protected void setEdgeNormals( DataList data ) {
		setEdgeAttribute( Attribute.NORMALS, data );
	}
	
	protected void setEdgeNormals( double [] data ) {
		if( data != null && data.length % noe() != 0 )
			throw new IllegalArgumentException( "array has wrong length" );	
		setEdgeAttribute( Attribute.NORMALS, data == null ? null : new DoubleArrayArray.Inlined( data, data.length / noe() ) );
	}
	
	protected void setEdgeNormals( double [][] data ) {
		setEdgeAttribute( Attribute.NORMALS, new DoubleArrayArray.Array( data ) );
	}
	
	protected void setEdgeColors( DataList data ) {
		setEdgeAttribute( Attribute.COLORS, data );
	}
	
	protected void setEdgeColors( double [] data ) {
		if( data != null && data.length % noe() != 0 )
			throw new IllegalArgumentException( "array has wrong length" );	
		setEdgeAttribute( Attribute.COLORS, data == null ? null : new DoubleArrayArray.Inlined( data, data.length / noe() ) );
	}
	
	protected void setEdgeColors( Color [] data ) {
		setEdgeColors( toDoubleArray(data));
	}
	
	protected void setEdgeColors( double [][] data ) {
		setEdgeAttribute( Attribute.COLORS, new DoubleArrayArray.Array( data ) );
	}


	protected void setEdgeLabels( DataList data ) {
		setVertexAttribute( Attribute.LABELS, data );
	}
	
	
	protected void setEdgeLabels( String[] data ) {
		if( data != null && data.length != noe() )
			throw new IllegalArgumentException( "array has wrong length" );
		setEdgeAttribute( Attribute.LABELS, data == null ? null : new StringArray(data));
	}

	protected void setEdgeRelativeRadii( DataList data ) {
		setEdgeAttribute( Attribute.RELATIVE_RADII, data );
	}
	
	protected void setEdgeRelativeRadii( double [] data ) {
		if( data != null && data.length != noe() )
			throw new IllegalArgumentException( "array has wrong length" );
		setEdgeAttribute( Attribute.RELATIVE_RADII, data == null ? null : new DoubleArray(data));
	}

	{
		edgeLabels.addIngr(edgeCount);
		edgeLabels.setUpdateMethod(
				new OoNode.UpdateMethod() {
					public Object update( Object object) {					
						return indexString( (Integer)(edgeCount.getObject()) );		
					}					
				}
		);
	}
	
	void recompute() {		
	
		super.recompute();
		
		if( isGenerateEdgeLabels() )
			edgeLabels.update();

	}
	
	
	protected void updateImpl() {
		
		super.updateImpl();
		
		if( ils.getNumEdges() != getEdgeCount() )
			ils.setNumEdges( getEdgeCount() );
		
		updateGeometryAttributeCathegory( edge );

		edgeLabels.updateArray();
	}
	
	public IndexedLineSet getIndexedLineSet() {
		return ils;
	}

	public boolean isGenerateEdgeLabels() {
		return edgeLabels.isGenerate();
	}

	public void setGenerateEdgeLabels(boolean generateEdgeLabels) {
		edgeLabels.setGenerate(generateEdgeLabels);
	}

}
