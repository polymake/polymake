package de.jreality.geometry;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashSet;
import java.util.Iterator;
import java.util.LinkedHashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Set;
import java.util.Vector;

import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.DataList;
import de.jreality.scene.data.DataListSet;
import de.jreality.scene.data.DoubleArray;
import de.jreality.scene.data.IntArray;

/** Fatten an <code>IndexedFaceSet</code>. This factory makes out of one <code>IndexedFaceSet</code>
 * two <code>IndexedFaceSet</code>s, top and bottom, by moving the given <code>IndexedFaceSet</code> along its
 * vertex normals "up" and "down" (in this process the length of the vertex normals
 * is not normalized). It may also generate <code>IndexedFaceSet</code>s for the 
 * boundary. All vertex, edges, and face attributes of the given <code>IndexedFaceSet</code> are
 * copied to the top and bottom 
 * <code>IndexedFaceSet</code>s. For the boundary this must be handled elsewhere.
 * 
 * <p> The {@link ThickenedSurfaceFactory} also fattens a given <code>IndexedFaceSet</code>, but
 * puts everything into one <code>IndexedFaceSet</code>. It is also not flexible at the
 * boundary. On the other hand, the <code>ThickenedSurfaceFactory</code> 
 * can make holes into the face, which is not possible with
 * the <code>FatIndexedFaceSetFactory</code>.
 * 
 * @author G. Paul Peters, Sep 23, 2009

 * @see ThickenedSurfaceFactory 
 *
 */
public class FatIndexedFaceSetFactory {
	
	private IndexedFaceSet inputIFS;
	private List<int[]> boundaryIndices=new ArrayList<int[]>();
	private double fatness=.05;
	private boolean generateBoundaryIndices=false;

	private final IndexedFaceSet topIFS;
	private final IndexedFaceSet bottomIFS;
	private final List<IndexedFaceSet> boundaryIFSs=new ArrayList<IndexedFaceSet>();
	private final IndexedFaceSetFactory topIFSF=new IndexedFaceSetFactory();
	private final IndexedFaceSetFactory bottomIFSF=new IndexedFaceSetFactory();
	private final List<IndexedFaceSetFactory> boundaryIFSFs = new ArrayList<IndexedFaceSetFactory>();
	private final SceneGraphComponent compoundSGC=new SceneGraphComponent();
	private final SceneGraphComponent topSGC=new SceneGraphComponent();	
	private final SceneGraphComponent bottomSGC=new SceneGraphComponent();
	private final List<SceneGraphComponent> boundarySGCs=new ArrayList<SceneGraphComponent>();
	

	public FatIndexedFaceSetFactory() {
		topIFS=topIFSF.getIndexedFaceSet();
		topSGC.setGeometry(topIFS);
		bottomIFS=bottomIFSF.getIndexedFaceSet();
		bottomSGC.setGeometry(bottomIFS);
		
		compoundSGC.addChild(topSGC);
		compoundSGC.addChild(bottomSGC);
	}
	
	public IndexedFaceSet getInputIFS() {
		return inputIFS;
	}


	/** The <code>IndexedFaceSet</code> to be fattened.
	 * 
	 * @param inputIFS
	 */
	public void setInputIFS(IndexedFaceSet inputIFS) {
		this.inputIFS = inputIFS;
		compoundSGC.setName("fat "+inputIFS.getName());
		topIFS.setName("Top("+inputIFS.getName()+")");
		topSGC.setName(topIFS.getName());
		bottomIFS.setName("Bottom("+inputIFS.getName()+")");
		bottomSGC.setName(bottomIFS.getName());
	}

	public double getFatness() {
		return 2*fatness;
	}

	/** The vertex coordinates of the top and bottom <code>IndexedFaceSet</code>s 
	 * are obtained by adding &plusmn;&frac12;<code>fatness</code> times the vertex normal
	 * to the given vertex.
	 * 
	 * @param fatness
	 */
	public void setFatness(double fatness) {
		this.fatness = fatness/2;
	}

	public boolean isGenerateBoundaryIndices() {
		return generateBoundaryIndices;
	}

	/** Set to true, when the factory should detect the boundary edges.  
	 * A boundary edge is an edge in the given <code>IndexedFaceSet</code>
	 * that does only occur once in the list of face indices 
	 * of the <code>IndexedFaceSet</code>, i.e., has only one adjacent face. 
	 * A boundary component is either closed or starts and ends at vertices,
	 * that only have two adjacent edges. This leads to correct, i.e., four, 
	 * boundary components in the case of <code>IndexedFaceSet</code>s produced
	 * by a {@link QuadMeshFactory}.
	 * 
	 * @param generateBoundaryIndices
	 */
	public void setGenerateBoundaryIndices(boolean generateBoundaryIndices) {	
		this.generateBoundaryIndices = generateBoundaryIndices;
	}
	
	public List<int[]> getBoundaryIndices() {
		return boundaryIndices==null?null:Collections.unmodifiableList(boundaryIndices);
	}

	/** Determines the boundary components.
	 * 
	 * @param boundaryIndices of boundary components. Each entry is an int array, where
	 * consecutive indices describe an edge (they point to the start and end vertices of that edge). 
	 */
	public void setBoundaryIndices(List<int[]> boundaryIndices)  {
		if (generateBoundaryIndices)
			throw new IllegalStateException("You can not set the boundary indices when boundary indices are to be generated.");
		setBoundaryIndicesImpl(boundaryIndices);
	}

	private void setBoundaryIndicesImpl(List<int[]> boundaryIndices) {
		if (this.boundaryIndices == boundaryIndices) return;
		
		boolean nbOfComponentsChanged=
			this.boundaryIndices == null || boundaryIndices == null
			|| this.boundaryIndices.size() != boundaryIndices.size();
		
		this.boundaryIndices = boundaryIndices;
		
		if (nbOfComponentsChanged) {
			boundaryIFSs.clear();
			boundaryIFSFs.clear();
			for (SceneGraphComponent sgc : boundarySGCs) compoundSGC.removeChild(sgc);
			boundarySGCs.clear();
		}

		if (boundaryIndices==null) return;

		for (int i = 0; i < boundaryIndices.size(); i++) {
			IndexedFaceSet ifs;
			SceneGraphComponent sgc;
			if (nbOfComponentsChanged){
				IndexedFaceSetFactory ifsf = new IndexedFaceSetFactory();
				ifs = ifsf.getIndexedFaceSet();
				sgc=new SceneGraphComponent();

				boundaryIFSFs.add(ifsf);;
				boundaryIFSs.add(ifs);
				compoundSGC.addChild(sgc);
				boundarySGCs.add(sgc);
				ifsf.setGenerateVertexNormals(true);			
				sgc.setGeometry(ifs);

			}
			else {
				ifs=boundaryIFSs.get(i);
				sgc=boundarySGCs.get(i);
			}

			ifs.setName("Boundary("+inputIFS.getName()+")["+i+"]");
			sgc.setName(ifs.getName());
		}
	}
	
	/** Use this method to initialize the lists returned by {@link #getBoundaryIFSFs()}, {@link #getBoundaryIFSs()}, and 
	 * {@link #getBoundarySGCs()}. This is useful when boundary components are to be generated, the number of boundary 
	 * components is known, but one wants to set properties e.g. of the <code>IndexedFaceSetFactory</code> that 
	 * generates a boundary component before {@link #update()} is called.  Otherwise it is not necessary to call this
	 * method. Then the boundary components are initialized when {@link #update()} is called.
	 * 
	 * @param expectedNbOfBoundaryComponents
	 */
	public void initBoundary(int expectedNbOfBoundaryComponents) {
		setBoundaryIndicesImpl(Arrays.asList(new int[expectedNbOfBoundaryComponents][]));
	}

	/** The returned reference is final.
	 * 
	 */
	public IndexedFaceSet getTopIFS() {
		return topIFS;
	}


	/** The returned reference is final.
	 * 
	 */
	public IndexedFaceSet getBottomIFS() {
		return bottomIFS;
	}

	/** The returned reference is final. The references in the list only change, when the number of
	 * boundary components changes. 
	 * 
	 */
	public List<IndexedFaceSet>getBoundaryIFSs() {
		return boundaryIFSs;
	}
	
	/** The returned reference is final. Change the properties of the returned factory
	 *  to influence the generation of 
	 * the top indexed face set.
	 * 
	 * @return the factory that is used to generate the top indexed face set. 
	 */
	public IndexedFaceSetFactory getTopIFSF() {
		return topIFSF;
	}

	/** The returned reference is final. Change the properties of the returned factory
	 * to influence the generation of the bottom indexed face set.
	 * 
	 * @return the factory that is used to generate the bottom indexed face set. 
	 */
	public IndexedFaceSetFactory getBottomIFSF() {
		return bottomIFSF;
	}

	/** The returned reference is final. The references in the list only change, when the number of
	 * boundary components changes. Change the properties of the returned factory
	 *  to influence the generation of 
	 * the boundary indexed face sets.
	 * 
	 * @return the factories that are used to generate the boundary indexed face sets. 
	 */
	public List<IndexedFaceSetFactory> getBoundaryIFSFs() {
		return boundaryIFSFs;
	}

	/** The returned reference is final. 
	 * 
	 * @return the <code>SceneGraphComponent</code> with the top indexed face set as geometry. 
	 */
	public SceneGraphComponent getTopSGC() {
		return topSGC;
	}

	/** The returned reference is final. 
	 * 
	 * @return the <code>SceneGraphComponent</code> with the bottom indexed face set as geometry. 
	 */
	public SceneGraphComponent getBottomSGC() {
		return bottomSGC;
	}

	/** The returned reference is final. The references in the list only change, when the number of
	 * boundary components changes.
	 * 
	 * @return the list of <code>SceneGraphComponent</code>s with the 
	 * boundary indexed face sets as geometry. 
	 */
	public List<SceneGraphComponent> getBoundarySGCs() {
		return boundarySGCs;
	}

	/** Returns all components of the fat indexed face set as children of one <code>SceneGraphComponent</code>.
	 * 
	 */
	public SceneGraphComponent getAllInOneSceneGraphComponent() {
		return compoundSGC;
	}

	

	/** Call this to generate the first time or update the components of the fat indexed face set 
	 * whenever ingredients - in particular the input index face set - change. 
	 * 
	 * @throws UnsupportedOperationException when the input index face set ({@link #setInputIFS(IndexedFaceSet)})
	 * is null, has no faces or no vertex normals.
	 */
	public void update()  {
		if (inputIFS==null) 
			throw new UnsupportedOperationException("The input IndexedFaceSet needs to be set first.");
		if (inputIFS.getFaceAttributes(Attribute.INDICES)==null) {
			throw new UnsupportedOperationException("The input IndexedFaceSet has no faces.");
		}
		if (inputIFS.getVertexAttributes(Attribute.NORMALS)==null) {
			throw new UnsupportedOperationException("The input IndexedFaceSet has no VertexNormals.");
		}
		updateImpl();
	}
	
	private void updateImpl() {
		copyAllAttributes(inputIFS, topIFSF, true, true, true);
		copyAllAttributes(inputIFS, bottomIFSF, true, true, true);
		
		setWithFlippedNormalsAndFaces();	

		setMovedVertices(topIFSF,fatness);
		setMovedVertices(bottomIFSF,-fatness);
 		
		topIFSF.update();
		bottomIFSF.update();
		
		if (generateBoundaryIndices) generateBoundaryIndices();
		
		if (boundaryIndices != null) {
			updateBoundaryFaces();
		}
		for (IndexedFaceSetFactory ifsf : boundaryIFSFs) ifsf.update();
	}


	private void copyAllAttributes(IndexedFaceSet from, IndexedFaceSetFactory to, 
			boolean vertices, boolean edges, boolean faces) {
		if (vertices) {
			to.setVertexCount(from.getNumPoints());
			to.setVertexAttributes(from.getVertexAttributes());
		}
		if (faces) {
			to.setFaceCount(from.getNumFaces());
			to.setFaceAttributes(from.getFaceAttributes());
		}
		if (edges) {
			to.setEdgeCount(from.getNumEdges());
			DataListSet edgeAttributes = from.getEdgeAttributes();
			if (edgeAttributes != null)
				for (Attribute attribute : edgeAttributes.storedAttributes()) 
					to.setEdgeAttribute(attribute, from.getEdgeAttributes(attribute));
		}
	}

	private void setWithFlippedNormalsAndFaces() {
		double[][] normals=inputIFS.getVertexAttributes(Attribute.NORMALS).toDoubleArrayArray(null);
		for (int i = 0; i < normals.length; i++) 
			for (int j=0; j<normals[i].length; j++)
				normals[i][j]=-normals[i][j];
		bottomIFSF.setVertexNormals(normals);
		
		int[][] fIndices=inputIFS.getFaceAttributes(Attribute.INDICES).toIntArrayArray(null);
		int d;
		for (int i = 0; i < fIndices.length; i++) 
			for (int j=0; j<fIndices[i].length / 2; j++){
				d=fIndices[i][fIndices[i].length-1-j];
				fIndices[i][fIndices[i].length-1-j]=fIndices[i][j];
				fIndices[i][j]=d;
			}
		bottomIFSF.setFaceIndices(fIndices);
	}

	private void setMovedVertices(IndexedFaceSetFactory ifsf, double epsilon) {
		double[][] coordinates=inputIFS.getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray(null);
		DataList normals = inputIFS.getVertexAttributes(Attribute.NORMALS);
		DoubleArray normal;
		for (int i = 0; i < coordinates.length; i++) {
			normal=normals.item(i).toDoubleArray();
			for (int j=0; j<coordinates[i].length; j++) {
				coordinates[i][j]=coordinates[i][j]+epsilon* normal.getValueAt(j);//[i][j];
			}
		}
		ifsf.setVertexCoordinates(coordinates);		
	}

	/** Assumes that the vertices of top and bottom IFS are already in their final position.
	 */
	private void updateBoundaryFaces() {
		DataList topVertices = topIFS.getVertexAttributes(Attribute.COORDINATES);
		DataList bottomVertices = bottomIFS.getVertexAttributes(Attribute.COORDINATES);
		
		for (int i=0; i<boundaryIndices.size(); i++) {
			int[] bComp=boundaryIndices.get(i);
			IndexedFaceSetFactory ifsf = boundaryIFSFs.get(i);

			double[][] vertices=new double[2*bComp.length][];
			int[][] faces=new int[bComp.length-1][4];

			vertices[0] = topVertices.item(bComp[0]).toDoubleArray().toDoubleArray(null);
			vertices[bComp.length] = bottomVertices.item(bComp[0]).toDoubleArray().toDoubleArray(null);
			for (int j=0; j<bComp.length-1; j++){
				vertices[j+1] = topVertices.item(bComp[j+1]).toDoubleArray().toDoubleArray(null);
				vertices[j+bComp.length+1] = bottomVertices.item(bComp[j+1]).toDoubleArray().toDoubleArray(null);
				faces[j][0]=j+1;
				faces[j][1]=j;
				faces[j][2]=bComp.length+j;
				faces[j][3]=bComp.length+j+1;
			}
			
			ifsf.setVertexCount(vertices.length);
			ifsf.setVertexCoordinates(vertices);
			ifsf.setFaceCount(faces.length);
			ifsf.setFaceIndices(faces);
		}
	}

	private void generateBoundaryIndices() {
		// a list of adjacent vertices of a vertex, sign indicates orientation of the connecting edge,
		// edges that occur in both directions are removed (twice or more in one direction .... god knows)
		// vertex numbering starts with 1 here, because 0 has no sign.
		Vector<List<Integer>> edgesAtVertex = new Vector<List<Integer>>(inputIFS.getNumPoints()+1);
		edgesAtVertex.setSize(inputIFS.getNumPoints()+1);
		// vertices on the boundary that have no inner edge, again vertex numbers start at 1 for consistancy 
		boolean[] isOnlyOuterBoundaryVertex=new boolean[inputIFS.getNumPoints()+1];
		Arrays.fill(isOnlyOuterBoundaryVertex,true);
		
		DataList faces = inputIFS.getFaceAttributes(Attribute.INDICES);
		for (int i=0; i<faces.size(); i++) {
			IntArray face = faces.item(i).toIntArray();
			for (int j=0; j<face.getLength(); j++) {
				int vertex1=face.getValueAt(j)+1;
				int vertex2=face.getValueAt((j+1) % face.getLength())+1;

				//save edge outgoing edge of vertex1
				List<Integer> adjacentVertices = edgesAtVertex.get(vertex1);
				if (adjacentVertices== null) 
					edgesAtVertex.set(vertex1,adjacentVertices=new LinkedList<Integer>());
				if (adjacentVertices.contains(-vertex2)) { //thats an inner edge, remove it
					adjacentVertices.remove((Integer) (-vertex2));
					if (adjacentVertices.isEmpty()) 
						edgesAtVertex.set(vertex1,null);
					isOnlyOuterBoundaryVertex[vertex1]=false;
				}
				else adjacentVertices.add(vertex2);
				
				//save edge as incomming edge of vertex2
				adjacentVertices = edgesAtVertex.get(vertex2);
				if (adjacentVertices== null) 
					edgesAtVertex.set(vertex2,adjacentVertices=new LinkedList<Integer>());
				if (adjacentVertices.contains(vertex1)) {//thats an inner edge, remove it
					adjacentVertices.remove((Integer) (vertex1));
					if (adjacentVertices.isEmpty()) 
						edgesAtVertex.set(vertex2,null);
					isOnlyOuterBoundaryVertex[vertex2]=false;
				}
				else adjacentVertices.add(-vertex1);
			}
		}
		
		Set<Integer> boundaryVertices=new HashSet<Integer>();
		for (int i=1; i<edgesAtVertex.size(); i++)
			if (edgesAtVertex.get(i)!=null)
				boundaryVertices.add(i);
		if (boundaryVertices.isEmpty()) return;
		
		Set<Integer> outerBoundaryVertices=new HashSet<Integer>();
		for (int i = 1; i < isOnlyOuterBoundaryVertex.length; i++) {
			if (isOnlyOuterBoundaryVertex[i] && boundaryVertices.contains(i)) 
				outerBoundaryVertices.add(i);
		}


		List<int[]> boundary =new LinkedList<int[]>() ;
		int vertex;
		do {
			vertex=outerBoundaryVertices.isEmpty()
					? boundaryVertices.iterator().next()
					: outerBoundaryVertices.iterator().next();
			boundaryVertices.remove((Integer) vertex);
			outerBoundaryVertices.remove((Integer) vertex);
				
			Set<Integer> vertices=new LinkedHashSet<Integer>();

			vertices.add(vertex);

			boolean closed=false;
			do {
				Integer newVertex=0;
				for (Iterator<Integer> iterator = edgesAtVertex.get(vertex).iterator(); iterator.hasNext();) {
					newVertex = iterator.next();
					if (newVertex>0) break;
				}
				if (newVertex<=0) break;
				if (!vertices.add(vertex=newVertex)) {closed=true; break;}
				if (!isOnlyOuterBoundaryVertex[vertex]) boundaryVertices.remove((Integer) vertex);

			}
			while (!isOnlyOuterBoundaryVertex[vertex]);

			int[] verticesArray=new int[closed?vertices.size()+1:vertices.size()];
			int i=0;
			for (Integer v : vertices)	verticesArray[i++]=v-1; //switch back to vertex indices starting at 0
			if (closed) verticesArray[verticesArray.length-1]=verticesArray[0];
			boundary.add(verticesArray);
		}
		while (boundaryVertices.iterator().hasNext());
		
		setBoundaryIndicesImpl(boundary);
	}

}
