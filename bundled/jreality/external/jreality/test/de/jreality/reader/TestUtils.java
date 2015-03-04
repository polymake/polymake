package de.jreality.reader;

import java.io.ByteArrayInputStream;
import java.io.IOException;

import org.junit.Assert;

import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.data.Attribute;
import de.jreality.util.Input;

public class TestUtils {

	public static void testVertexCoordinates(double[][] vertices, IndexedFaceSet ifs, double delta) {
	    double[][] verticesIFS = ifs.getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray(null);
		testDoubleArrayArray(vertices, verticesIFS, delta);
	}

	public static void testIntArrayArray(int[][] edges, int[][] edgesIFS) {
		Assert.assertEquals(edges.length, edgesIFS.length);
//	    
//	    for(int i = 0; i < edges.length; ++i) {
//	    	Arrays.sort(edges[i]);
//	    	Arrays.sort(edgesIFS[i]);
//	    }
//	    Comparator<int[]> cmp = new Comparator<int[]>() {
//			@Override
//			public int compare(int[] o1, int[] o2) {
//				for(int i = 0; i < o1.length; ++i) {
//					if(o1[i] != o2[i]) {
//						return o1[i] - o2[i];
//					}
//				}
//				return 0;
//			}};
//	    Arrays.sort(edges,cmp);
//	    Arrays.sort(edgesIFS, cmp);
	    
	    for(int i = 0; i < edges.length; ++i) {
	    	Assert.assertArrayEquals(edges[i], edgesIFS[i]);
	    }
	}

	public static void testDoubleArrayArray(double[][] coords, double[][] coordsIFS, double delta) {
		Assert.assertEquals(coords.length, coordsIFS.length);
	    
	    for(int i = 0; i < coords.length; ++i) {
	    	Assert.assertArrayEquals(coords[i], coordsIFS[i], delta);
	    }
	}

	public static void testEdgeIndices(int[][] edges, IndexedFaceSet ifs) {
		int[][] edgesIFS = ifs.getEdgeAttributes(Attribute.INDICES).toIntArrayArray(null);
	    testIntArrayArray(edges, edgesIFS);
	}

	public static void testTextureCoordinates(double[][] texture, IndexedFaceSet g, double delta) {
		double[][] textureIFS = g.getVertexAttributes(Attribute.TEXTURE_COORDINATES).toDoubleArrayArray(null);
		testDoubleArrayArray(texture, textureIFS, delta);
	}
	
	public static SceneGraphComponent parseString(String objData) throws IOException {
		ByteArrayInputStream stream = new ByteArrayInputStream(objData.getBytes());
		Input input = new Input("Direct String Data", stream);
		ReaderOBJ o = new ReaderOBJ();
        SceneGraphComponent root = o.read(input);
		return root;
	}

}
