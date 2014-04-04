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
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.Arrays;

import junit.framework.TestCase;
import de.jreality.math.MatrixBuilder;
import de.jreality.plugin.JRViewer;
import de.jreality.plugin.content.ContentAppearance;
import de.jreality.plugin.content.ContentLoader;
import de.jreality.plugin.content.ContentTools;
import de.jreality.scene.Appearance;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.IntArray;
import de.jreality.scene.pick.AABBTree;
import de.jreality.shader.ImageData;
import de.jreality.shader.Texture2D;
import de.jreality.shader.TextureUtility;
import de.jreality.tools.ActionTool;
import de.jreality.util.CameraUtility;
import de.jreality.util.Input;
import de.jreality.util.PickUtility;

public class IndexedFaceSetFactoryTest extends TestCase {

	IndexedFaceSetFactory factory;

	static double [] faceNormalse  = new double[] {
		0,  0,  1,
		-1,  0,  0,
		1,  0,  0,
		0,  1,  0,
		-1,  0,  0,
		0, -1,  0
	 };
	
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

	static double [] unwrapVertices  = new double[] {
		 0,  0,  0, //0
		 1,  0,  0, //1
		 1,  1,  0, //2
		 0,  1,  0, //3

		 0,  0,  1, //4
		 1,  0,  1, //5
		 1,  1,  1, //6
		 0,  1,  1, //7
		 
		 1,  1,  0, //8 -> 2
		 1,  1,  1, //9 -> 6
		 
		 0,  1,  0, //10 -> 3
		 0,  1,  1, //11 -> 7
		 
		 0,  1,  0, //12 -> 3
		 0,  1,  1, //13 -> 7	 
	};

	static int [][] unwrapIndices = new int [][] {
		// first two have different orientation from the other 4
		{ 0, 1, 2, 3 }, 
		{ 7, 6, 5, 4 }, 
		{ 0, 1, 5, 4 }, 
		{ 1, 8, 9, 5 }, //{ 1, 2, 6, 5 }, 
		{ 8, 10, 11, 9 }, //{ 2, 3, 7, 6 }, 
		{ 12, 0, 4, 13 }, //{ 3, 0, 4, 7 }, 
	};
	
	static double [][] unwrapTextureCoordinates = new double[][] {
		
		{ .25, .5},
		{ .5, .5},
		{ .5, .75},
		{ .25, .75},

		{ .25, .25 },
		{ .5, .25 },
		{ .5, .0 },
		{ .25, .0 },
		
		{ .75, .5 },
		{ .75, .25 },
		
		{ 1., .5 },
		{ 1., .25 },

		{ 0., .5 },
		{ 0., .25 },
	};
	
	static double [] vertices2  = new double[] {

		 0,  0,  0,
		 -1,  0,  0,
		 -1,  -1,  0,
		 0,  -1,  0,

		 0,  0,  -1,
		 -1,  0,  -1,
		 -1,  -1,  -1,
		 0,  -1,  -1,

		};

	static int [][] faceIndices = new int [][] {
		{ 0, 1, 2, 3 }, 
		{ 7, 6, 5, 4 }, 
		{ 0, 1, 5, 4 }, 
		{ 1, 2, 6, 5 }, 
		{ 2, 3, 7, 6 }, 
		{ 3, 0, 4, 7 }
	};

	static int [][] faceIndicesTriangles = new int [][] {

		{ 0, 1, 2 }, 
		{ 7, 6, 5 }, 
		{ 0, 1, 5 }, 
		{ 1, 2, 6 }, 
		{ 2, 3, 7 }, 
		{ 3, 0, 4 }
	};

	static int [][] edgeIndicesShort = new int [][] {
		{0, 1} , {1, 2} , {2, 3}, {3, 0}, 
		{7, 6} , {6, 5} , {5, 4}, {4, 7}, 
		{1, 5} , {4, 0}, 
		{2, 6} , {5, 1}, 
		{3, 7} , {6, 2}
	};
	
	static int [][] edgeIndicesLong = new int [][] {
		{ 0, 1, 2, 3 }, 
		{ 7, 6, 5, 4 }, 
		{ 0, 1, 5, 4 }, 
		{ 1, 2, 6, 5 }, 
		{ 2, 3, 7, 6 }, 
		{ 3, 0, 4, 7 }
	};


	static int [][] faceIndices2 = new int [][] {
		{ 7, 6, 5, 4 }, 
		{ 0, 1, 2, 3 }, 
		{ 1, 2, 6, 5 }, 
		{ 0, 1, 5, 4 }, 
		{ 3, 0, 4, 7 }, 
		{ 2, 3, 7, 6 }, 
	};

	public void setUp() {
		factory = new IndexedFaceSetFactory();

	}
	
	public void tearDown() {
		
		
	}
	

	public void testWeirdProblemWithEdgeColors()	{
		
		double y = .25, z = 9/24.0, a = .16666;
		double[][] verts = {
				{-1,1,1}, {-1,1,0}, {-1,y,1}, {-1,y,z}, {-a,-a,a},{0,0,0}, {y,-z,1}
		};
		int[][] faceI =  {{2,3,4,6}, {5,4,3,1}};  
		Color[] fc = {new Color(.7f, .7f, 0, .6f), new Color(.5f, 0, .8f,.3f)}; 
		int[][] edgeI = {{1,3},{3,2},{3,4},{4,6}, {4,5}};
		Color edge1 = new Color(1f, 0, 0), edge2 = new Color(0,0,1f);
		Color[] edgeC = {edge2, edge1, edge1, edge1, edge2};
		for (int j = 0; j<2; ++j)	{
			IndexedFaceSetFactory ifsf = new IndexedFaceSetFactory();
			//ifsf.debug = true;
			ifsf.setVertexCount(verts.length);
			ifsf.setVertexCoordinates(verts);
			ifsf.setFaceCount(faceI.length);
			ifsf.setFaceIndices(faceI);
			ifsf.setFaceColors(fc);
			ifsf.setEdgeCount(edgeI.length);
			ifsf.setEdgeIndices(edgeI);
			if (j == 1) ifsf.setEdgeColors(edgeC);
			ifsf.setGenerateEdgesFromFaces(false);
			ifsf.setGenerateFaceNormals(true);
			ifsf.update();
			IndexedFaceSet ifs = ifsf.getIndexedFaceSet();
			int n = ifs.getNumEdges();
			System.err.println("IFS edgecount: "+n);
			for (int i = 0; i<n; ++i)	{
				IntArray ia = ifs.getEdgeAttributes(Attribute.INDICES).item(i).toIntArray();
				System.err.print("Edge "+i+":\t");
				for (int k = 0; k<ia.getLength(); ++k)	
					System.err.print(ia.item(k)+"\t");
				System.err.println("");
			}			
			System.err.println("Created ifs "+(j==0 ? "without" : "with")+" edge colors");
		}
	}

	
	public void testBugInitialGetVertexCount() {
		assertEquals(factory.getFaceCount(), 0);
	}
	
	public void testFaceLabels()	{
		
		//factory.debug = true;
		
		factory.setVertexCount( vertices.length );
		assertTrue(factory.getVertexCount()== vertices.length);
		factory.setVertexCoordinates( vertices );	
		
		factory.setFaceCount( faceIndices.length );
		factory.setFaceIndices( faceIndices );
		factory.setGenerateFaceLabels( true );
		factory.update();
		
		IndexedFaceSet ifs = factory.getIndexedFaceSet();
		
		String [] labels = ifs.getFaceAttributes(Attribute.LABELS).toStringArray(null);
		
		for( int i=0; i<labels.length; i++ ) {
			assertEquals( labels[i], new Integer( i ).toString());
		}
		
		factory.setGenerateFaceLabels( false );
		
		factory.update();
		
		assertEquals( ifs.getFaceAttributes(Attribute.LABELS), null );
		
		labels[0] = "gaga";
		
		factory.setFaceLabels( labels );
		
		factory.update();
		
		labels = ifs.getFaceAttributes(Attribute.LABELS).toStringArray(null);
		
		assertEquals( labels[0],  "gaga" );
		for( int i=1; i<labels.length; i++ ) {
			assertEquals( labels[i], new Integer( i ).toString());
		}
		
		// this should work
		factory.setGenerateFaceLabels( false );
		
		//this should fail
		try {
			factory.setGenerateFaceLabels( true );
		} catch( UnsupportedOperationException e ) {
		}
		
		factory.setFaceLabels( (String[])null );
		factory.setGenerateFaceLabels( true );
	}
	
	public void testFaceColors()	{
		double[][] jitterbugEdgeVerts = new double[][] {{0,0,0,1},{1,0,0,1},{1,1,0,1},{0,1,0,1}};
		int[][] jitterbugSegmentIndices1 = {{0,1},{2,3}}; //,{{{0,1,2,3,0,1},{4,5,6,7,4,5},{8,9,10,11,8,9}};
		int[][] jitterbugFaceIndices = {{0,1,2,3}};
		factory = new IndexedFaceSetFactory();
		//factory.debug = true;
		factory.setVertexCount(jitterbugEdgeVerts.length);
		factory.setVertexCoordinates(jitterbugEdgeVerts);	
		factory.setFaceCount(1);
		factory.setFaceIndices(jitterbugFaceIndices);	
		factory.setFaceColors(new double[][]{{0,1,0}});
		factory.setGenerateFaceNormals(true);
		factory.setEdgeCount(jitterbugSegmentIndices1.length);
		factory.setEdgeIndices(jitterbugSegmentIndices1);
		factory.update();
		
		IndexedFaceSet ifs = factory.getIndexedFaceSet();
			
		assertEquals( ifs.getFaceAttributes(Attribute.COLORS).toDoubleArrayArray(null)[0][0], 0, 0);
		assertEquals( ifs.getFaceAttributes(Attribute.COLORS).toDoubleArrayArray(null)[0][1], 1, 0);
		assertEquals( ifs.getFaceAttributes(Attribute.COLORS).toDoubleArrayArray(null)[0][2], 0, 0);
		//System.err.println("Alpha channel is "+borromeanRectFactory.getIndexedFaceSet().getFaceAttributes(Attribute.COLORS).toDoubleArrayArray(null)[0][3]);
		
		// now we try to change the alpha channel of the face color
		// just to be safe, we don't use the old array but create a new one.
		factory.setFaceColors(new double[][]{{1,0,0}});
		
		factory.update();
		
		assertEquals( 1, ifs.getFaceAttributes(Attribute.COLORS).toDoubleArrayArray(null)[0][0], 0);
		assertEquals( 0, ifs.getFaceAttributes(Attribute.COLORS).toDoubleArrayArray(null)[0][1], 0);
		assertEquals( 0, ifs.getFaceAttributes(Attribute.COLORS).toDoubleArrayArray(null)[0][2], 0);
		
		//System.err.println("Alpha channel is "+borromeanRectFactory.getIndexedFaceSet().getFaceAttributes(Attribute.COLORS).toDoubleArrayArray(null)[0][3]);
		
		//ViewerApp.display(factory.getIndexedFaceSet());
	}
	
	public void testGenerateEdgesFromFaces() {
		//factory.debug = true;
		factory.setVertexCount( vertices.length);
		factory.setVertexCoordinates( vertices );
		factory.setFaceCount( faceIndices.length );
		factory.setFaceIndices( faceIndices );
		
		factory.setGenerateEdgeLabels(true);
		factory.setGenerateEdgesFromFaces( true );
		factory.update();
		
		IndexedFaceSet ifs = factory.getIndexedFaceSet();
		
		int[][] edges = ifs.getEdgeAttributes(Attribute.INDICES).toIntArrayArray(null);
		
		assertEquals( 12, ifs.getNumEdges() );
		
		try {
		factory.setEdgeCount( edges.length );
		} catch( UnsupportedOperationException e ) {	
		}
		assertEquals( 12, ifs.getNumEdges() );
		factory.update();
		
		assertEquals( 12, ifs.getNumEdges() );
		
		try {
			factory.setEdgeIndices( edges );
		} catch( UnsupportedOperationException e ) {		
		}
		
		factory.update();
		
		assertEquals( 12, ifs.getNumEdges() );
		
		factory.setGenerateEdgesFromFaces( false );
		
		factory.update();
		
		assertEquals( 0, ifs.getNumEdges() );
		
		factory.setGenerateEdgesFromFaces( true );		
		factory.setGenerateEdgesFromFaces( false );
		
		factory.update();
		
		assertEquals( 0, ifs.getNumEdges() );
		
		factory.setGenerateEdgesFromFaces( true );		
		
		factory.update();
		
		assertEquals( 12, ifs.getNumEdges() );
		
		String [] edgeLabels = ifs.getEdgeAttributes(Attribute.LABELS).toStringArray(null);
		assertEquals( 12, edgeLabels.length );
		for( int i=0; i<edgeLabels.length; i++ )
				assertEquals( i+"", edgeLabels[i] );	
		
		factory.setGenerateEdgesFromFaces( false );
		
		factory.update();
		
		assertEquals( 0, ifs.getNumEdges() );
		
		factory.setEdgeCount( edges.length );
		factory.setEdgeIndices( edges );
	}
	
	public void testGenerateAABBTrees() {
		//factory.debug = true;
		factory.setVertexCount( vertices.length/3);
		factory.setVertexCoordinates( vertices );
		factory.setFaceCount( faceIndices.length );
		factory.setFaceIndices( faceIndices );
		
		factory.setGenerateEdgeLabels(true);
		factory.setGenerateEdgesFromFaces( true );
		factory.update();
		
		IndexedFaceSet ifs = factory.getIndexedFaceSet();
		
		AABBTree aabb = (AABBTree) ifs.getGeometryAttributes(PickUtility.AABB_TREE); 
		assertNull(aabb);

		factory.setGenerateAABBTree(true);
		
		factory.update();
		
		aabb = (AABBTree) ifs.getGeometryAttributes(PickUtility.AABB_TREE); 
		assertNotNull(aabb);
		
		factory.setVertexCoordinates(vertices2);
		factory.update();

		AABBTree aabb2 = (AABBTree) ifs.getGeometryAttributes(PickUtility.AABB_TREE); 
		assertFalse(aabb == aabb2);
		
		factory.setGenerateEdgeLabels(false);
		factory.update();
		
		aabb = (AABBTree) ifs.getGeometryAttributes(PickUtility.AABB_TREE); 
		assertTrue(aabb == aabb2);
		
		factory.setFaceIndices(faceIndices2);
		factory.update();

		aabb = (AABBTree) ifs.getGeometryAttributes(PickUtility.AABB_TREE); 
		assertFalse(aabb == aabb2);

		factory.setGenerateAABBTree(false);
		factory.update();

		aabb = (AABBTree) ifs.getGeometryAttributes(PickUtility.AABB_TREE); 
		assertNull(aabb);
		

	}	
	
	 public void testStrangeError() {
         IndexedFaceSet ifs = Primitives.cube();
         IndexedFaceSetFactory ifsf=new IndexedFaceSetFactory();
         ifsf.setGenerateEdgesFromFaces(false);
         ifsf.setGenerateFaceNormals(true);
         ifsf.setGenerateVertexNormals(false);
         System.out.println(ifs.getEdgeAttributes());
         //       uebernehmen der Face Attribute:
         ifsf.setFaceCount(6);
         ifsf.setVertexCount(8);
//       ifsf.setLineCount(12);
//       ifsf.setVertexAttributes(ifs.getVertexAttributes());
         ifsf.setGenerateFaceNormals(false);
         ifsf.setFaceAttributes(ifs.getFaceAttributes());
//       ifsf.setEdgeAttributes(ifs.getEdgeAttributes());

         ifsf.setFaceIndices(ifs.getFaceAttributes(Attribute.INDICES).toIntArrayArray(null));

         ifsf.setVertexCoordinates(ifs.getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray(null));
         ifsf.setFaceNormals( (double[])null );
         ifsf.setGenerateFaceNormals(true);
         ifsf.update();
 }
	 
	 public void testUnwrapFaceIndices() {
		 /* initialize without unwrap face indices (but already to much vertices) */
		 factory.setVertexCount( unwrapVertices.length/3);
		 factory.setVertexCoordinates( unwrapVertices );
		 factory.setFaceCount( faceIndices.length );
		 factory.setFaceIndices( faceIndices );
		 factory.setVertexTextureCoordinates( unwrapTextureCoordinates );
		 factory.setGenerateAABBTree( true );
		 factory.setGenerateEdgeLabels(true);
		 factory.setGenerateEdgesFromFaces( true );
		 factory.setGenerateFaceLabels(true);
		 factory.setGenerateFaceNormals(true);
		 factory.setGenerateVertexLabels(true);
		 factory.setGenerateVertexNormals( true );
		 factory.update();

		 /* check whether attributes(vertex coordinates, face indices, texture coordinates) 
		  * where set as planed in the indexed face set
		  * and save the generated attributes for comparison 
		  */
		 IndexedFaceSet ifs=factory.getIndexedFaceSet();
		 assertTrue(Arrays.equals(ifs.getVertexAttributes(Attribute.COORDINATES).toDoubleArray(null),unwrapVertices));
		 assertTrue(Arrays.deepEquals(ifs.getFaceAttributes(Attribute.INDICES).toIntArrayArray(null), faceIndices));
		 assertTrue(Arrays.deepEquals(ifs.getVertexAttributes(Attribute.TEXTURE_COORDINATES).toDoubleArrayArray(null), unwrapTextureCoordinates));
		 AABBTree aabbTree=(AABBTree) ifs.getGeometryAttributes(PickUtility.AABB_TREE);
		 String[] edgeLabels = ifs.getEdgeAttributes(Attribute.LABELS).toStringArray(null);
		 int[] edgeIndices = ifs.getEdgeAttributes(Attribute.INDICES).toIntArray(null);
		 String[] faceLabels = ifs.getFaceAttributes(Attribute.LABELS).toStringArray(null);
		 double[] faceNormals = ifs.getFaceAttributes(Attribute.NORMALS).toDoubleArray(null);
		 String[] vertexLabels = ifs.getVertexAttributes(Attribute.LABELS).toStringArray(null);
		 double[][] vertexNormals = ifs.getVertexAttributes(Attribute.NORMALS).toDoubleArrayArray(null);

		 /* introduce unwrap face indices, check whether 
		  * attributes(vertex coordinates, face indices, texture coordinates) 
		  * where set as planed in the indexed face set
		  * and check whether generated attributes are correct again
		  */
		 factory.setUnwrapFaceIndices(unwrapIndices);
		 factory.update();
		 assertTrue(Arrays.equals(ifs.getVertexAttributes(Attribute.COORDINATES).toDoubleArray(null),unwrapVertices));
		 assertTrue(Arrays.deepEquals(ifs.getFaceAttributes(Attribute.INDICES).toIntArrayArray(null), unwrapIndices));
		 assertTrue(Arrays.deepEquals(ifs.getVertexAttributes(Attribute.TEXTURE_COORDINATES).toDoubleArrayArray(null), unwrapTextureCoordinates));
		 assertEquals(ifs.getGeometryAttributes(PickUtility.AABB_TREE), aabbTree);
		 assertTrue(Arrays.deepEquals(ifs.getEdgeAttributes(Attribute.LABELS).toStringArray(null),edgeLabels));
		 assertTrue(Arrays.equals(ifs.getEdgeAttributes(Attribute.INDICES).toIntArray(null),edgeIndices));
		 assertTrue(Arrays.equals(ifs.getFaceAttributes(Attribute.LABELS).toStringArray(null),faceLabels));
		 assertTrue(Arrays.equals(ifs.getFaceAttributes(Attribute.NORMALS).toDoubleArray(null),faceNormals));
		 assertTrue(Arrays.equals(ifs.getVertexAttributes(Attribute.LABELS).toStringArray(null), vertexLabels));
		 double[][] unwrapNormals = ifs.getVertexAttributes(Attribute.NORMALS).toDoubleArrayArray(null);
		 assertFalse(Arrays.deepEquals(unwrapNormals, vertexNormals));
		 int[] trans=new int[] {0,1,2,3,4,5,6,7,2,6,3,7,3,7};
		 for (int i=0; i<14; i++ )
			 assertTrue(Arrays.equals(unwrapNormals[i], vertexNormals[trans[i]]));

		 /* check the unwrap vertex attributes methods */
		 assertTrue(Arrays.equals(factory.unwrapVertexAttributes(vertices,3),unwrapVertices));
		 assertTrue(Arrays.equals(IndexedFaceSetFactory.unwrapVertexAttributes(vertices,3,faceIndices,unwrapIndices,14),unwrapVertices));
		 assertTrue(Arrays.equals(factory.unwrapVertexAttributes(
				 new String[] {"0","1","2","3","4","5","6","7"}),
				 new String[] {"0","1","2","3","4","5","6","7","2","6","3","7","3","7"}));
		 assertTrue(Arrays.equals(IndexedFaceSetFactory.unwrapVertexAttributes(
				 new String[] {"0","1","2","3","4","5","6","7"},faceIndices, unwrapIndices,14),
				 new String[] {"0","1","2","3","4","5","6","7","2","6","3","7","3","7"}));
		 
		 /* check whether a reset of unwrap face indices works */
		 factory.setUnwrapFaceIndices((int[])null);
		 factory.update();
		 assertTrue(Arrays.equals(ifs.getVertexAttributes(Attribute.COORDINATES).toDoubleArray(null),unwrapVertices));
		 assertTrue(Arrays.deepEquals(ifs.getFaceAttributes(Attribute.INDICES).toIntArrayArray(null), faceIndices));
		 assertTrue(Arrays.deepEquals(ifs.getVertexAttributes(Attribute.TEXTURE_COORDINATES).toDoubleArrayArray(null), unwrapTextureCoordinates));
		 assertEquals(ifs.getGeometryAttributes(PickUtility.AABB_TREE), aabbTree);
		 assertTrue(Arrays.deepEquals(ifs.getEdgeAttributes(Attribute.LABELS).toStringArray(null),edgeLabels));
		 assertTrue(Arrays.equals(ifs.getEdgeAttributes(Attribute.INDICES).toIntArray(null),edgeIndices));
		 assertTrue(Arrays.equals(ifs.getFaceAttributes(Attribute.LABELS).toStringArray(null),faceLabels));
		 assertTrue(Arrays.equals(ifs.getFaceAttributes(Attribute.NORMALS).toDoubleArray(null),faceNormals));
		 assertTrue(Arrays.equals(ifs.getVertexAttributes(Attribute.LABELS).toStringArray(null), vertexLabels));
		 double[][] normals = ifs.getVertexAttributes(Attribute.NORMALS).toDoubleArrayArray(null);
		 assertTrue(Arrays.deepEquals(normals, vertexNormals));
	 }
	 
	 public void testSetFaceIndicesDifferentSizes() {
			factory.setVertexCount(vertices.length/3);
			factory.setVertexCoordinates(vertices);
			IndexedFaceSet ifs = factory.getIndexedFaceSet();
			
			factory.setFaceCount(faceIndicesTriangles.length);
			factory.setFaceIndices(faceIndicesTriangles);
			factory.update();
			assertTrue(Arrays.deepEquals(ifs.getFaceAttributes(Attribute.INDICES).toIntArrayArray(null), 
					faceIndicesTriangles));
			
			factory.setFaceCount(faceIndices.length);
			factory.setFaceIndices(faceIndices);
			factory.update();
			assertTrue(Arrays.deepEquals(ifs.getFaceAttributes(Attribute.INDICES).toIntArrayArray(null), 
					faceIndices));
			
			factory.setFaceCount(faceIndicesTriangles.length);
			factory.setFaceIndices(faceIndicesTriangles);
			factory.update();
			assertTrue(Arrays.deepEquals(ifs.getFaceAttributes(Attribute.INDICES).toIntArrayArray(null), 
					faceIndicesTriangles));
	 }

	 public void testSetEdgeIndicesDifferentSizes() {
			factory.setVertexCount( vertices.length/3);
			factory.setVertexCoordinates( vertices );
			IndexedFaceSet ifs = factory.getIndexedFaceSet();
	
			factory.setEdgeCount(edgeIndicesShort.length);
			factory.setEdgeIndices(edgeIndicesShort);
			factory.update();
			assertTrue(Arrays.deepEquals(ifs.getEdgeAttributes(Attribute.INDICES).toIntArrayArray(null), 
					edgeIndicesShort));
		 
			
			factory.setEdgeCount(edgeIndicesLong.length);
			factory.setEdgeIndices(edgeIndicesLong);
			factory.update();
			assertTrue(Arrays.deepEquals(ifs.getEdgeAttributes(Attribute.INDICES).toIntArrayArray(null), 
					edgeIndicesLong));

			
			factory.setEdgeCount(edgeIndicesShort.length);
			factory.setEdgeIndices(edgeIndicesShort);
			factory.update();
			assertTrue(Arrays.deepEquals(ifs.getEdgeAttributes(Attribute.INDICES).toIntArrayArray(null), 
					edgeIndicesShort));
}

	public static void main( String [] arg ) {

		IndexedFaceSetFactory factory2 = new IndexedFaceSetFactory();
		factory2.setVertexCount( 8 );
		factory2.setFaceCount( 6 );	
		factory2.setVertexCoordinates( vertices );
		factory2.setFaceIndices( faceIndices );
		factory2.setGenerateFaceNormals( true );
		factory2.setGenerateVertexNormals( false );
		factory2.setGenerateEdgesFromFaces( true );
		factory2.setGenerateVertexLabels(true);
		factory2.setVertexRelativeRadii( new double [] { 1, 2, 3, 4, 1, 2, 3, 4 } );
		factory2.setFaceColors( new Color[] {Color.RED, Color.GREEN, Color.RED, Color.GREEN, Color.RED, Color.GREEN })  ;
		factory2.update();
	    JRViewer v = new JRViewer();
		v.addBasicUI();
		v.setContent(factory2.getIndexedFaceSet());
		v.registerPlugin(new ContentAppearance());
		v.registerPlugin(new ContentLoader());
		v.registerPlugin(new ContentTools());
		v.startup();

		
		IndexedFaceSetFactory factory3 = new IndexedFaceSetFactory();
		factory3.setVertexCount( 8 );
		factory3.setFaceCount( 6 );	
		factory3.setVertexCoordinates( vertices );
		factory3.setFaceIndices( faceIndices );
		factory3.setGenerateFaceNormals( true );
		factory3.setGenerateVertexNormals( false );
		factory3.setGenerateEdgesFromFaces( true );
		factory3.setGenerateVertexLabels(true);
		//factory3.setEdgeIndices( new int[][] {{0,1}} );
		factory3.setVertexRelativeRadii( new double [] { 1, 2, 3, 4, 1, 2, 3, 4 } );		
		factory3.setFaceColors( new Color[] {Color.RED, Color.YELLOW, Color.RED, Color.YELLOW, Color.RED, Color.YELLOW })  ;
		factory3.update();
	    JRViewer v1 = new JRViewer();
		v1.addBasicUI();
		v1.setContent(factory3.getIndexedFaceSet());
		v1.registerPlugin(new ContentAppearance());
		v1.registerPlugin(new ContentLoader());
		v1.registerPlugin(new ContentTools());
		v1.startup();
		
		final IndexedFaceSetFactory ifsf = new IndexedFaceSetFactory();
		
		ifsf.setVertexCount( 14 );
		ifsf.setVertexCoordinates( unwrapVertices );
		ifsf.setVertexAttribute(Attribute.TEXTURE_COORDINATES, unwrapTextureCoordinates);
		
		ifsf.setFaceCount( 6 );	
		ifsf.setFaceIndices( faceIndices );

		ifsf.setGenerateVertexNormals( true );
		ifsf.setGenerateEdgesFromFaces( true );
		/* The texture labels indicate that edges are doubled only in mode 2 below*/
		ifsf.setGenerateEdgeLabels(true);
		/* Crude documentation */ 
		ifsf.setVertexLabels(new String[]{"","","","","","    Klick with middle mouse","","","","","","","",""});
		ifsf.update();
		
		SceneGraphComponent sgc = new SceneGraphComponent("scene");
		
		/* An action tool that cycles through 3 modes when the cube 
		 * is clicked with the middle mouse button 
		 * mode0: the texture cube with texture jumps, because no unwrapped indices are set
		 * mode1: the nicely textured unwrapped cube
		 * mode3: also nicely textured unwrapped cube, but now edges are doubled 
		 * and broken vertex normals
		 */
		ActionTool tool = new ActionTool("PrimaryMenu");
		tool.addActionListener(new ActionListener(){
			int mode=0;
			public void actionPerformed(ActionEvent e) {
				mode = (mode +1) % 3;
				if (mode==0) {
					ifsf.setFaceIndices( faceIndices ); 
					ifsf.setUnwrapFaceIndices((int[][]) null); 
					ifsf.setVertexLabels(new String[]{"","","","","","    NO unwrapped face indices","","","","","","","",""});
				}
				if (mode==1) {
					ifsf.setFaceIndices( faceIndices ); 
					ifsf.setUnwrapFaceIndices( unwrapIndices ); 
					ifsf.setVertexLabels(new String[]{"","","","","","    wrapped and UNWRAPPED face indices","","","","","","","",""});
					}
				if (mode==2) {
					ifsf.setFaceIndices( unwrapIndices ); 
					ifsf.setUnwrapFaceIndices( (int[][]) null ); 
					ifsf.setVertexLabels(new String[]{"","","","","","    DOUBLED edges and BROKEN vertex normals, unwrapped indices","","","","","","","",""});
					}
				ifsf.update();
			}
		});
		sgc.addTool(tool);
		sgc.setGeometry(ifsf.getIndexedFaceSet());
		
		/* Add a texture */
		sgc.setAppearance(new Appearance());
		Texture2D tex;
		try{
			tex=TextureUtility.createTexture(
				sgc.getAppearance(),       
				"polygonShader", 
				ImageData.load(Input.getInput("de/jreality/geometry/black_cross.png")),
				false);
			tex.setTextureMatrix(MatrixBuilder.euclidean().scale(12).getMatrix());
		} catch (Exception e) {
			e.printStackTrace();
		}
		
		/* scale the cube, to get the lables smaller */
		MatrixBuilder.euclidean().scale(4).assignTo(sgc);

	    JRViewer v2 = new JRViewer();
		v2.addBasicUI();
		v2.setContent(sgc);
		v2.registerPlugin(new ContentAppearance());
		v2.registerPlugin(new ContentLoader());
		v2.registerPlugin(new ContentTools());
		v2.startup();
		CameraUtility.encompass(v2.getViewer());
	};
	
	public void testFaceIndicesUnset() throws Exception {
		factory.setVertexCount(vertices.length/3);
		factory.setVertexCoordinates(vertices);
		factory.setFaceCount(0);
		factory.update();
	}
	
}
