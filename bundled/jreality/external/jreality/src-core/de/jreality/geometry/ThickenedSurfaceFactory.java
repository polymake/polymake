package de.jreality.geometry;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

import de.jreality.math.Pn;
import de.jreality.math.Rn;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.DataList;
import de.jreality.scene.data.DoubleArrayArray;
import de.jreality.scene.data.IntArray;
import de.jreality.scene.data.IntArrayArray;

/**
 * 
 * This factory class allows you to create a <i>thickened</i> surface from a given surface (given as an instance of {@link IndexedFaceSet}.
 * The result is a closed surface with a well-defined interior and exterior.
 * <p>
 * The first step is that a copy of the surface is created by moving the original vertices 
 * along the vertex normal direction until they are a distance <i>thickness</i> from the original vertices.
 * For convenience, call the original surface the bottom surface and this new surface the top surface.
 * <p>
 * The second step is to find all edges which only appear once in the combinatorics of the indexed face set.
 * These edges are assumed to be boundary edges; to each such edge a new quadrilateral is created which joins
 * the edge to its translated image under step one.  If face colors are provided for the original surface, then
 * the face color of these new boundary faces is inherited from the unique face to which the boundary edge belongs.
 * <p>
 * The third step is to (optionally) generate holes in the thickened surface, one for each face of the original surface.  
 * To be exact, a hole are introduced into each face of the bottom surface and its corresponding face in the top surface.
 * A tube is then constructed joining the two holes together. The tube is constructed as a product surface.
 *  Currently, any cross section of the tube parallel to the two faces 
 * is a scaled version of a special curve inscribed in this face; to be exact, it is 
 * a quadric curve tangent to the midpoints of the faces of the face. And the cross section
 * of the tube by a plane passing through the center of the hole and perpendicular to the original
 * face is determined by the <i>profile curve</i> parameter.
 * <p>
 * A typical invocation of an instance of this class looks like:
 * <code><b><pre>
		surface = SphereUtility.tessellatedIcosahedronSphere(1); 
		tsf = new ThickenedSurfaceFactory(surface);		// constructor requires a surface
		tsf.setThickness(.1);				// distance between top and bottom
		tsf.setMakeHoles(true);				// boolean
		tsf.setHoleFactor(.5);				// values smaller than one make the holes bigger
		tsf.setStepsPerEdge(6);				// each original edge is replaced by 6 segments
		// profile curve describes the cross-section of the hole
		tsf.setProfileCurve(new double[][]{{0,0}, {.333,.5},{.666, .5},{1,0}});
		tsf.update();
		IndexedFaceSet thickSurface = tsf.getThickenedSurface();

 * </pre></b></code>
 * <p>
 * Note: the case of a non-orientable surface represents special difficulties for the thicken factory.  Some remarks:
 * <ul>
 * <li>If a single vertex borders on two faces with opposite-facing normals, the factory will produce incorrect results, in which
 * the two sides of the thickened surface cross each other.
 * <li>If duplicate vertices are present on two face with opposite-facing normals, there is hope.  
 * Call the the method {@link #setMergeDuplicateBoundaryVerts(boolean)}
 * with the value <code>true</code>. The factory will then attempt to find such vertices and do surgery on the
 * thickened surface to avoid the crossing.
 * </ul>
 * <p>
 * See also <a href = "http://www.math.tu-berlin.de/~gunn/webstart/GeometryDemos/Thickener.jnlp">Thicken Factory webstart</a>.
 * <p>
 * @author Charles Gunn
 *
 */
 public class ThickenedSurfaceFactory {
	IndexedFaceSet theSurface;
	IndexedFaceSet thickSurface;
	double thickness;
	boolean makeHoles = false;
	boolean curvedEdges = false;
	boolean linearHole = false;
	boolean thickenAlongFaceNormals = false;
	boolean mergeDuplicateBoundaryVerts = false;
	double holeFactor = 1.0;
	double shiftAlongNormal = .5;
	int stepsPerEdge = 3;
	int metric = Pn.EUCLIDEAN;
	boolean keepFaceColors = true;
	double[][] profileCurve = {{0,0}, {.5,1}, {1,0}};
	boolean getGoodTextureCoordinates = true;
	private IndexedFaceSetFactory thickSurfaceIFSF;
	private HashMap<SharedEdge, Integer> sharedVertices;
	private int[][] newIndices, origIndices;
	private int[] faceOffsets;
	private List<Pair> edgelist;
	private List<SharedEdge> dupEdgeList;
	private int profileCurveSize;
	private double[][] origVertices;
	private double tolerance = 10E-4;
	private double[][] allVertices;
	
	public ThickenedSurfaceFactory(IndexedFaceSet ifs)	{
		theSurface = ifs;
	}

	public IndexedFaceSet getSurface() {
		return theSurface;
	}

	public void setSurface(IndexedFaceSet theSurface) {
		this.theSurface = theSurface;
	}

	public boolean isMakeHoles() {
		return makeHoles;
	}

	/**
	 * Determines whether holes are generated in the thickened surface or not.  Default: false;
	 * @param makeHoles
	 */
	public void setMakeHoles(boolean makeHoles) {
		this.makeHoles = makeHoles;
	}

	public double[][] getProfileCurve() {
		return profileCurve;
	}

	/**
	 * This set of number pairs controls the cross-section of the hole when I cut 
	 * it with a plane passing through the center of the hole parallel to the normal
	 * direction of the original surface.  to be precise, they are weights used with a 
	 * bilinear interpolation scheme.
	 * <p>
	 * The first coordinate of each pair controls the mixture of the top and bottom surfaces
	 * and the second coordinate controls the mixture of the center point and the peripheral point
	 * (point on the original edge).
	 * <p>
	 * For example, the triple <code>{{0,0},{.5,1},{1,0}}</code> results in a profile curve that
	 * begins on the bottom surface (x = 0) at the original edge (y=0), moves to the middle between top and bottom
	 * (x = .5) at the center of the hole (y=1), and ends at the top surface (x = 0) at the original edge
	 * there (y = 0).			examples.put("borromean ring",surface);

	 * @param profileCurve
	 */public void setProfileCurve(double[][] profileCurve) {
		this.profileCurve = profileCurve;
	}

	public int getStepsPerEdge() {
		return stepsPerEdge;
	}

	/**
	 * The refinement of the geometry created to represent a hole is controlled by this parameter. Default: 3
	 * @param stepsPerEdge
	 */public void setStepsPerEdge(int stepsPerEdge) {
		this.stepsPerEdge = stepsPerEdge;
	}

	public double getThickness() {
		return thickness;
	}

	/**
	 * This parameter controls how far apart the top and bottom surfaces are.
	 * @param thickness
	 */
	public void setThickness(double thickness) {
		this.thickness = thickness;
	}
	
	public double getHoleFactor() {
		return holeFactor;
	}

	/**
	 * This parameter is an additional parameter controlling the size of the hole.  It basically
	 * is used as a scale factor on the y-coordinate of the profile curve; larger values make the hole smaller,
	 * while smaller ones make the whole bigger.  Negative values are discouraged.  Default: 1.0
	 * @param holeFactor
	 */public void setHoleFactor(double holeFactor) {
		this.holeFactor = holeFactor;
	}

	public boolean isKeepFaceColors() {
		return keepFaceColors;
	}

	public int getMetric() {
		return metric;
	}

	/**
	 * For working in non-euclidean geometries, set this.  Default: {@link Pn#EUCLIDEAN}.
	 * Warning: currently ignored.
	 * @param metric
	 */
	public void setMetric(int metric) {
		this.metric = metric;
	}

	/**
	 * Set this to true to retain the original face colors of the input surface in the thickened surface.
	 * @param keepFaceColors
	 */
	public void setKeepFaceColors(boolean keepFaceColors) {
		this.keepFaceColors = keepFaceColors;
		if (keepFaceColors && theSurface.getFaceAttributes(Attribute.COLORS) == null) {
			//throw new IllegalStateException("No face colors to keep");
			this.keepFaceColors = false;
		}
	}

	public boolean isCurvedEdges() {
		return curvedEdges;
	}

	public void setCurvedEdges(boolean curvedEdges) {
		this.curvedEdges = curvedEdges;
	}

	public boolean isLinearHole() {
		return linearHole;
	}

	public void setLinearHole(boolean linearHole) {
		this.linearHole = linearHole;
	}

	public double getShiftAlongNormal() {
		return shiftAlongNormal;
	}

	public void setShiftAlongNormal(double shiftAlongNormal) {
		this.shiftAlongNormal = shiftAlongNormal;
	}

	public boolean isThickenAlongFaceNormals() {
		return thickenAlongFaceNormals;
	}

	public void setThickenAlongFaceNormals(boolean thickedAlongFaceNormals) {
		this.thickenAlongFaceNormals = thickedAlongFaceNormals;
	}

	/**
	 * This returns the thickened surface.  This remains the same for the life of the factory.
	 * @return
	 */
	public IndexedFaceSet getThickenedSurface()	{
		if (thickSurfaceIFSF == null) thickSurfaceIFSF = new IndexedFaceSetFactory();
		return thickSurface;
	}
	
//	protected IndexedFaceSet thicken(IndexedFaceSet result, IndexedFaceSet ifs, double thickness, boolean holes, double holeSize, int stepsPerEdge, double[][] profile)	{
	/**
	 * This has to be called after each set of edits to the state of the factory, in order to update the
	 * result.
	 *
	 */
	public void update()	{
		if (thickSurfaceIFSF == null) thickSurfaceIFSF = new IndexedFaceSetFactory();
		origVertices = theSurface.getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray(null);
		int originalVertexCount = origVertices.length;
		int fiberlength = origVertices[0].length;
		double[][] oldVN = null;
		if (theSurface.getVertexAttributes(Attribute.NORMALS) == null)	
			oldVN = IndexedFaceSetUtility.calculateVertexNormals(theSurface);
		else 
			oldVN = theSurface.getVertexAttributes(Attribute.NORMALS).toDoubleArrayArray(null);			

		if (oldVN[0].length == 3)	{
			oldVN = Pn.homogenize(null, oldVN);
		}
		if (oldVN[0].length == 4) 
			for (int i = 0; i<oldVN.length; ++i) {
				oldVN[i][3] = 0.0;
			}
		double[][] newVN = doubleIt(oldVN);

		origIndices = theSurface.getFaceAttributes(Attribute.INDICES).toIntArrayArray(null);
		processBoundary();
		double[][] newVertices = new double[originalVertexCount*2][4];

		for (int i = 0; i<originalVertexCount; ++i)	{
			System.arraycopy(origVertices[i], 0, newVertices[i], 0, fiberlength);
			if (fiberlength == 3) newVertices[i][3] = 1.0;
			// estimate adjustment factor to attain perpendicular thickness
			//double factor = Math.abs(Math.cos(Pn.angleBetween(oldVN[i], oldFN[anyFace[i]], metric)));
			//System.err.println("cos of angle is "+factor);
			// TODO make this correct for noneuclidean case too
			//Pn.dragTowards(newV[i+n], oldV[i], oldN[i], thickness, Pn.EUCLIDEAN);
			double[] tmp = Rn.linearCombination(null, 1.0, newVertices[i], shiftAlongNormal*thickness, oldVN[i]);
			Rn.linearCombination(newVertices[i+originalVertexCount], 1.0, newVertices[i], (shiftAlongNormal-1.0)*thickness, oldVN[i]);
			System.arraycopy(tmp, 0, newVertices[i], 0, tmp.length);
//			System.err.println("Normal = "+Rn.toString(oldVN[i]));
		}
		
		int numFaces = origIndices.length;
		int numBoundaryEdges = edgelist.size();
		newIndices = new int[numFaces*2+numBoundaryEdges][];
		// the new face list consists of the top faces (m), bottome faces (m), 
		// and boundary faces joining top and bottom (nm)
		for (int i = 0; i<numFaces; ++i)		{
			newIndices[i] = origIndices[i];
			int k = origIndices[i].length;
			newIndices[i+numFaces] = new int[k];
			for (int j = 0; j<k; ++j)	{
				if (makeHoles) newIndices[i+numFaces][j] = origIndices[i][j]+originalVertexCount;
				else newIndices[i+numFaces][j] = origIndices[i][k-1-j]+originalVertexCount; // for correct normal orientation
			}
		}
		for (int i = 0; i<numBoundaryEdges; ++i)	{
			newIndices[2*numFaces+i] = new int[4];
			Pair p = edgelist.get(i);
			int h = p.h, l = p.l;
			newIndices[2*numFaces+i][0] = h;
			newIndices[2*numFaces+i][1] = l;
			newIndices[2*numFaces+i][2] = l+originalVertexCount;
			newIndices[2*numFaces+i][3] = h+originalVertexCount;				
		}

		// merge the boundary edges identified as duplicated
		List<Integer> alreadyDone = new ArrayList<Integer>();
		for (SharedEdge se : dupEdgeList) {
			Pair p1 = se.p1, p2 = se.p2;
			System.err.println("handling duplicate edge");
			int fsize1 = origIndices[p1.face].length;
			int diff = originalVertexCount; //(profileCurveSize - 1) * (fsize1 * stepsPerEdge + 1);
			int ul1 = origIndices[p1.face][p1.edge],
				ur1 = origIndices[p1.face][(p1.edge+1)%fsize1],
				ll1 = ul1+diff,
				lr1 = ur1+diff;
			int fsize2 = origIndices[p2.face].length;
			int ul2 = origIndices[p2.face][p2.edge],
				ur2 = origIndices[p2.face][(p2.edge+1)%fsize2],
				ll2 = ul2+diff,
				lr2 = ur2+diff;

			if (se.flipped)	{
				int tmp = ul2;
				ul2 = ur2;
				ur2 = tmp;
				tmp = ll2;
				ll2 = lr2;
				lr2 = tmp;
			}
			boolean normalflip = false;
			double d1 = Rn.euclideanDistance(newVertices[ul1],newVertices[ul2]),
				d2 = Rn.euclideanDistance(newVertices[ul1], newVertices[ll2]);
			if (d2 < d1) normalflip = true;
			if (normalflip)	{
				int tmp = ul2;
				ul2 = ll2;
				ll2 = tmp;
				tmp = ur2;
				ur2 = lr2;
				lr2 = tmp;
			}
//			System.err.println("leftright flip = "+se.flipped+"\tup-down flip = "+normalflip);
			mergeVertices(newVertices, ul1, ul2);
			mergeVertices(newVertices, ur1, ur2);
			mergeVertices(newVertices, ll1, ll2);
			mergeVertices(newVertices, lr1, lr2);
		}
		if (!makeHoles)	{
			//thickSurfaceIFSF = new IndexedFaceSetFactory();
			thickSurfaceIFSF.setVertexCount(2*originalVertexCount);
			thickSurfaceIFSF.setFaceCount(2*numFaces+numBoundaryEdges);
			thickSurfaceIFSF.setVertexCoordinates(newVertices);
			if (theSurface.getVertexAttributes(Attribute.COLORS) != null)	{
				thickSurfaceIFSF.setVertexColors( doubleIt(theSurface.getVertexAttributes(Attribute.COLORS).toDoubleArray(null)));
			}
			if (theSurface.getVertexAttributes(Attribute.NORMALS) != null)	{
				thickSurfaceIFSF.setGenerateVertexNormals(false);
				thickSurfaceIFSF.setVertexNormals(newVN );
			}
			if (theSurface.getVertexAttributes(Attribute.TEXTURE_COORDINATES) != null)	{
				thickSurfaceIFSF.setVertexTextureCoordinates( doubleIt(theSurface.getVertexAttributes(Attribute.TEXTURE_COORDINATES).toDoubleArray(null)));
			}
			if (theSurface.getFaceAttributes(Attribute.COLORS) != null)	{
				thickSurfaceIFSF.setFaceColors( doubleIt(theSurface.getFaceAttributes(Attribute.COLORS).toDoubleArrayArray(null), edgelist));
			}
			if (theSurface.getFaceAttributes(Attribute.NORMALS) != null)	{
				thickSurfaceIFSF.setGenerateFaceNormals(true);
			}
			if (theSurface.getEdgeAttributes(Attribute.INDICES) != null)	{
				thickSurfaceIFSF.setGenerateEdgesFromFaces(true);
			}
			thickSurfaceIFSF.setFaceIndices(newIndices);
			thickSurfaceIFSF.update();
			thickSurface = thickSurfaceIFSF.getIndexedFaceSet();
			return;
//			return thickSurfaceIFSF.getIndexedFaceSet();		
		}
		profileCurveSize = profileCurve.length;
		int totalEdges = 0, allVerts = 0; //2*originalVertexCount;
		for (int i = 0; i<numFaces; ++i)	{
			totalEdges += origIndices[i].length;
			allVerts += (origIndices[i].length*stepsPerEdge+1)*profileCurveSize;
		}
//		System.err.println("Found "+totalEdges+" edges");
		int allFaces = totalEdges * stepsPerEdge * (profileCurveSize-1) + stepsPerEdge*numBoundaryEdges;
//		System.err.println("Vert, face # = "+allVerts+" "+allFaces);
		allVertices = new double[allVerts][4];
		double[][] allTexCoords = new double[allVerts][2];
		int[][] allIndices = new int[allFaces][4];
		DoubleArrayArray oldFaceColors = null;
		double[][] newFaceColors = null;
		double[][] faceColorsWithoutHoles = null;
		if (keepFaceColors && theSurface.getFaceAttributes(Attribute.COLORS) != null)	{
			oldFaceColors = theSurface.getFaceAttributes(Attribute.COLORS).toDoubleArrayArray();
			newFaceColors = new double[allFaces][];
			faceColorsWithoutHoles = doubleIt(theSurface.getFaceAttributes(Attribute.COLORS).toDoubleArrayArray(null), edgelist);
		}
		// copy over original vertices 
//		for (int i = 0; i<2*originalVertexCount; ++i)	{
//			allVertices[i] = newV[i];
//		}
//		for (int i = 0; i<numBoundaryEdges; ++i)	{
//			allIndices[i] = newIndices[2*numFaces+i];
//			if (keepFaceColors) newFaceColors[i] = faceColorsWithoutHoles[2*numFaces+i];
//		}
		// now go through the original faces and generate "holes"
		int allVertCount = 0;
		int allFaceCount = 0;
		faceOffsets = new int[numFaces];
		for (int i = 0; i<numFaces; ++i)	{
			faceOffsets[i] = allVertCount;
			int[] bottomface = newIndices[i];
			int[] topface = newIndices[i+numFaces];
			double[] cb = centroid(bottomface, newVertices);
			double[] ct = centroid(topface, newVertices);
			int fsize = bottomface.length;
			int vertexCount = 0;
			int totalVerticesPerLoop = stepsPerEdge * fsize+1;
			int totalVertsThisFace = profileCurveSize * totalVerticesPerLoop;
			double[][] borderBottom = linearHole(bottomface, newVertices, newVN, stepsPerEdge, curvedEdges);
			double[][] borderTop = linearHole(topface, newVertices, newVN, stepsPerEdge, curvedEdges);
			double[][] tangentQuadricBottom = null;
			double[][] tangentQuadricTop = null;
			if (linearHole)	{
				tangentQuadricBottom = borderBottom;
				tangentQuadricTop = borderTop;				
			} else {
				if (curvedEdges)	{
					double[][] controlpoints = linearHole(bottomface, newVertices, newVN, 2*stepsPerEdge, curvedEdges);
					tangentQuadricBottom = quadraticCurvedHole(bottomface, stepsPerEdge, controlpoints);
					controlpoints = linearHole(topface, newVertices, newVN, 2*stepsPerEdge, curvedEdges);
					tangentQuadricTop = quadraticCurvedHole(topface, stepsPerEdge, controlpoints);				
				} else { 
					tangentQuadricBottom = linearHole ? borderBottom : quadraticHole(bottomface, newVertices, stepsPerEdge);
					tangentQuadricTop = linearHole ? borderTop : quadraticHole(topface, newVertices, stepsPerEdge);								
				}
			}
			int[] nonDuplicateVertexIndicesForThisHole = new int[totalVertsThisFace];
			// for each element of the profile curve ...
			for (int k = 0; k<profileCurveSize; ++k)	{
				double tu = k/(profileCurveSize-1.0);
				if (tu > 1) tu = 1;
				double u = profileCurve[k][0];
				double v = profileCurve[k][1] * holeFactor;
				// for each point of the "hole curve"
				for (int j = 0; j<totalVerticesPerLoop; ++j)	{
					double tv = j/(totalVerticesPerLoop-1.0);
					if (tv > 1) tv = 1.0;
					double[] vb = null, vt = null;
					if (k == 0 || k == (profileCurveSize-1))	{
						vb = borderBottom[j];
						vt = borderTop[j];
					}  else {
						vb = tangentQuadricBottom[j]; //Rn.linearCombination(null, 1-t, vb1, t,vb2);
						vt = tangentQuadricTop[j]; //Rn.linearCombination(null, 1-t, vt1, t,vt2);
					}
//					else {
						Rn.bilinearInterpolation(allVertices[allVertCount+vertexCount], u, v, vb, vt, cb, ct);
						allTexCoords[allVertCount+vertexCount][0] = tu;
						allTexCoords[allVertCount+vertexCount][1] = tv;
						nonDuplicateVertexIndicesForThisHole[vertexCount] = allVertCount+vertexCount;
//					}
					vertexCount++;
					}				
			}
			// build the faces using the non-duplicate indices found above
			double[] oldFaceColor = null;
			if (keepFaceColors && oldFaceColors != null)	
				oldFaceColor = oldFaceColors.item(i).toDoubleArray(null);
			if (oldFaceColor == null) oldFaceColor = new double[]{1,1,1,1};
			int[] ndvi = nonDuplicateVertexIndicesForThisHole;
			for (int k = 0; k<profileCurveSize-1; ++k)	{
				for (int j = 0; j<totalVerticesPerLoop-1; ++j)	{
						int[] thisF = allIndices[allFaceCount];
						thisF[0] = ndvi[k*totalVerticesPerLoop + j];
						thisF[1] = ndvi[k*totalVerticesPerLoop + (j+1)%totalVerticesPerLoop];
						thisF[2] = ndvi[(k+1)*totalVerticesPerLoop + (j+1)%totalVerticesPerLoop];
						thisF[3] = ndvi[(k+1)*totalVerticesPerLoop + j];
						if (keepFaceColors && newFaceColors != null) newFaceColors[allFaceCount] = oldFaceColor;
						allFaceCount++;
				}
			}
			allVertCount += totalVertsThisFace;
		}
		// put in the faces on the boundary now
		for (int i = 0; i<edgelist.size(); ++i)	{
			Pair p = edgelist.get(i);
			int offset = faceOffsets[p.face]+stepsPerEdge*p.edge;
			int fsize = origIndices[p.face].length;
			int diff = (profileCurveSize - 1) * (fsize * stepsPerEdge + 1);
			for (int i1 = 0; i1<stepsPerEdge; ++i1)	{
				int[] thisF = allIndices[allFaceCount];
				thisF[0] = offset+diff+i1;
				thisF[1] = offset+diff+1+i1;
				thisF[2] = offset+1+i1;
				thisF[3] = offset+i1;
				if (keepFaceColors && newFaceColors != null) newFaceColors[allFaceCount] = new double[]{1,1,1,1};;
				allFaceCount++;
			}
		}

//		System.err.println("Found "+foundSharedVerts+" shared vertices");
//		if (newFaceColors != null) System.err.println("new face colors = "+Rn.toString(newFaceColors));
		thickSurfaceIFSF.setVertexCount(allVerts);
		thickSurfaceIFSF.setFaceCount(allFaces);
		thickSurfaceIFSF.setVertexCoordinates(allVertices);
		thickSurfaceIFSF.setVertexTextureCoordinates(allTexCoords);
		thickSurfaceIFSF.setFaceIndices(allIndices);
		if (keepFaceColors && newFaceColors != null) {
//			thickSurfaceIFSF.setFaceColors( (double[][]) null);
			thickSurfaceIFSF.setFaceColors(newFaceColors);
		}
		thickSurfaceIFSF.setGenerateEdgesFromFaces(true);
		thickSurfaceIFSF.setGenerateFaceNormals(true);
		thickSurfaceIFSF.setGenerateVertexNormals(true);
		thickSurfaceIFSF.update();
		thickSurface = thickSurfaceIFSF.getIndexedFaceSet();	
//		return result;
		
	}

	private void mergeVertices(double[][] newVertices, int ul1, int ul2) {
		double[] v1 = newVertices[ul1], v2 = newVertices[ul2];
//				System.err.println("redefining vertices "+Rn.toString(v1));
//				System.err.println("redefining vertices "+Rn.toString(v2));
		double[] vv = Rn.times(null, .5, Rn.add(null, v1, v2));
//				System.err.println("new vertex "+Rn.toString(vv));
		newVertices[ul1] = newVertices[ul2] = vv;
	}

	private void setVertex(int ii, double[] vv) {
//		System.err.println("setting index "+ii);
		allVertices[ii] = vv;
	}

	private double[][] linearHole(int[] face, double[][] vv, double[][] nv, int stepsPerEdge, boolean curvedEdges) {
		double[][] values = new double[face.length * stepsPerEdge+1][vv[0].length];
		int fsize = face.length;
		for (int j = 0; j<fsize; ++j)	{
			double[] vb1 = vv[face[j]];
			double[] vb2 = vv[face[(j+1)%fsize]];
			if (!curvedEdges) {
				for (int jj = 0; jj<stepsPerEdge; ++jj)	{
					double t = jj/(stepsPerEdge*1.0);
					Rn.linearCombination(values[j*stepsPerEdge+jj], 1-t, vb1, t,vb2);
				}
			} else {
				if (vb1.length == 3)	{
					vb1 = Pn.homogenize(vb1, vb1);
					vb2 = Pn.homogenize(vb2, vb2);
				}
				double[] dv = Rn.subtract(null, vb2, vb1);
				double length = Pn.norm(dv, metric);
				double[] t1 = Pn.projectOntoComplement(null, nv[face[j]], dv, metric);
				Pn.setToLength(t1, t1, length/3, metric);
				dv = Rn.subtract(null, vb1, vb2);
				double[] t2 = Pn.projectOntoComplement(null, nv[face[(j+1)%fsize]], dv, metric);
				Pn.setToLength(t2, t2, length/3, metric);
				double[] m1 = Rn.add(null, vb1, t1);
				double[] m2 = Rn.add(null, vb2, t2);
				for (int jj = 0; jj<stepsPerEdge; ++jj)	{
					double t = jj/(stepsPerEdge*1.0);
					Rn.bezierCombination(values[j*stepsPerEdge+jj], t, vb1, m1, m2, vb2);
				}				
			}
		}
		System.arraycopy(values[0], 0, values[values.length-1], 0, values[0].length);
		//values[0] = values[values.length-1];
		return values;
	}

	private static double[][] quadraticHole(int[] face, double[][] vv, int stepsPerEdge) {
		int fsize = face.length;
		double[][] controlpoints = new double[2*fsize][vv[0].length];
		for (int j = 0; j<fsize; ++j)	{
			controlpoints[2*j] = vv[face[j]];
			Rn.linearCombination(controlpoints[2*j+1], .5, vv[face[j]], .5, vv[face[((j+1)%fsize)]]);
		}
		return quadraticHole(face, stepsPerEdge, controlpoints );
	}
	
	private static double[][] quadraticHole(int[] face, int stepsPerEdge, double[][] controlpoints) {
		double[][] values = new double[face.length * stepsPerEdge+1][controlpoints[0].length];
		int fsize = face.length;
		for (int j = 0; j<fsize; ++j)	{
			int i0 = (2*j+1)%(2*fsize);
			int i1 = (2*j+2)%(2*fsize);
			int i2 = (2*j+3)%(2*fsize);
			boolean even = (stepsPerEdge % 2) == 0;
			for (int jj = 0; jj<stepsPerEdge; ++jj)	{
				double t = jj/(stepsPerEdge*1.0);
				if (!even) t += (.5)/stepsPerEdge;
				double a0 = (1-t)*(1-t);
				double a1 = 2*(1-t)*t;
				double a2 = t*t;
				// don't ask!
				int index = (j*stepsPerEdge + (stepsPerEdge + 1)/2 +  jj+values.length-1) % (values.length-1);
				for (int k = 0; k<controlpoints[0].length; ++k)	{
					values[index][k] = a0*controlpoints[i0][k];
					values[index][k] += a1*controlpoints[i1][k];
					values[index][k] += a2*controlpoints[i2][k];
				}
			}
		}
		System.arraycopy(values[0], 0, values[values.length-1], 0, values[0].length);
		//values[0] = values[values.length-1];
		return values;
	}
	private static double[][] quadraticCurvedHole(int[] face, int stepsPerEdge, double[][] lch) {
		double[][] values = new double[face.length * stepsPerEdge+1][lch[0].length];
		int lchl = lch.length;
		int fsize = face.length;
		for (int j = 0; j<fsize; ++j)	{
			int i0 = (2*stepsPerEdge*j+stepsPerEdge)%(lchl);
			int i1 = (2*stepsPerEdge*(j+1))%(lchl);
//			int i2 = (2*j+3)%(2*fsize);
//			boolean even = (stepsPerEdge % 2) == 0;
			for (int jj = 0; jj<stepsPerEdge; ++jj)	{
				double t = jj/(stepsPerEdge*1.0);
				double[] p1 = Rn.linearCombination(null, 1-t, lch[(i0+jj)%lchl], t, lch[(i1+jj)%lchl]);
//				double a0 = (1-t)*(1-t);
//				double a1 = 2*(1-t)*t;
//				double a2 = t*t;
//				// don't ask!
				int index = (j*stepsPerEdge + (stepsPerEdge + 1)/2 +  jj+values.length-1) % (values.length-1);
//				System.err.println("Index is "+index);
				values[index] = p1;
//				for (int k = 0; k<controlpoints[0].length; ++k)	{
//					values[index][k] = a0*controlpoints[i0][k];
//					values[index][k] += a1*controlpoints[i1][k];
//					values[index][k] += a2*controlpoints[i2][k];
//				}
			}
		}
		System.arraycopy(values[0], 0, values[values.length-1], 0, values[0].length);
//		//values[0] = values[values.length-1];
		return values;
	}

	private static double[] centroid(int[] face, double[][] v)	{
		double[] center = new double[v[0].length];
		for (int i = 0; i<face.length; ++i)	{
			Rn.add(center, v[face[i]], center);
		}
		Rn.times(center, 1.0/face.length, center);
		return center;
	}
	private static double[] doubleIt(double[] data)	{
		double[] newD = new double[data.length*2];
		System.arraycopy(data, 0, newD, 0, data.length);
		System.arraycopy(data, 0, newD, data.length, data.length);
		return newD;
	}

	private static double[][] doubleIt(double[][] data)	{
		double[][] newD = new double[data.length*2][data[0].length];
		System.arraycopy(data, 0, newD, 0, data.length);
		System.arraycopy(data, 0, newD, data.length, data.length);
		return newD;
	}
	private static double[][] doubleIt(double[][] data,  List<Pair> edges)	{
		double[][] newD = new double[data.length*2 + edges.size()][];
		for (int i = 0; i < data.length; ++i)	{
			newD[i] = newD[i+data.length] = data[i];
		}
		int n = 2*data.length; 
		for (int i = 0; i<edges.size(); ++i)	{
			Pair p = edges.get(i);
			newD[n+i] = data[p.face];
		}	
		return newD;
	}

	private static final class SharedEdge {
		Pair p1, p2;
		boolean flipped;
		SharedEdge(Pair p1, Pair p2, boolean f)	{
			this.p1 = p1;
			this.p2 = p2;
			flipped = f;
		}
	}

	private static final class Pair {
		  final int l, h, face, edge;
		  boolean flipped = false;
		  Pair(int a, int b, int c, int d) {
			  if (a<=b) flipped = true;
			  l = a;
			  h = b;
//		    if(a<=b) { l=a; h=b; }
//		    else     { h=a; l=b; }
		    face = c;
		    edge = d;
		  }
		  public boolean equals(Object obj) {
		    if(this==obj) return true;
		    try {
		      final Pair p=(Pair)obj;
		      boolean sameSense = (l == p.l && h == p.h);
		      boolean flippedSense = (l==p.h && h == p.l);
//		      if (sameSense || flippedSense)	{
//		    	  if (sameSense) System.err.println("same = "+sameSense);
//		    	  return true;
//		      } else return false;
		      return flippedSense || sameSense;
		    } catch(ClassCastException ex) {
		      return false;
		    }
		  }
		  public int hashCode() {
		    return (l<<16)^h;
		  }
		}

	public void processBoundary() {
		DataList faceIndices = theSurface.getFaceAttributes(Attribute.INDICES);
		if (theSurface.getFaceAttributes(Attribute.attributeForName("faceIndices")) != null) {
			faceIndices = theSurface.getFaceAttributes(Attribute.attributeForName("faceIndices"));
		}
		DataList faceNormals = theSurface.getFaceAttributes(Attribute.NORMALS);
		double[][] fn = null;
		if (faceNormals != null) fn = faceNormals.toDoubleArrayArray(null);
		
		IntArrayArray faces = faceIndices.toIntArrayArray(); 
	    edgelist =new ArrayList<Pair>();
	   
	    for (int i= 0; i < faces.getLength(); i++)
	    {
	        IntArray f= faces.getValueAt(i);
	        for (int j= 0; j < f.getLength(); j++)
	        {
	        	int i1 = f.getValueAt(j), j1 = f.getValueAt((j + 1)%f.getLength());
	        	Pair p = new Pair(i1, j1, i, j);
	            if (edgelist.contains(p)) edgelist.remove(p);
	            else edgelist.add(p);
	        }
	    }
	    // now go through and check for duplicate edges (geometrically duplicate)
	    dupEdgeList = new ArrayList<SharedEdge>();
	    if (mergeDuplicateBoundaryVerts)	{
		    ArrayList<Pair> toRemove = new ArrayList<Pair>();
	    	int n = edgelist.size();
	    	for (int i = 0; i<n ; ++ i)	{
	    		Pair ip = edgelist.get(i);
	    		int length = origIndices[ip.face].length;
	    		int i0 = origIndices[ip.face][ip.edge],
	    			i1 = origIndices[ip.face][(ip.edge+1)%length];
    			double[] v0i = origVertices[i0],
    				v1i = origVertices[i1];
	    		for (int j = i+1; j<n; ++j)	{
	    			Pair jp = edgelist.get(j);
		    		length = origIndices[jp.face].length;
		    		int j0 = origIndices[jp.face][jp.edge],
		    			j1 = origIndices[jp.face][(jp.edge+1)%length];
	    			double[] v0j = origVertices[j0],
	    				v1j = origVertices[j1];
	    			double d00 = Rn.euclideanDistance(v0i, v0j),
	    				d11 = Rn.euclideanDistance(v1i, v1j),
	    				d01 = Rn.euclideanDistance(v0i, v1j),
	    				d10 = Rn.euclideanDistance(v1i, v0j);
	    			SharedEdge se = null;
	    			if (d00 < tolerance && d11 < tolerance )	{
	    				se = new SharedEdge(ip, jp, false);
	    			}
	    			if (d01 < tolerance && d10 < tolerance )	{
	    				se = new SharedEdge(ip, jp, true);
	    			}
	    			if (se != null) {
	    				dupEdgeList.add(se);
	    				toRemove.add(ip);
	    				toRemove.add(jp);
	    			}
	    		}
	    	}
	    	System.err.println("Found duplicate edges: "+dupEdgeList.size());
	    	edgelist.removeAll(toRemove);
	    }
	}

//	private void markDuplicateBorderVertices()	{
//		// omit following code since it's not compatible with assigning texture coordinates to vertices
//		// look for shared vertices along edges
//		int vertexIndex = -1;
//		SharedVertex sv = null;
//		if (tv == 0 && (tu==0 || tu==1)) {
//			int onCorner = j % stepsPerEdge;
//			if (onCorner == 0)	{
//				if (u == 0) vertexIndex = bottomface[j/stepsPerEdge];
//				else if (u == 1) vertexIndex = topface[j/stepsPerEdge];
//			} else {
//				int v0 = 0, v1 = 0;
//				if (u == 0)	{		// possible shared vertex on bottom 
//					v0 = bottomface[ j / stepsPerEdge];
//					v1 = bottomface[ ((j/stepsPerEdge)+1) % bottomface.length];
//				}
//				else if (u == 1)	{		// possible shared vertex on bottom 
//					v0 = topface[ j / stepsPerEdge];
//					v1 = topface[ ((j/stepsPerEdge)+1) % topface.length];
//				}
//				sv = new SharedVertex(v0, v1, j % stepsPerEdge, stepsPerEdge);
//				if (sharedVertices.get(sv) != null)
//					vertexIndex = sharedVertices.get(sv).intValue();
//				//System.err.println("Getting vertex "+v0+":"+v1+":"+j%stepsPerEdge);						
//			}
//		}
//		if (vertexIndex != -1) {
//			nonDuplicateVertexIndicesForThisHole[vertexCount] = vertexIndex;
//			if (sv != null) System.err.println("Found vertex "+sv.v0+":"+sv.v1+":"+sv.step);
//			foundSharedVerts++;
//		}
//		if (sv != null) {
//			sharedVertices.put(sv, allVertCount+vertexCount);
//			System.err.println("Putting vertex "+sv.v0+":"+sv.v1+":"+sv.step);
//		}
//		
//	}

//	if (weights != null && weights.length == 3) {
//	double[][] smoothedpoints = new double[totalVertsThisFace][fiberlength];
////	 smooth off the curves
//	for (int jj = 0; jj < p; ++jj)	{
//	    for (int j = 0; j<fsize*stepsPerEdge; ++j)	{
//			if (jj == 0 || jj == (p-1))	
//				smoothedpoints[((j+1)*p + jj)%totalVertsThisFace] = allVertices[allVertCount+(((j+1)*p+jj)%totalVertsThisFace)];
//			else
//			    Rn.add(smoothedpoints[((j+1)*p + jj)%totalVertsThisFace], 
//					Rn.linearCombination(null, weights[0], allVertices[allVertCount+j*p+jj], 
//							weights[1], allVertices[allVertCount+(((j+1)*p+jj)%totalVertsThisFace)]),
//					Rn.times(null, weights[2], allVertices[allVertCount+(((j+2)*p+jj)%totalVertsThisFace)]));
//		}
//	}	public boolean isKeepFaceColors() {

	public boolean isMergeDuplicateBoundaryVerts() {
		return mergeDuplicateBoundaryVerts;
	}

	public void setMergeDuplicateBoundaryVerts(boolean b) {
		this.mergeDuplicateBoundaryVerts = b;
	}


//	for (int ii = 0; ii<totalVertsThisFace; ++ii)	
//		allVertices[allVertCount + ii] = smoothedpoints[ii];
//	}


}

