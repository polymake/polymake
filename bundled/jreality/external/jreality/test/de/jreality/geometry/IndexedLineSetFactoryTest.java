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

import junit.framework.TestCase;
import de.jreality.plugin.JRViewer;
import de.jreality.plugin.content.ContentAppearance;
import de.jreality.plugin.content.ContentLoader;
import de.jreality.plugin.content.ContentTools;
import de.jreality.scene.IndexedLineSet;
import de.jreality.scene.data.Attribute;

public class IndexedLineSetFactoryTest extends TestCase {

	IndexedLineSetFactory factory;
	
	static double [] vertices  = new double[] {

	 0,  0,  0,
	 1,  0,  0,
	 1,  1,  0,
	 0,  1,  0,

	 0,  0,  1,
	 1,  0,  1,
	 1,  1,  1,
	 0,  1,  1,

	};

	static int [][] indices = new int [][] {

	{ 0, 1, 2, 3 }, 
	{ 7, 6, 5, 4 }, 
	{ 0, 1, 5, 4 }, 
	{ 1, 2, 6, 5 }, 
	{ 2, 3, 7, 6 }, 
	{ 3, 0, 4, 7 }, 

	};

	public void setUp() {
		
	
		factory=new de.jreality.geometry.IndexedLineSetFactory();

	}
	
	public void tearDown() {
		
		
	}
	
	
	
	
	public void test() {

		factory.setVertexCount( 8 );
		factory.setVertexCoordinates( vertices );
		factory.setEdgeCount(6);
		factory.setEdgeIndices( indices );
		
		factory.update();
		
		factory.setEdgeIndices( indices );
		
		factory.update();
		
		factory.setVertexCoordinates( vertices );
		
		factory.update();
		
	}
	
	public void testEdgeLabels()	{
		
		//factory.debug = true;
		
		factory.setVertexCount( 8 );
		factory.setVertexCoordinates( vertices );
		factory.setEdgeCount(6);
		factory.setEdgeIndices( indices );
		
		factory.setGenerateEdgeLabels( true );
		factory.update();
		
		IndexedLineSet ils = factory.getIndexedLineSet();
		
		String [] labels = ils.getEdgeAttributes(Attribute.LABELS).toStringArray(null);
		
		for( int i=0; i<labels.length; i++ ) {
			assertEquals( labels[i], new Integer( i ).toString());
		}
		
		factory.setGenerateEdgeLabels( false );
		
		factory.update();
		
		assertEquals( ils.getEdgeAttributes(Attribute.LABELS), null );
		
		labels[0] = "gaga";
		
		factory.setEdgeLabels( labels );
		
		factory.update();
		
		labels = ils.getEdgeAttributes(Attribute.LABELS).toStringArray(null);
		
		assertEquals( labels[0],  "gaga" );
		for( int i=1; i<labels.length; i++ ) {
			assertEquals( labels[i], new Integer( i ).toString());
		}
		
		// this should work
		factory.setGenerateEdgeLabels( false );
		
		//this should fail
		try {
			factory.setGenerateEdgeLabels( true );
		} catch( UnsupportedOperationException e ) {
		}
		
		factory.setEdgeLabels( (String[])null );
		factory.setGenerateEdgeLabels( true );
	}
	
	
	public void testIndexedLineSet(){

		IndexedLineSetFactory ilsf = new IndexedLineSetFactory();
		int[][] ind1 = {{0,1}};
		double[][] pts = new double[10][3];
		ilsf.setVertexCount(pts.length);
		ilsf.setVertexCoordinates(pts);
		ilsf.setEdgeCount(ind1.length);
		ilsf.setEdgeIndices(ind1);
		ilsf.update();

		int[][] ind2 = new int[][]{{0,1,2}};
		ilsf.setEdgeIndices(ind2);
		ilsf.update();

	}

	public static void main( String [] arg ) {

		IndexedLineSetFactory factory = new IndexedLineSetFactory();


		factory.setVertexCount( 8 );
		factory.setEdgeCount( 6 );	
		factory.setVertexCoordinates( vertices );
		factory.setEdgeIndices( indices );
	    factory.setGenerateEdgeLabels(true);
	    factory.setGenerateVertexLabels(true);
		factory.setEdgeCount( 2 );
		factory.setEdgeIndices( new int[][] {{0,1},{2,3}} );
		factory.setEdgeColors( new Color[] {Color.RED, Color.YELLOW} );
		//factory.setEdgeLabels( new String[] {"A","B"} );
		factory.update();
	    JRViewer v = new JRViewer();
		v.addBasicUI();
		v.setContent(factory.getIndexedLineSet());
		v.registerPlugin(new ContentAppearance());
		v.registerPlugin(new ContentLoader());
		v.registerPlugin(new ContentTools());
		v.startup();
	}
}