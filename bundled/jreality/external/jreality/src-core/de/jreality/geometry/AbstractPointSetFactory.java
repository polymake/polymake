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

import de.jreality.math.Pn;
import de.jreality.scene.Geometry;
import de.jreality.scene.PointSet;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.DataList;
import de.jreality.scene.data.DataListSet;
import de.jreality.scene.data.DoubleArray;
import de.jreality.scene.data.DoubleArrayArray;
import de.jreality.scene.data.StorageModel;
import de.jreality.scene.data.StringArray;

public class AbstractPointSetFactory extends AbstractGeometryFactory {
	
	final PointSet ps;

	final GeometryAttributeListSet vertex = new GeometryAttributeListSet( this, Geometry.CATEGORY_VERTEX );

	OoNode vertexCount = node( "vertexCount", Integer.class, 0 );
	
	AttributeGenerator vertexLabels = attributeGeneratorNode( vertex, String[].class, Attribute.LABELS );
	
	AbstractPointSetFactory( PointSet ps, int metric ) {
		
		super( ps, metric );
		
		this.ps = ps;
	}

	public AbstractPointSetFactory() {
		this( new PointSet(), Pn.EUCLIDEAN);
	}
	
	int nov(){
		return (Integer)vertexCount.getObject();
	}
	
	public int getVertexCount() {
		return nov();
	}
	
	public void setVertexCount( int count ) {
		vertex.setCount( count );
		vertexCount.setObject(new Integer(count));
	}
	
	protected void setVertexAttribute( Attribute attr, DataList data ) {
		vertex.setAttribute( attr, data );
	}
	
	protected void setVertexAttribute(Attribute attr, double [] data ) {
		if( data != null && (nov() == 0 && data.length != 0 || data.length % nov() != 0) )
			throw new IllegalArgumentException( "array has wrong length" );
		setVertexAttribute( attr, data==null ? null : new DoubleArrayArray.Inlined( data, data.length / nov() ) );
	}
	
	public void setVertexAttribute(Attribute attr,  double [][] data ) {
		setVertexAttribute( attr,
				StorageModel.DOUBLE_ARRAY.array(data[0].length).createReadOnly(data));
	}
	
	protected void setVertexAttributes( DataListSet dls ) {
		vertex.setAttributes( dls );
	}
	
	protected void setVertexCoordinates( DataList data ) {
		setVertexAttribute( Attribute.COORDINATES, data );
	}
	
	protected void setVertexCoordinates( double [] data ) {
		if( data != null && (nov() == 0 && data.length != 0 || data.length % nov() != 0) )
			throw new IllegalArgumentException( "array has wrong length" );
		setVertexAttribute( Attribute.COORDINATES, data==null ? null : new DoubleArrayArray.Inlined( data, data.length / nov() ) );
	}
	
	protected void setVertexCoordinates( double [][] data ) {
		setVertexAttribute( Attribute.COORDINATES,
				StorageModel.DOUBLE_ARRAY.array(data[0].length).createReadOnly(data));
	}
	
	protected void setVertexNormals( DataList data ) {
		setVertexAttribute( Attribute.NORMALS, data );
	}
	
	protected void setVertexNormals( double [] data ) {
		if( data != null && data.length % nov() != 0 )
			throw new IllegalArgumentException( "array has wrong length" );
		setVertexAttribute( Attribute.NORMALS, data == null ? null : new DoubleArrayArray.Inlined( data,  data.length / nov() ) );
	}
	
	protected void setVertexNormals( double [][] data ) {
		setVertexAttribute( Attribute.NORMALS,  data == null ? null :  new DoubleArrayArray.Array( data, data[0].length ) );
	}
	
	protected void setVertexColors( DataList data ) {
		setVertexAttribute( Attribute.COLORS, data );
	}
	
	protected void setVertexColors( double [] data ) {
		if( data != null && data.length % nov() != 0 )
			throw new IllegalArgumentException( "array has wrong length" );
		setVertexAttribute( Attribute.COLORS, data == null ? null : new DoubleArrayArray.Inlined( data, data.length / nov() )  );
	}
	
	protected void setVertexColors( Color [] data ) {
		setVertexColors( toDoubleArray( data ) );
	}
	
	protected void setVertexColors( double [][] data ) {
		setVertexAttribute( Attribute.COLORS, data == null ? null : new DoubleArrayArray.Array( data, data[0].length ) );
	}
	
	protected void setVertexTextureCoordinates( DataList data ) {
		setVertexAttribute( Attribute.TEXTURE_COORDINATES, data );
	}
	
	protected void setVertexTextureCoordinates( double [] data ) {
		if( data != null && data.length % nov() != 0 )
			throw new IllegalArgumentException( "array has wrong length" );
		setVertexAttribute( Attribute.TEXTURE_COORDINATES, data==null ? null : new DoubleArrayArray.Inlined( data, data.length / nov() ) );
	}
	
	protected void setVertexTextureCoordinates( double [][] data ) {
		setVertexAttribute( Attribute.TEXTURE_COORDINATES, new DoubleArrayArray.Array( data, data[0].length ) );
	}

	protected void setVertexLabels( DataList data ) {
		setVertexAttribute( Attribute.LABELS, data );
	}
	
	protected void setVertexLabels( String[] data ) {
		if( data != null && data.length != nov() )
			throw new IllegalArgumentException( "array has wrong length" );
		setVertexAttribute( Attribute.LABELS, data == null ? null : new StringArray(data));
	}
	
	protected void setVertexRelativeRadii( DataList data ) {
		setVertexAttribute( Attribute.RELATIVE_RADII, data );
	}
	
	protected void setVertexRelativeRadii( double [] data ) {
		if( data != null && data.length != nov() )
			throw new IllegalArgumentException( "array has wrong length" );
		setVertexAttribute( Attribute.RELATIVE_RADII, data == null ? null : new DoubleArray(data));
	}
	
	String [] indexString(int nov) {
		if( nov == 0 ) return null;
		String [] labels = new String[nov];
		for( int i=0; i<nov; i++ ) {
			labels[i]=Integer.toString(i);
		}
		return labels;
	}

	{
		vertexLabels.addIngr(vertexCount);
		vertexLabels.setUpdateMethod(
				new OoNode.UpdateMethod() {
					public Object update( Object object) {					
						return indexString(nov());		
					}					
				}
		);
	}
	
	void recompute() {	
		super.recompute();				
		vertexLabels.update();	
	}

	
	void updateImpl() {
		super.updateImpl();
		
		if( ps.getNumPoints() != getVertexCount() )	
			ps.setNumPoints( getVertexCount());
		
		updatePointSet();
	}
	
	void updatePointSet() {
		
		updateGeometryAttributeCathegory( vertex );
		
		vertexLabels.updateArray();
		
	}
	
	public PointSet getPointSet() {
		return ps;
	}
	
	public boolean isGenerateVertexLabels() {
		return vertexLabels.isGenerate();
	}

	public void setGenerateVertexLabels(boolean generateVertexLabels) {
		vertexLabels.setGenerate(generateVertexLabels);
	}
	
	OoNode node( String name, Class type, Object value ) {
		OoNode node = new OoNode(name, type, this.update);
		node.setObject(value);
		return node;
	}
	
	OoNode node( String name, Class type ) {
		return new OoNode(name, type, this.update);
	}
	
	AttributeGenerator attributeGeneratorNode( GeometryAttributeListSet gals, Class type, Attribute attr ) {
		return new AttributeGenerator(  gals, type, attr, this );
	}
	
	
	
	static class AttributeGenerator extends OoNode {
	
		final GeometryAttributeListSet gals;
		final Attribute attr;
		final AbstractGeometryFactory factory;
		
		boolean generate=false;
		
		AttributeGenerator( GeometryAttributeListSet gals, Class type, Attribute attr, AbstractGeometryFactory factory ) {
			super(gals.category.toLowerCase()+"."+attr.getName(), type, factory.update );
			//super(gals.category+"."+attr.getName(), type, factory.update );
			this.gals = gals;
			this.attr = attr;
			this.factory = factory;

			this.addIngr( this.gals.attributeNode(attr) );
			
			setUpdateMethod(null);
		}
		
		public void updateArray() {
			factory.updateNode( gals, attr, generate, this );
		}
		
		public void update() {
			//if( generate ) 
				super.update();
			
		}

		public void setUpdateMethod(final UpdateMethod method) {
			super.setUpdateMethod(
					new OoNode.UpdateMethod() {

						public Object update(Object object) {
							if( generate && method != null )
								return method.update(object);
							
							DataList dl = AttributeGenerator.this.gals.DLS.getList(AttributeGenerator.this.attr);
							
							return dl==null?null:converteDataListToArray(dl);
						}
						
					}
			);	
		}

		boolean isGenerate() {
			return generate;
		}
		
		void setGenerate(boolean generate) {
			if( this.generate==generate)
				return;
			
			this.outdate();
			
			this.generate = generate;
			
			if( generate ) {
				if( gals.DLS.containsAttribute( attr))
					throw new UnsupportedOperationException( "you cannot not generate the attribute " + attr +
							"because it is explicitly set. Unset it first." );
				gals.blockAttribute( attr );
			} else {
				gals.unblockAttribute( attr );
			}
		}
	}
	
	static double [] toDoubleArray( Color [] color ) {
		float [] c = new float[5];
		double [] array = new double[color.length * 4 ];
		for( int i=0, j=0; i<array.length; i+=4, j++ ) {
			color[j].getComponents(c);
			array[i+0] = c[0];
			array[i+1] = c[1];
			array[i+2] = c[2];
			array[i+3] = c[3];
		}		
		return array;
	}
}
