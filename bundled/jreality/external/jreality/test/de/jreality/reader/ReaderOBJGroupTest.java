package de.jreality.reader;

import static de.jreality.reader.TestUtils.parseString;

import org.junit.Assert;
import org.junit.Test;

import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.DataList;

public class ReaderOBJGroupTest {

	final static double delta = 1E-10;
	
	@Test public void testOneGroup() throws Exception {
    	String testGroup = "testGroup";
		String objData = 
    		"g "+ testGroup +"\n"+
    		"v 0 0 0\n" + 
    		"v 0 0 1\n" + 
    		"v 1 1 0\n" +
    		"vt 0 0\n" +
    		"vt 0 1\n" +
    		"vt 1 1\n" +
    		"f 1/1 2/2 3/3";
    	SceneGraphComponent root = parseString(objData);
    	SceneGraphComponent child = root.getChildComponent(0);
    	Assert.assertTrue(testGroup.equals(child.getName()));
		IndexedFaceSet g = (IndexedFaceSet)child.getGeometry();
    	
		double[][] vertices = {{0,0,0},{0,0,1},{1,1,0}};
    	TestUtils.testVertexCoordinates(vertices, g, delta);
    	double[][] texture = {{0,0},{0,1},{1,1}};
    	TestUtils.testTextureCoordinates(texture,g,delta);
    	DataList tc = g.getVertexAttributes(Attribute.TEXTURE_COORDINATES);
    	Assert.assertNotNull(tc);
    }
	
	@Test public void testOneGroupFace() throws Exception {
    	String testGroup = "testGroup";
		String objData = 
    		"v 0 0 0\n" + 
    		"v 0 0 1\n" + 
    		"v 1 1 0\n" +
    		"vt 0 0\n" +
    		"vt 0 1\n" +
    		"vt 1 1\n" +
    		"g "+ testGroup +"\n"+
    		"f 1/1 2/2 3/3";
    	SceneGraphComponent root = parseString(objData);
    	SceneGraphComponent child = root.getChildComponent(0);
    	Assert.assertTrue(testGroup.equals(child.getName()));
		IndexedFaceSet g = (IndexedFaceSet)child.getGeometry();
    	
		double[][] vertices = {{0,0,0},{0,0,1},{1,1,0}};
    	TestUtils.testVertexCoordinates(vertices, g, delta);
    	double[][] texture = {{0,0},{0,1},{1,1}};
    	TestUtils.testTextureCoordinates(texture,g,delta);
    }
	
	@Test public void testTwoGroups() throws Exception {
    	String testGroup1 = "testGroup1";
    	String testGroup2 = "testGroup2";
		String objData = 
    		"v 0 0 0\n" + 
    		"v 0 0 1\n" + 
    		"v 1 1 0\n" +
    		"vt 0 0\n" +
    		"vt 0 1\n" +
    		"vt 1 1\n" +
    		"g "+ testGroup1 + " " + testGroup2 + "\n"+
    		"f 1/1 2/2 3/3";
    	SceneGraphComponent root = parseString(objData);
    	SceneGraphComponent child0 = root.getChildComponent(0);
    	SceneGraphComponent child1 = root.getChildComponent(1);
    	Assert.assertTrue(testGroup1.equals(child0.getName()) || testGroup2.equals(child0.getName()));
		IndexedFaceSet g = (IndexedFaceSet)child0.getGeometry();
    	
		double[][] vertices = {{0,0,0},{0,0,1},{1,1,0}};
    	TestUtils.testVertexCoordinates(vertices, g, delta);
    	double[][] texture = {{0,0},{0,1},{1,1}};
    	TestUtils.testTextureCoordinates(texture,g,delta);
    	
    	
		IndexedFaceSet g1 = (IndexedFaceSet)child1.getGeometry();
    	
		double[][] vertices1 = {{0,0,0},{0,0,1},{1,1,0}};
    	TestUtils.testVertexCoordinates(vertices1, g1, delta);
    	double[][] texture1 = {{0,0},{0,1},{1,1}};
    	TestUtils.testTextureCoordinates(texture1,g1,delta);
    }

}
