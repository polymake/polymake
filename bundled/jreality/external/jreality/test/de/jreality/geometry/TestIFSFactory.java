package de.jreality.geometry;

import junit.framework.TestCase;
import de.jreality.plugin.JRViewer;
import de.jreality.plugin.content.ContentAppearance;
import de.jreality.plugin.content.ContentLoader;
import de.jreality.plugin.content.ContentTools;
import de.jreality.scene.IndexedFaceSet;

public class TestIFSFactory extends TestCase {

	public IndexedFaceSet testFaceColors()	{
		IndexedFaceSetFactory borromeanRectFactory = null;
		double[][] jitterbugEdgeVerts = new double[][] {{0,0,0,1},{1,0,0,1},{1,1,0,1},{0,1,0,1}};
		int[][] jitterbugSegmentIndices1 = {{0,1},{2,3}}; //,{{{0,1,2,3,0,1},{4,5,6,7,4,5},{8,9,10,11,8,9}};
		int[][] jitterbugFaceIndices = {{0,1,2,3}};
		double[][] borromeanRectColors = {{1.0, 1.0, 200/255.0, 1}};

				borromeanRectFactory = new IndexedFaceSetFactory();
				borromeanRectFactory.setVertexCount(jitterbugEdgeVerts.length);
				borromeanRectFactory.setVertexCoordinates(jitterbugEdgeVerts);	
				borromeanRectFactory.setFaceCount(1);
				borromeanRectFactory.setFaceIndices(jitterbugFaceIndices);	
				borromeanRectFactory.setFaceColors(new double[][]{{1,0,0}});
				borromeanRectFactory.setGenerateFaceNormals(true);
				borromeanRectFactory.setEdgeCount(jitterbugSegmentIndices1.length);
				borromeanRectFactory.setEdgeIndices(jitterbugSegmentIndices1);
				borromeanRectFactory.update();
				borromeanRectFactory.getIndexedLineSet();			
				//System.err.println("Alpha channel is "+borromeanRectFactory.getIndexedFaceSet().getFaceAttributes(Attribute.COLORS).toDoubleArrayArray(null)[0][3]);
				// just to be safe, we don't use the old array but create a new one.
				borromeanRectFactory.setFaceColors(new double[][]{{0,1,0}});
				borromeanRectFactory.update();
				//System.err.println("Alpha channel is "+borromeanRectFactory.getIndexedFaceSet().getFaceAttributes(Attribute.COLORS).toDoubleArrayArray(null)[0][3]);
				
				
				return borromeanRectFactory.getIndexedFaceSet();
	}
	
	public static  void main( String [] arg ) {
		TestIFSFactory test = new TestIFSFactory();
	    JRViewer v = new JRViewer();
		v.addBasicUI();
		v.setContent(test.testFaceColors());
		v.registerPlugin(new ContentAppearance());
		v.registerPlugin(new ContentLoader());
		v.registerPlugin(new ContentTools());
		v.startup();
	}
}
