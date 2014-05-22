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

import de.jreality.scene.IndexedLineSet;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.DataList;
import de.jreality.scene.data.DataListSet;

/**
 * This is a factory class for constructing and editing instances of {@link IndexedLineSet}. For an introduction
 * to the way these geometry factories work, see the documentation for the superclass {@link PointSetFactory}.
 * <p>
 * In addition to the functionality inherited from PointSetFactory, this class offers methods to set and edit
 * the edge information of an IndexedLineSet.  (Here <i>edge</i> is synonymous with 'line'.)  This is first and foremost
 * an array of type <code>int[][]</code> which specifies the combinatorics: which vertices are connected by edges.
 * First call {@link #setEdgeCount(int)} to set the number of edges, then
 * use the method {@link #setEdgeIndices(int[][])} and its variants to set this information.
 * <p>
 * There are also methods for setting the builtin attributes colors and labels. For other attributes, use the
 * methods:
 *  * For attributes not included in the built-in set, use the methods
 * <ul>
 * <li>{@link #setEdgeAttribute(Attribute, DataList)}</li>
 * <li>{@link #setEdgeAttribute(Attribute, double[])}</li>
 * <li>{@link #setEdgeAttribute(Attribute, double[][])}</li>
 * </ul>
 * <p>
 * You can also request the factory to automatically generate edge labels (which are strings displayed 
 * as 3D text at the midpoint of the edges) using the method {@link #setGenerateEdgeLabels(boolean)}. This 
 * will generate labels showing the index of the edge within the edge array. (Probably only works correctly
 * when all edges consist of two points).
 * <p>
 *  For an example, see
 * <a href=http://www3.math.tu-berlin.de/jreality/mediawiki/index.php/Use_an_indexed_line_set_factory> this tutorial</a>.
 * <p>
 * @author gunn
 *
 */
public class IndexedLineSetFactory extends AbstractIndexedLineSetFactory {

	public IndexedLineSetFactory() {
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
	
	/**
	 * It's not documented why, but the superclass methods are protected, so we
	 * have to implement these as public
	 */
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

   public void setEdgeIndices( DataList data ) {
        super.setEdgeIndices(data);
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
		super.setEdgeColors(data);
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
	

}
