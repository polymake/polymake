package de.jreality.reader;

import static de.jreality.reader.TestUtils.parseString;
import static de.jreality.reader.TestUtils.testDoubleArrayArray;
import static de.jreality.reader.TestUtils.testEdgeIndices;
import static de.jreality.reader.TestUtils.testIntArrayArray;
import static de.jreality.reader.TestUtils.testVertexCoordinates;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.net.URL;
import java.util.logging.LogManager;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.data.Attribute;

public class ReaderOBJTest {

	public static double delta = 1E-10;
	
	@Before
	public void readCustomLoggingProperties() {
		try {
			LogManager.getLogManager().readConfiguration(new FileInputStream( new File("test_logging.properties")));
		} catch (IOException e) {
			System.out.println("File test_logging.properties not found.");
			// You can customize log levels in test_logging.properties file.
		} 
	}
	
	@Test
	public void testPointSetRead() throws IOException{
		URL url = this.getClass().getResource("points.obj");
        ReaderOBJ reader = new ReaderOBJ();
        reader.read(url);
        SceneGraphComponent sgc = reader.getComponent();
        IndexedFaceSet ifs = (IndexedFaceSet) sgc.getChildComponent(0).getGeometry();
        double[] vertices = { 0,0,0,0,1,0,1,0,0,1,1,0 };
        Assert.assertArrayEquals(vertices, ifs.getVertexAttributes(Attribute.COORDINATES).toDoubleArray(null), delta);
	}
	
	@Test
	public void testSignedPointSetRead() throws IOException{
		URL url = this.getClass().getResource("signs.obj");
        ReaderOBJ reader = new ReaderOBJ();
        reader.read(url);
        SceneGraphComponent sgc = reader.getComponent();
        IndexedFaceSet ifs = (IndexedFaceSet) sgc.getChildComponent(0).getGeometry();
        double[] vertices = { 1, -1, 1E-10 };
        Assert.assertArrayEquals(vertices, ifs.getVertexAttributes(Attribute.COORDINATES).toDoubleArray(null), delta);
	}
	
	@Test
	public void testSignedSmallEPointSetRead() throws IOException{
		URL url = this.getClass().getResource("signs_e.obj");
        ReaderOBJ reader = new ReaderOBJ();
        reader.read(url);
        SceneGraphComponent sgc = reader.getComponent();
        IndexedFaceSet ifs = (IndexedFaceSet) sgc.getChildComponent(0).getGeometry();
        double[] vertices = { 1, -1, 0.1 };
        Assert.assertArrayEquals(vertices, ifs.getVertexAttributes(Attribute.COORDINATES).toDoubleArray(null), delta);
	}
	
	@Test
	public void testQuad2DRead() throws IOException{
		URL url = this.getClass().getResource("quad_2d.obj");
        ReaderOBJ reader = new ReaderOBJ();
        reader.read(url);
        SceneGraphComponent sgc = reader.getComponent();
        IndexedFaceSet ifs = (IndexedFaceSet) sgc.getChildComponent(0).getGeometry();
        
        double[][] vertices = { {0,0,0}, {0,1,0}, {1,0,0}, {1,1,0} };
		testVertexCoordinates(vertices, ifs, delta);
        
        int[] faces = {0,1,2,3};
        Assert.assertArrayEquals(faces, ifs.getFaceAttributes(Attribute.INDICES).toIntArray(null));
        
        int[][] edges = {{0,1},{1,2},{2,3},{0,3}};
        testEdgeIndices(edges, ifs);
	}

	@Test
	public void testUseAllRead() throws IOException{
		URL url = this.getClass().getResource("tri_4verts.obj");
        ReaderOBJ reader = new ReaderOBJ();
        reader.setGenerateEdgesFromFaces(true);
        reader.read(url);
        SceneGraphComponent sgc = reader.getComponent();
        IndexedFaceSet ifs = (IndexedFaceSet) sgc.getChildComponent(0).getGeometry();
        double[][] vertices = { {0,0,0} , {0,1,0} , {1,0,0}};
        testVertexCoordinates(vertices, ifs, delta);
        
        int[] faces = {0,1,2};
        Assert.assertArrayEquals(faces, ifs.getFaceAttributes(Attribute.INDICES).toIntArray(null));
        
        int[][] edges = {{0,1},{1,2},{0,2}};
        testEdgeIndices(edges, ifs);
	}
	
	@Test
	public void testGenerateEdgesTrueRead() throws IOException{
		URL url = this.getClass().getResource("tri_4verts.obj");
        ReaderOBJ reader = new ReaderOBJ();
        
        reader.setGenerateEdgesFromFaces(true);
        
        reader.read(url);
        SceneGraphComponent sgc = reader.getComponent();
        IndexedFaceSet ifs = (IndexedFaceSet) sgc.getChildComponent(0).getGeometry();
        double[][] vertices = { {0,0,0},{0,1,0},{1,0,0} };
        testVertexCoordinates(vertices, ifs, delta);
        
        int[][] faces = {{0,1,2}};
        Assert.assertArrayEquals(faces, ifs.getFaceAttributes(Attribute.INDICES).toIntArrayArray(null));
        
        int[][] edges = {{0,1},{1,2},{0,2}};
        testEdgeIndices(edges, ifs);
	}
	
	@Test
	public void testGenerateEdgesFalseRead() throws IOException{
		URL url = this.getClass().getResource("tri_4verts.obj");
        ReaderOBJ reader = new ReaderOBJ();
        
        reader.setGenerateEdgesFromFaces(false);
        
        reader.read(url);
        SceneGraphComponent sgc = reader.getComponent();
        IndexedFaceSet ifs = (IndexedFaceSet) sgc.getChildComponent(0).getGeometry();
        double[] vertices = { 0,0,0,0,1,0,1,0,0 };
        
        Assert.assertArrayEquals(vertices, ifs.getVertexAttributes(Attribute.COORDINATES).toDoubleArray(null), delta);
        
        int[] faces = {0,1,2};
        Assert.assertArrayEquals(faces, ifs.getFaceAttributes(Attribute.INDICES).toIntArray(null));
        
        Assert.assertNull(ifs.getEdgeAttributes(Attribute.INDICES));
	}
	
	@Test
	public void testLinesAndFacesGenerateEdgesFalseRead() throws IOException{
		URL url = this.getClass().getResource("trilines.obj");
        ReaderOBJ reader = new ReaderOBJ();
        
        reader.setGenerateEdgesFromFaces(false);
        
        reader.read(url);
        SceneGraphComponent sgc = reader.getComponent();
        IndexedFaceSet ifs = (IndexedFaceSet) sgc.getChildComponent(0).getGeometry();
        double[] vertices = { 0,0,0,0,1,0,1,0,0,1,1,0 };
        
        Assert.assertArrayEquals(vertices, ifs.getVertexAttributes(Attribute.COORDINATES).toDoubleArray(null), delta);
        
        int[] faces = {0,1,2};
        Assert.assertArrayEquals(faces, ifs.getFaceAttributes(Attribute.INDICES).toIntArray(null));
        
        int[][] edges = {{0,1,2,3}};
        testEdgeIndices(edges, ifs);
	}
	
	@Test
	public void testUseMultipleTexAndNormalsTrueRead() throws IOException{
		URL url = this.getClass().getResource("multiTex.obj");
        ReaderOBJ reader = new ReaderOBJ();
        
        reader.setUseMultipleTexAndNormalCoords(true);
        
        reader.read(url);
        SceneGraphComponent sgc = reader.getComponent();
        IndexedFaceSet ifs = (IndexedFaceSet) sgc.getChildComponent(0).getGeometry();
        double[][] vertices = { {0,0,0},{0,1,0},{1,0,0},{0,1,0},{1,0,0},{1,1,0} };
        testVertexCoordinates(vertices, ifs, delta);
        
        int[][] faces = {{0,1,2},{3,4,5}};
        int[][] facesIFS = ifs.getFaceAttributes(Attribute.INDICES).toIntArrayArray(null);
        testIntArrayArray(faces, facesIFS);
        
        int[][] edges = {{0,1},{1,2},{0,2},{3,4},{4,5},{3,5}};
        testEdgeIndices(edges, ifs);
        
        double[][] texCoords = {{0,0,0},{0,1,0},{1,0,0},{0,0,0},{0,1,0},{1,0,0}};
        double[][] texIFS = ifs.getVertexAttributes(Attribute.TEXTURE_COORDINATES).toDoubleArrayArray(null);
        testDoubleArrayArray(texCoords,texIFS, delta);
	}

	@Test
	public void testUseMultipleTexAndNormalsFalseRead() throws IOException{
		URL url = this.getClass().getResource("multiTex.obj");
        ReaderOBJ reader = new ReaderOBJ();
        
        reader.setUseMultipleTexAndNormalCoords(false);
        
        reader.read(url);
        SceneGraphComponent sgc = reader.getComponent();
        IndexedFaceSet ifs = (IndexedFaceSet) sgc.getChildComponent(0).getGeometry();
        double[][] vertices = { {0,0,0},{0,1,0},{1,0,0},{1,1,0}};
        testVertexCoordinates(vertices, ifs, delta);
        
        int[][] faces = {{0,1,2},{1,2,3}};
        int[][] facesIFS = ifs.getFaceAttributes(Attribute.INDICES).toIntArrayArray(null);
        testIntArrayArray(faces, facesIFS);
        
        int[][] edges = {{0,1},{1,2},{0,2},{2,3},{1,3}};
        testEdgeIndices(edges, ifs);
        
        double[][] texture = {{0,0,0},{0,1,0},{1,0,0},{1,0,0}};
        double[][] textureIFS = ifs.getVertexAttributes(Attribute.TEXTURE_COORDINATES).toDoubleArrayArray(null);
        testDoubleArrayArray(texture, textureIFS, delta);
	}
	
	@Test public void testNegativeIndices() throws Exception {
    	String testGroup = "testGroup";
		String objData = 
    		"g "+ testGroup +"\n"+
    		"v 0 0 0\n" + 
    		"v 0 0 1\n" + 
    		"v 1 1 0\n" +
    		"vt 0 0\n" +
    		"vt 0 1\n" +
    		"vt 1 1\n" +
    		"f -1/1 -2/2 -3/3";
    	SceneGraphComponent root = parseString(objData);
    	SceneGraphComponent child = root.getChildComponent(0);
    	Assert.assertTrue(testGroup.equals(child.getName()));
		IndexedFaceSet g = (IndexedFaceSet)child.getGeometry();
    	
		double[][] vertices = {{1,1,0},{0,0,1},{0,0,0}};
    	TestUtils.testVertexCoordinates(vertices, g, delta);
    	double[][] texture = {{0,0},{0,1},{1,1}};
    	TestUtils.testTextureCoordinates(texture,g,delta);
    }

	@Test public void testNegativeIndicesTexture() throws Exception {
    	String testGroup = "testGroup";
		String objData = 
    		"g "+ testGroup +"\n"+
    		"v 0 0 0\n" + 
    		"v 0 0 1\n" + 
    		"v 1 1 0\n" +
    		"vt 0 0\n" +
    		"vt 0 1\n" +
    		"vt 1 1\n" +
    		"f -1/-3 -2/-2 -3/-1";
    	SceneGraphComponent root = parseString(objData);
    	SceneGraphComponent child = root.getChildComponent(0);
    	Assert.assertTrue(testGroup.equals(child.getName()));
		IndexedFaceSet g = (IndexedFaceSet)child.getGeometry();
    	
		double[][] vertices = {{1,1,0},{0,0,1},{0,0,0}};
    	TestUtils.testVertexCoordinates(vertices, g, delta);
    	double[][] texture = {{0,0},{0,1},{1,1}};
    	TestUtils.testTextureCoordinates(texture,g,delta);
    }

	@Test public void testIgnoreTag() throws Exception {
		String objData = 
    		"k 0 0 0\n" + 
    		"k 0 0 1";
    	SceneGraphComponent root = parseString(objData);
    	Assert.assertEquals(root.getChildComponentCount(), 0);
    }

}
