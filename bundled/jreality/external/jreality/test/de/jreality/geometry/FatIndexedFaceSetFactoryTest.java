package de.jreality.geometry;

import java.util.Arrays;
import java.util.List;

import junit.framework.TestCase;
import de.jreality.geometry.ParametricSurfaceFactory.Immersion;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.IntArrayArray;
import de.jreality.tools.ClickWheelCameraZoomTool;
import de.jreality.tools.DraggingTool;

public class FatIndexedFaceSetFactoryTest extends TestCase {
	
	IndexedFaceSet simpleIfs,quadmIfs,moebiusIfs,holeIfs;
	double[] vertices1;
	int[][] faces1;
	List<int[]> boundary1a, boundary1b;
	FatIndexedFaceSetFactory fatFactory;
	
	SceneGraphComponent sgc;
	
	public void setUp() {
		//ifsf1 - small test example
		IndexedFaceSetFactory ifsf = new IndexedFaceSetFactory();
		vertices1 = new double[]{0,0,0 , 5,0,3 , 6,2,2 , 10,0,2 , 15,0,-2 , 9,3,4};
		faces1 = new int[][]{{0,1,2} , {1,3,5,2} , {3,4,5}  };
		ifsf.setVertexCount(vertices1.length/3);
		ifsf.setVertexCoordinates(vertices1);
		ifsf.setFaceCount(faces1.length);
		ifsf.setFaceIndices(faces1);
		ifsf.setGenerateEdgesFromFaces(true);
		ifsf.setGenerateVertexNormals(true);
		ifsf.update();
		simpleIfs = ifsf.getIndexedFaceSet();
		boundary1a = Arrays.asList(new int[][] {{0,1,3,4} , {4,5,2,0} }); //This should be generated
		boundary1b = Arrays.asList(new int[][] {{0,1,3,4} , {4,5} , {5,2,0} }); //more compts than 1a!!
		
		//simple quad mesh
		QuadMeshFactory quadmf = new QuadMeshFactory();
		quadmf.setULineCount(3);
		quadmf.setVLineCount(2);
		quadmf.setVertexCoordinates(new double[] {0,0,0 , 0,-.5,1 , 0,0,2 , 0,1,0 , 0,1.5,1 , 0,1,2});
		quadmf.setGenerateEdgesFromFaces(true);
		quadmf.setEdgeFromQuadMesh(true);
		quadmf.setGenerateVertexNormals(true);
		quadmf.update();
		quadmIfs=quadmf.getIndexedFaceSet();
		
		//Moebius
		ParametricSurfaceFactory psf = new ParametricSurfaceFactory(new Immersion() {
			public void evaluate(double u, double v, double[] xyz, int index) {
				xyz[index++]= Math.sin(u) * (1+Math.sin(u/2) *v);
				xyz[index++]= Math.cos(u) * (1+Math.sin(u/2) *v);
				xyz[index++]= Math.cos(u/2) *v;
					
			}
			public int getDimensionOfAmbientSpace() {
				return 3;
			}
			public boolean isImmutable() {
				return true;
			}
		});
		psf.setUMin(0); psf.setUMax(2*Math.PI); psf.setVMin(-.3); psf.setVMax(.3);
		psf.setULineCount(40);psf.setVLineCount(10);
		psf.setGenerateVertexNormals(true);
		psf.setGenerateEdgesFromFaces(true);
		psf.update();
		moebiusIfs=psf.getIndexedFaceSet();

		//Hole
		psf = new ParametricSurfaceFactory(new Immersion() {
			public void evaluate(double u, double v, double[] xyz, int index) {
				xyz[index++]= Math.sin(u) * (1+v);
				xyz[index++]= Math.cos(u) * (1+v);
				xyz[index++]= Math.sin(u)*.4;
					
			}
			public int getDimensionOfAmbientSpace() {
				return 3;
			}
			public boolean isImmutable() {
				return true;
			}
		});
		psf.setUMin(0); psf.setUMax(2*Math.PI); psf.setVMin(-.3); psf.setVMax(.3);
		psf.setULineCount(7);psf.setVLineCount(2);
		psf.setGenerateVertexNormals(true);
		psf.setGenerateEdgesFromFaces(true);
		psf.update();
		holeIfs=psf.getIndexedFaceSet();
		int[][] faces = holeIfs.getFaceAttributes(Attribute.INDICES).toIntArrayArray().toIntArrayArray(null);
		for (int i=0; i<faces.length; i++)
			for (int j=0; j<faces[i].length; j++) {
				if (faces[i][j]==13) faces[i][j]=7;
				if (faces[i][j]==6) faces[i][j]=0;
		}
		holeIfs.setFaceAttributes(Attribute.INDICES, new IntArrayArray.Array(faces));
		
		fatFactory = new FatIndexedFaceSetFactory();
	}
	
	public void testSwitchingBoundarySettingAndGeneration() {
		
		//set->generate->set test
		fatFactory.setInputIFS(simpleIfs);
		assertTrue(fatFactory.getBoundaryIFSFs().isEmpty());
		assertTrue(fatFactory.getBoundaryIFSs().isEmpty());
		assertTrue(fatFactory.getBoundaryIndices().isEmpty());
		assertTrue(fatFactory.getBoundarySGCs().isEmpty());
		assertEquals( 2, fatFactory.getAllInOneSceneGraphComponent().getChildComponentCount());

		fatFactory.setBoundaryIndices(boundary1b);
		assertEquals(boundary1b.size(),fatFactory.getBoundaryIFSFs().size());
		assertEquals(boundary1b.size(),fatFactory.getBoundaryIFSs().size());
		assertEquals(boundary1b.size(),fatFactory.getBoundaryIndices().size());
		assertEquals(boundary1b.size(),fatFactory.getBoundarySGCs().size());
		assertEquals(boundary1b.size()+2, fatFactory.getAllInOneSceneGraphComponent().getChildComponentCount());
		List<IndexedFaceSetFactory> ifsfs = fatFactory.getBoundaryIFSFs();
		IndexedFaceSetFactory[] ifsfsa = ifsfs.toArray(new IndexedFaceSetFactory[ifsfs.size()]);
		List<IndexedFaceSet> ifss = fatFactory.getBoundaryIFSs();
		IndexedFaceSet[] ifssa = ifss.toArray(new IndexedFaceSet[ifss.size()]);
		List<SceneGraphComponent> sgcs = fatFactory.getBoundarySGCs();
		SceneGraphComponent[] sgcsa = sgcs.toArray(new SceneGraphComponent[sgcs.size()]);

		fatFactory.update();
		assertSame(ifsfs,fatFactory.getBoundaryIFSFs());
		assertSame(ifss,fatFactory.getBoundaryIFSs());
		assertSame(sgcs,fatFactory.getBoundarySGCs());
		assertTrue(Arrays.equals(ifsfsa,fatFactory.getBoundaryIFSFs().toArray()));
		assertTrue(Arrays.equals(ifssa,fatFactory.getBoundaryIFSs().toArray()));
		assertTrue(Arrays.equals(sgcsa,fatFactory.getBoundarySGCs().toArray()));
		assertEquals(boundary1b.size(),fatFactory.getBoundaryIFSFs().size());
		assertEquals(boundary1b.size(),fatFactory.getBoundaryIFSs().size());
		assertEquals(boundary1b.size(),fatFactory.getBoundaryIndices().size());
		assertEquals(boundary1b.size(),fatFactory.getBoundarySGCs().size());
		assertEquals(boundary1b.size()+2, fatFactory.getAllInOneSceneGraphComponent().getChildComponentCount());

		fatFactory.setGenerateBoundaryIndices(true);
		fatFactory.update();
		assertSame(ifsfs,fatFactory.getBoundaryIFSFs());
		assertSame(ifss,fatFactory.getBoundaryIFSs());
		assertSame(sgcs,fatFactory.getBoundarySGCs());
		assertEquals(boundary1a.size(),fatFactory.getBoundaryIFSFs().size());
		assertEquals(boundary1a.size(),fatFactory.getBoundaryIFSs().size());
		assertEquals(boundary1a.size(),fatFactory.getBoundaryIndices().size());
		assertEquals(boundary1a.size(),fatFactory.getBoundarySGCs().size());
		assertEquals(boundary1a.size()+2, fatFactory.getAllInOneSceneGraphComponent().getChildComponentCount());
		ifsfsa = ifsfs.toArray(new IndexedFaceSetFactory[ifsfs.size()]);
		ifssa = ifss.toArray(new IndexedFaceSet[ifss.size()]);
		sgcsa = sgcs.toArray(new SceneGraphComponent[sgcs.size()]);
		
		fatFactory.setGenerateBoundaryIndices(false);
		fatFactory.setBoundaryIndices(boundary1a);
		assertSame(ifsfs,fatFactory.getBoundaryIFSFs());
		assertSame(ifss,fatFactory.getBoundaryIFSs());
		assertSame(sgcs,fatFactory.getBoundarySGCs());
		assertTrue(Arrays.equals(ifsfsa,fatFactory.getBoundaryIFSFs().toArray()));
		assertTrue(Arrays.equals(ifssa,fatFactory.getBoundaryIFSs().toArray()));
		assertTrue(Arrays.equals(sgcsa,fatFactory.getBoundarySGCs().toArray()));
		assertEquals(boundary1a.size(),fatFactory.getBoundaryIFSFs().size());
		assertEquals(boundary1a.size(),fatFactory.getBoundaryIFSs().size());
		assertEquals(boundary1a.size(),fatFactory.getBoundaryIndices().size());
		assertEquals(boundary1a.size()+2,fatFactory.getAllInOneSceneGraphComponent().getChildComponentCount());

		fatFactory.update();
		assertSame(ifsfs,fatFactory.getBoundaryIFSFs());
		assertSame(ifss,fatFactory.getBoundaryIFSs());
		assertSame(sgcs,fatFactory.getBoundarySGCs());
		assertTrue(Arrays.equals(ifsfsa,fatFactory.getBoundaryIFSFs().toArray()));
		assertTrue(Arrays.equals(ifssa,fatFactory.getBoundaryIFSs().toArray()));
		assertTrue(Arrays.equals(sgcsa,fatFactory.getBoundarySGCs().toArray()));
		assertEquals(boundary1a.size(),fatFactory.getBoundaryIFSFs().size());
		assertEquals(boundary1a.size(),fatFactory.getBoundaryIFSs().size());
		assertEquals(boundary1a.size(),fatFactory.getBoundaryIndices().size());
		assertEquals(boundary1a.size(),fatFactory.getBoundarySGCs().size());
		assertEquals(boundary1a.size()+2,fatFactory.getAllInOneSceneGraphComponent().getChildComponentCount());

		//set boundary to null test
		fatFactory.setBoundaryIndices(null);
		fatFactory.update();
		assertSame(ifsfs,fatFactory.getBoundaryIFSFs());
		assertSame(ifss,fatFactory.getBoundaryIFSs());
		assertSame(sgcs,fatFactory.getBoundarySGCs());
		assertEquals(0,fatFactory.getBoundaryIFSFs().size());
		assertEquals(0,fatFactory.getBoundaryIFSs().size());
		assertEquals(null,fatFactory.getBoundaryIndices());
		assertEquals(0,fatFactory.getBoundarySGCs().size());
		assertEquals(2,fatFactory.getAllInOneSceneGraphComponent().getChildComponentCount());
		
		//initBoundary test
		fatFactory.initBoundary(boundary1b.size());
		assertEquals(boundary1b.size(),fatFactory.getBoundaryIFSFs().size());
		assertEquals(boundary1b.size(),fatFactory.getBoundaryIFSs().size());
		assertEquals(boundary1b.size(),fatFactory.getBoundaryIndices().size());
		assertEquals(boundary1b.size(),fatFactory.getBoundarySGCs().size());
		assertEquals(boundary1b.size()+2,fatFactory.getAllInOneSceneGraphComponent().getChildComponentCount());
		ifsfs = fatFactory.getBoundaryIFSFs();
		ifsfsa = ifsfs.toArray(new IndexedFaceSetFactory[ifsfs.size()]);
		ifss = fatFactory.getBoundaryIFSs();
		ifssa = ifss.toArray(new IndexedFaceSet[ifss.size()]);
		sgcs = fatFactory.getBoundarySGCs();
		sgcsa = sgcs.toArray(new SceneGraphComponent[sgcs.size()]);
		
		fatFactory.setBoundaryIndices(boundary1b);
		fatFactory.update();
		assertSame(ifsfs,fatFactory.getBoundaryIFSFs());
		assertSame(ifss,fatFactory.getBoundaryIFSs());
		assertSame(sgcs,fatFactory.getBoundarySGCs());
		assertTrue(Arrays.equals(ifsfsa,fatFactory.getBoundaryIFSFs().toArray()));
		assertTrue(Arrays.equals(ifssa,fatFactory.getBoundaryIFSs().toArray()));
		assertTrue(Arrays.equals(sgcsa,fatFactory.getBoundarySGCs().toArray()));
		assertEquals(boundary1b.size(),fatFactory.getBoundaryIFSFs().size());
		assertEquals(boundary1b.size(),fatFactory.getBoundaryIFSs().size());
		assertEquals(boundary1b.size(),fatFactory.getBoundaryIndices().size());
		assertEquals(boundary1b.size(),fatFactory.getBoundarySGCs().size());
		assertEquals(boundary1b.size()+2,fatFactory.getAllInOneSceneGraphComponent().getChildComponentCount());

		fatFactory.setBoundaryIndices(boundary1b);
		fatFactory.update();
		assertSame(ifsfs,fatFactory.getBoundaryIFSFs());
		assertSame(ifss,fatFactory.getBoundaryIFSs());
		assertSame(sgcs,fatFactory.getBoundarySGCs());
		assertTrue(Arrays.equals(ifsfsa,fatFactory.getBoundaryIFSFs().toArray()));
		assertTrue(Arrays.equals(ifssa,fatFactory.getBoundaryIFSs().toArray()));
		assertTrue(Arrays.equals(sgcsa,fatFactory.getBoundarySGCs().toArray()));
		assertEquals(boundary1b.size(),fatFactory.getBoundaryIFSFs().size());
		assertEquals(boundary1b.size(),fatFactory.getBoundaryIFSs().size());
		assertEquals(boundary1b.size(),fatFactory.getBoundaryIndices().size());
		assertEquals(boundary1b.size(),fatFactory.getBoundarySGCs().size());
		assertEquals(boundary1b.size()+2,fatFactory.getAllInOneSceneGraphComponent().getChildComponentCount());
				
		//re-initBoundary test
		fatFactory.initBoundary(boundary1a.size());
		assertEquals(boundary1a.size(),fatFactory.getBoundaryIFSFs().size());
		assertEquals(boundary1a.size(),fatFactory.getBoundaryIFSs().size());
		assertEquals(boundary1a.size(),fatFactory.getBoundaryIndices().size());
		assertEquals(boundary1a.size(),fatFactory.getBoundarySGCs().size());
		assertEquals(boundary1a.size()+2,fatFactory.getAllInOneSceneGraphComponent().getChildComponentCount());
		ifsfs = fatFactory.getBoundaryIFSFs();
		ifsfsa = ifsfs.toArray(new IndexedFaceSetFactory[ifsfs.size()]);
		ifss = fatFactory.getBoundaryIFSs();
		ifssa = ifss.toArray(new IndexedFaceSet[ifss.size()]);
		sgcs = fatFactory.getBoundarySGCs();
		sgcsa = sgcs.toArray(new SceneGraphComponent[sgcs.size()]);

		fatFactory.setBoundaryIndices(boundary1a);
		fatFactory.update();
		assertSame(ifsfs,fatFactory.getBoundaryIFSFs());
		assertSame(ifss,fatFactory.getBoundaryIFSs());
		assertSame(sgcs,fatFactory.getBoundarySGCs());
		assertTrue(Arrays.equals(ifsfsa,fatFactory.getBoundaryIFSFs().toArray()));
		assertTrue(Arrays.equals(ifssa,fatFactory.getBoundaryIFSs().toArray()));
		assertTrue(Arrays.equals(sgcsa,fatFactory.getBoundarySGCs().toArray()));
		assertEquals(boundary1a.size(),fatFactory.getBoundaryIFSFs().size());
		assertEquals(boundary1a.size(),fatFactory.getBoundaryIFSs().size());
		assertEquals(boundary1a.size(),fatFactory.getBoundaryIndices().size());
		assertEquals(boundary1a.size(),fatFactory.getBoundarySGCs().size());
		assertEquals(boundary1a.size()+2,fatFactory.getAllInOneSceneGraphComponent().getChildComponentCount());

		fatFactory.setGenerateBoundaryIndices(true);
		fatFactory.update();
		assertSame(ifsfs,fatFactory.getBoundaryIFSFs());
		assertSame(ifss,fatFactory.getBoundaryIFSs());
		assertSame(sgcs,fatFactory.getBoundarySGCs());
		assertTrue(Arrays.equals(ifsfsa,fatFactory.getBoundaryIFSFs().toArray()));
		assertTrue(Arrays.equals(ifssa,fatFactory.getBoundaryIFSs().toArray()));
		assertTrue(Arrays.equals(sgcsa,fatFactory.getBoundarySGCs().toArray()));
		assertEquals(boundary1a.size(),fatFactory.getBoundaryIFSFs().size());
		assertEquals(boundary1a.size(),fatFactory.getBoundaryIFSs().size());
		assertEquals(boundary1a.size(),fatFactory.getBoundaryIndices().size());
		assertEquals(boundary1a.size(),fatFactory.getBoundarySGCs().size());
		assertEquals(boundary1a.size()+2,fatFactory.getAllInOneSceneGraphComponent().getChildComponentCount());


	}
	
	public void testSetBoundarySimpleIfs() {
		fatFactory.setInputIFS(simpleIfs);
		fatFactory.setBoundaryIndices(boundary1b);
		fatFactory.setGenerateBoundaryIndices(true);
		fatFactory.update();		

		assertEquals( 2, fatFactory.getBoundaryIFSFs().size());
		assertEquals( 2, fatFactory.getBoundaryIFSs().size());
		assertEquals( 2, fatFactory.getBoundarySGCs().size());
		assertEquals( 4, fatFactory.getAllInOneSceneGraphComponent().getChildComponentCount());		
		assertTrue(Arrays.deepEquals(
				(int[][]) boundary1a.toArray(new int[boundary1a.size()][]),
				(int[][]) fatFactory.getBoundaryIndices().toArray(new int[ fatFactory.getBoundaryIndices().size()][])));
		boundary1a = Arrays.asList(new int[][] {{0,1,3,4} , {4,5,2,0} }); //This should be generated
		boundary1b = Arrays.asList(new int[][] {{0,1,3,4} , {4,5} , {5,2,0} }); //more compts than 1a!!
		assertTrue(Arrays.deepEquals(
				new int[][] {{1,0,4,5},{2,1,5,6},{3,2,6,7}},
				fatFactory.getBoundaryIFSs().get(0).getFaceAttributes(Attribute.INDICES).toIntArrayArray().toIntArrayArray(null)));
		assertTrue(Arrays.deepEquals(
				new int[][] {{1,0,4,5},{2,1,5,6},{3,2,6,7}},
				fatFactory.getBoundaryIFSs().get(1).getFaceAttributes(Attribute.INDICES).toIntArrayArray().toIntArrayArray(null)));

		fatFactory.setInputIFS(simpleIfs);
		fatFactory.setGenerateBoundaryIndices(false);
		fatFactory.setBoundaryIndices(boundary1b);
		fatFactory.update();		
		assertEquals( 3, fatFactory.getBoundaryIFSFs().size());
		assertEquals( 3, fatFactory.getBoundaryIFSs().size());
		assertEquals( 3, fatFactory.getBoundarySGCs().size());
		assertEquals( 5, fatFactory.getAllInOneSceneGraphComponent().getChildComponentCount());		
		assertTrue(Arrays.deepEquals(
				(int[][]) boundary1b.toArray(new int[boundary1a.size()][]),
				(int[][]) fatFactory.getBoundaryIndices().toArray(new int[ fatFactory.getBoundaryIndices().size()][])));
		assertTrue(Arrays.deepEquals(
				new int[][] {{1,0,4,5},{2,1,5,6},{3,2,6,7}},
				fatFactory.getBoundaryIFSs().get(0).getFaceAttributes(Attribute.INDICES).toIntArrayArray().toIntArrayArray(null)));
		assertTrue(Arrays.deepEquals(
				new int[][] {{1,0,2,3}},
				fatFactory.getBoundaryIFSs().get(1).getFaceAttributes(Attribute.INDICES).toIntArrayArray().toIntArrayArray(null)));
		assertTrue(Arrays.deepEquals(
				new int[][] {{1,0,3,4},{2,1,4,5}},
				fatFactory.getBoundaryIFSs().get(2).getFaceAttributes(Attribute.INDICES).toIntArrayArray().toIntArrayArray(null)));
	
		sgc=fatFactory.getAllInOneSceneGraphComponent();
	}

	public void testGenerateBoundarySimpleQuadmesh() {
		fatFactory.setInputIFS(quadmIfs);
		fatFactory.setGenerateBoundaryIndices(true);
		fatFactory.update();		

		assertEquals( 4, fatFactory.getBoundaryIFSFs().size());
		assertEquals( 4, fatFactory.getBoundaryIFSs().size());
		assertEquals( 4, fatFactory.getBoundarySGCs().size());
		assertEquals( 6, fatFactory.getAllInOneSceneGraphComponent().getChildComponentCount());
		
		sgc=fatFactory.getAllInOneSceneGraphComponent();
	}
	
	public void testGenerateBoundaryMoebius() {
		fatFactory.setInputIFS(moebiusIfs);
		fatFactory.setGenerateBoundaryIndices(true);
		fatFactory.update();		

		assertEquals( 4, fatFactory.getBoundaryIFSFs().size());
		assertEquals( 4, fatFactory.getBoundaryIFSs().size());
		assertEquals( 4, fatFactory.getBoundarySGCs().size());
		assertEquals( 6, fatFactory.getAllInOneSceneGraphComponent().getChildComponentCount());
		
		sgc=fatFactory.getAllInOneSceneGraphComponent();
	}

	public void testGenerateBoundaryHole() {

		fatFactory.setInputIFS(holeIfs);
		fatFactory.setGenerateBoundaryIndices(true);
		fatFactory.update();
		
		assertEquals( 2, fatFactory.getBoundaryIFSFs().size());
		assertEquals( 2, fatFactory.getBoundaryIFSs().size());
		assertEquals( 2, fatFactory.getBoundarySGCs().size());
		assertEquals( 4, fatFactory.getAllInOneSceneGraphComponent().getChildComponentCount());
		
		sgc=fatFactory.getAllInOneSceneGraphComponent();
	}

	public static void main(String[] args) {
		FatIndexedFaceSetFactoryTest test = new FatIndexedFaceSetFactoryTest();

		test.setUp();	
		test.testGenerateBoundarySimpleQuadmesh();
		test.sgc.addTool(new ClickWheelCameraZoomTool());
		test.fatFactory.getTopSGC().addTool(new DraggingTool());
		JRViewer.display(test.sgc);

		test.setUp();
		test.testSetBoundarySimpleIfs();
		test.sgc.addTool(new ClickWheelCameraZoomTool());
		test.fatFactory.getTopSGC().addTool(new DraggingTool());
		JRViewer.display(test.sgc);


		test.setUp();
		test.testGenerateBoundaryMoebius();
		test.sgc.addTool(new ClickWheelCameraZoomTool());
		test.fatFactory.getTopSGC().addTool(new DraggingTool());
		JRViewer.display(test.sgc);
	
		test.setUp();
		test.testGenerateBoundaryHole();
		test.sgc.addTool(new ClickWheelCameraZoomTool());
		test.fatFactory.getTopSGC().addTool(new DraggingTool());
		JRViewer.display(test.sgc);
	}
}
