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

import de.jreality.scene.PointSet;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.DataList;
import de.jreality.scene.data.DataListSet;

/**
 * 
 * A factory class for creating and updating point sets.  See {@link PointSet} for details of the
 * underlying point set. 
 * <p>
 * There are different formats which can be used to set this data, and corresponding methods for setting
 * this data.  You must first set the number of vertices using {@link #setVertexCount(int)}. Then 
 * vertex attributes can be set.  For example to set the attribute {@link Attribute#COORDINATES}:
 * <ul>
 * <li>{@link #setVertexCoordinates(DataList)}</li>
 * <li>{@link #setVertexCoordinates(double[])}</li>
 * <li>{@link #setVertexCoordinates(double[][])}</li>
 * </ul>
 * <p>
 * In the case the argument is <code>double[]</code> then the length must be length 3 or 4 times the number of vertices.
 * (Point sets can have either ordinary or homogeneouse coordinates).  This vector length (3 or 4) is sometimes called the 
 * <i>fiber length</i> of the data.
 * <p>
 * There are analogous methods for setting the built-in attributes normals, colors, texture coordinates, relative radii, and labels.
 * Texture coordinates can have fiber length 2, 3, or 4.  Normals in euclidean case must have fiber length 3; otherwise they should have 
 * length 4. Labels are represented by an array of type <code>String[]</code>.
 * <p>
 * For attributes not included in the built-in set, use the methods
 * <ul>
 * <li>{@link #setVertexAttribute(Attribute, DataList)}</li>
 * <li>{@link #setVertexAttribute(Attribute, double[])}</li>
 * <li>{@link #setVertexAttribute(Attribute, double[][])}</li>
 * </ul>
 * <p>
 * Each instance of a factory always acts on the same instance of {@link PointSet}.  To get this instance use the
 * method {@link #getPointSet()}.  To edit the instance, call the various set methods, then call {@link #update()}.
 * You don't need to call {@link #getPointSet()} again, as it always returns the same value.
 * <p>
 * You can also request the factory to automatically generate vertex labels (which are strings displayed 
 * as 3D text at the position of the vertices) using the method {@link #setGenerateVertexLabels(boolean)}. This 
 * will generate labels showing the index of the point within the point array.
 * <p>
 * The abstract superclass {@link AbstractPointSetFactory} is not visible publicly. 
 * It provides protected methods to subclasses.  Subclasses such as this one
 * can then decide which of these methods to reveal by implementing them as public 
 * methods.
 * <p>
 * To specify that the geometry should be handled according to non-euclidean metric, use the method {@link #setMetric(int)}. 
 * This has little significance for the point sets, but becomes more important for subclasses such as {@link IndexedFaceSetFactory}.
 * <p>
 * This class writes itself 
 *  For an example, see
 * <a href=http://www3.math.tu-berlin.de/jreality/mediawiki/index.php/Use_a_point_set_factory> this tutorial</a>.
 * <p>
 * See {@link DataList}.
 *
 * @author gunn
 *
 */
public class PointSetFactory extends AbstractPointSetFactory {

	public PointSetFactory() {
		super();
	}
	
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
	
	public PointSet getPointSet() {
		return ps;
	}
	
}
