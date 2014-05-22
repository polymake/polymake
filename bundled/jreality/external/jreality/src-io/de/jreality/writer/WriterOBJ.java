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


package de.jreality.writer;

import java.io.OutputStream;
import java.io.PrintWriter;
import java.util.logging.Level;

import de.jreality.geometry.GeometryUtility;
import de.jreality.math.Pn;
import de.jreality.scene.Geometry;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.DataList;
import de.jreality.scene.data.IntArray;
import de.jreality.util.LoggingSystem;
import de.jreality.util.SceneGraphUtility;

/**
 * @author schmies
 * 
 */
public class WriterOBJ {

	public static int write( IndexedFaceSet ifs, OutputStream out, int startVertex ) {
		return write( ifs, null, new PrintWriter( out ), startVertex);
	}
	
	public static int write( IndexedFaceSet ifs, OutputStream out) {
		return write( ifs, null, new PrintWriter( out ), 0);
	}
	
	static void write( PrintWriter out, double [] array, String seperator ) {
		if( array==null || array.length==0) return;
		out.print(array[0]);
		for( int i=1; i<array.length; i++ ) {
			out.print(seperator);
			out.print(array[i]);
		}
	}
	
	static void write( PrintWriter out, double [][] array, String prefix ) {
		if( array==null) return;
		String seperator = " ";
		for( int i=0; i<array.length; i++ ) {
			out.print(prefix);
			out.print( seperator );
			write(out, array[i], seperator );
			out.println();
		}
	}

	static int write( Geometry geom, String groupName, PrintWriter out, int startVertex ) {
		if( geom == null ) return 0;
		
		if( geom instanceof IndexedFaceSet ) {
			return write( ((IndexedFaceSet)geom), groupName, out, startVertex);
		} else {
			 LoggingSystem.getLogger(GeometryUtility.class).log(Level.WARNING, 
					 	"ignoring scene graph component " + groupName );
		}
		return 0;
	}
	

	public static void write( SceneGraphComponent sgc, OutputStream out) {
		write( sgc, new PrintWriter( out ));
	}
	
	public static void write( SceneGraphComponent sgc, PrintWriter out ) {
		
		SceneGraphComponent flat = SceneGraphUtility.flatten(sgc);
		
		int vertex = write( flat.getGeometry(), flat.getName(), out, 0);
		
		final int noc = flat.getChildComponentCount();
			
		for( int i=0; i<noc; i++ ) {
			SceneGraphComponent child=flat.getChildComponent(i);
			vertex += write( child.getGeometry(), child.getName(), out, vertex);
		}
	}
	
	static void writeFaceIndex( PrintWriter out, int index, boolean hasTexture, boolean hasNormals ) {
		out.print(index+1);
		if( !hasTexture && !hasNormals ) return;
		out.print("/");
		if( hasTexture ) out.print(index+1);
		if( !hasNormals ) return;
		out.print("/");
		out.print(index+1);
	}

	static int write( IndexedFaceSet ifs, String groupName, PrintWriter out, int startVertex ) {
		
		if( groupName != null ) {
			out.println();	
			out.println( "g " + groupName );
		    out.println();
		}

		double [][] points = ifs.getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray(null);
		if (points[0].length == 4)	{
			// dehomogenize!
			double[][] points3 = new double[points.length][3];
			Pn.dehomogenize(points3, points);
			points = points3;
		}
        double [][] normals = null;
		if( ifs.getVertexAttributes( Attribute.NORMALS ) != null ) {
			try {
			normals = ifs.getVertexAttributes(Attribute.NORMALS).toDoubleArrayArray(null);
			} catch (NullPointerException e) {
				System.err.println("Skipped normals WriterOBJ.write(): Null value normal data");
			}
		}
		double [][] texture = null;
		if( ifs.getVertexAttributes( Attribute.TEXTURE_COORDINATES ) != null ) {
			try {
				texture = ifs.getVertexAttributes(Attribute.TEXTURE_COORDINATES).toDoubleArrayArray(null);
			} catch (NullPointerException e) {
				System.err.println("Skipped texture coordinates WriterOBJ.write(): Null value texture coordinate data");
			}
		}
		
		write( out, points, "v" );
		write( out, texture, "vt" );
		write( out, normals, "vn" );

		out.println();

		DataList indices = ifs.getFaceAttributes(Attribute.INDICES  );
		
		for (int i= 0; i < ifs.getNumFaces(); i++) {
			out.print( "f  ");
			IntArray faceIndices=indices.item(i).toIntArray();
			writeFaceIndex( out, startVertex + faceIndices.getValueAt(0), texture!=null, normals!=null );	
			for (int j= 1; j < faceIndices.size(); j++) {
				out.print( " " );
				writeFaceIndex( out, startVertex + faceIndices.getValueAt(j), texture!=null, normals!=null );
			}

			out.println();	
		}

		out.flush();
		return ifs.getNumPoints();
	}
	
	
}
