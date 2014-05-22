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

/** 
 * @author Bernd Gonska
 */
package de.jreality.geometry;

import java.util.LinkedList;
import java.util.List;
import java.util.Set;

import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.IndexedLineSet;
import de.jreality.scene.PointSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.DataList;
import de.jreality.scene.data.DataListSet;
import de.jreality.scene.data.DoubleArray;
import de.jreality.scene.data.DoubleArrayArray;
import de.jreality.scene.data.IntArray;
import de.jreality.scene.data.IntArrayArray;
import de.jreality.scene.data.StringArray;
import de.jreality.scene.data.StringArrayArray;

/** A class for removing duplicate information in {@link PointSet} and its sub-classes.
 * <p>
 * The idea is to find data elements which are close in value to each other and
 * merge them into a single instance, then change the indexing of the edges and faces
 * to reflect these changes.
 * 
 * @author gonska
 *
 */
public class RemoveDuplicateInfo {


	/** retains only vertices which differs enough in the coordinstes. 
	 * <i>enough</i> means the distance in the three directions is smaler than <code>eps</code> 
	 * retains the following Vertex Attribute Datatypes in the reduced way:
	 *  String-,Double- and Int-, -Array and -ArrayArray  
	 * face- and edge- attributes stay the same.
	 * only Face and Edge Indices changes.
	 * Remark:
	 *  In some rare cases(many near Vertices) Vertices 
	 *   within eps distance do not collapse to one.
	 *  and in some cases Vertices with euclidean
	 *   distance up to <i>5.2*eps</i> could be merged. 
	 * Remark: The GeometryAttribute 
	 * 			<code>quadmesh</code> will be deleted
	 * Remark: some other Attributes may collide with the new Geometry
	 * 
	 * @param ps       can be <code>IndexedFaceSet,IndexedLineSet or PointSet</code>
	 * @param atts	   this Attributes must be DoubleArray or DoubleArrayArray Attributes 
	 * 					(others will be ignored) they will be respected by testing equality of Vertices.
	 * @return  returns IndexedFaceSet,  IndexedLineSet or PointSet depending on input.
	 */
////	---------- new start-----------------
	private int[] referenceTable;
	private int[] mergeReferenceTable;
	private int[] removeReferenceTable;
	private int[] sublistTable;


	private PointSet source;
	private PointSet geo;
	private double[][] points; // the vertices
	private double[][] attrVals; // the vertices

	private double eps; // Tolereanz for merging
	private int dim;// =points[x].length
	private int maxPointPerBoxCount=50;
	private int numSubBoxes;
	private int numNewVerts;
	// constr ----------------------------------------
	private RemoveDuplicateInfo(PointSet ps, Attribute ...attributes ){
		source=ps;
		points=source.getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray(null);
	}
	// methods----------------------------------------
	/**  merges vertices which appears more than once in the geometry
	 *  the result of type PointSet is especialy the same typ as the given geometry 
	 */
	public static PointSet removeDuplicateVertices(PointSet ps, Attribute ...attributes ) {
		return removeDuplicateVertices(ps,10E-8,attributes);		
	}
	/**  merges vertices which appears more than once in the geometry
	 *  the result of type PointSet is especialy the same typ as the given geometry 
	 */
	public static PointSet removeDuplicateVertices(PointSet ps, double eps, Attribute ...attributes ) {
		// inittialize some data
		RemoveDuplicateInfo r= new RemoveDuplicateInfo(ps);
		if (ps instanceof IndexedFaceSet) 
			r.geo=new IndexedFaceSet();
		else if (ps instanceof IndexedLineSet) 
			r.geo=new IndexedLineSet();
		else r.geo=new PointSet();
		{
			// TODO: make better output
			if(r.points.length==0) return null;
			if(r.points.length==1) return null;
		}
		r.eps=Math.max(0, eps);
		r.dim=r.points[0].length;
		r.mergeReferenceTable=new int[r.points.length];
		for (int i = 0; i < r.points.length; i++) {
			r.mergeReferenceTable[i]=i;
		}
		r.numSubBoxes=(int)Math.pow(2, r.dim);
		r.readOutAttributes(attributes);
		// create first box:
		Box b=r.fillFirstBox();
		// start sortBox(firstBox):
		r.processBox(b);
		// return:
		r.postCalulation();
		return r.geo;
	} 
	/** read out the attributes, which are given by the user, 
	 * to be compared for equality.
	 * @param attributes
	 */
	public final static Attribute[] defaultAttrs = {Attribute.COORDINATES};
	private void readOutAttributes(Attribute ... attributes ){
		List<double[][]> DAAList= new LinkedList<double[][]>();
		List<double[]> DAList= new LinkedList<double[]>();
		int dim= 0;
		if (attributes == null) attributes = defaultAttrs;
		// sort and remember meaningfull Atributes
		for(Attribute at: attributes){
			if(at.getName().equals(Attribute.COORDINATES.getName()))continue;
			DataList currDL=source.getVertexAttributes(at);
			if(currDL== null)continue;
			if(currDL instanceof DoubleArrayArray){
				if(currDL.item(0)== null)continue;
				DAAList.add(currDL.toDoubleArrayArray(null));
				dim+=currDL.item(0).size();
			}
			if(currDL instanceof DoubleArray){
				DAList.add(currDL.toDoubleArray(null));
				dim++;
			}
		}
		System.err.println("dim = "+dim);
		// put the doubles into an array
		attrVals= new double[source.getNumPoints()][dim];
		int dimCount=0;
		for (double[][] daa: DAAList) {
			int currDim=daa[0].length; 
			for (int i = 0; i < daa.length; i++) 
				System.arraycopy(daa[i], 0, attrVals[i], dimCount, currDim);
			dimCount+=currDim;
		}
		for (double[] da: DAList) {
			for (int i = 0; i < da.length; i++)
				attrVals[i][dimCount]=da[i];
			dimCount++;
		}
	}

	/** inserts all points in the first Box	 */
	private Box fillFirstBox(){
		double[] max= new double[dim];
		double[] min= new double[dim];
		for (int i = 0; i < dim; i++) 
			min[i]=max[i]=points[0][i];
		for (int i = 1; i < points.length; i++) {
			for (int j = 0; j < dim; j++) {
				double[] p=points[i];
				if(p[j]>=max[j]) max[j]=p[j];
				if(p[j]<=min[j]) min[j]=p[j];
			}	
		}
		Box b=new Box(max,min,dim);
		for (int i = 0; i < points.length; i++) {
			b.addPointIfPossible(i);
		}
		return b;
	}


	/** fills the references 
	 * of the Vertices in the Box
	 * in the referenceTable
	 */	
	private void processBox(Box b){
		if(b.numOfPoints<=1) return;
		// case of to small Box:
		if(b.getSize()<=(3.0*eps)) {
			compareInBox(b);
			return;
		}
		// recursion case(subdivision needed):
		if(b.numOfPoints>maxPointPerBoxCount){
			Box[] subBoxes = createSubBoxes(b);
			for (int i = 0; i < subBoxes.length; i++) {
				processBox(subBoxes[i]);
			}
			return;
		}
		// comparing:
		compareInBox(b);
	}
	/** indicates if a Point is within the box given by 
	 *  min and max plus an eps. */
	private boolean inBetween(double[]max,double[]min,double eps, double[] val){
		for (int i = 0; i < val.length; i++) {
			if(val[i]>max[i]+eps) return false;
			if(val[i]<min[i]-eps) return false;
		}
		return true;
	}
	/** compares the points in every important attribute.
	 *  uses the same <code>eps</code> for every attribute.  */
	private boolean isEqualByEps(int p1, int p2){
		double[] c1=points[p1];// coords
		double[] c2=points[p2];
		double[] a1=attrVals[p1];//important double attributes (inlined)
		double[] a2=attrVals[p2];
		return (inBetween(c1,c1, eps, c2) && inBetween(a1,a1, eps, a2)); 
	}
	/** sets the references of all Vertices in the box	
	 */ 
	private void compareInBox(Box b) {
		for (int p1: b.innerPoints){
			if(!isLegalPoint(p1)) continue;
			for (int p2: b.innerPoints){
				if(p1>=p2)continue;
				if(!isLegalPoint(p2)) continue;
				if (isEqualByEps(p1, p2))
					mergeReferenceTable[p2]=p1;
			}
		}
	}
	/** indicates if a point is not referenced to an other; */
	private boolean isLegalPoint(int p){
		return (mergeReferenceTable[p]==p);
	}

	private Box[] createSubBoxes(Box b) { 
		Box[] result= new Box[numSubBoxes];
		for (int i = 0; i < result.length; i++) {
			// calc max & min:
			double[] min= new double[dim];
			double[] max= new double[dim];
			int k=i;
			for (int d = 0; d < dim; d++) {
				if(b.realMax[d]-b.realMin[d]<=2*eps){
					max[d]=min[d]=(b.realMax[d]+b.realMin[d])/2;
				}
				if(k%2==0){
					max[d]=(b.realMin[d]+b.realMax[d])/2;
					min[d]=b.realMin[d]+eps;
				}
				else{
					min[d]=(b.realMin[d]+b.realMax[d])/2;
					max[d]=b.realMax[d]-eps;
				}
				k = k>>1;
			}
			// make new subBox
			result[i]=new Box(max,min,dim);			
		}
		// insert points
		for(int v: b.innerPoints)
			if(isLegalPoint(v))
				for (int i = 0; i < numSubBoxes; i++) 
					// bei allen probieren 
					result[i].addPointIfPossible(v);
		return result;
	}
	// DataStructures----------------------------------------
	/** holds a bucket full of points.
	 *  this points will be directly compared (O(n^2)) 
	 */
	private class Box{
		int numOfPoints=0; 
		double[] originalMax; // without eps
		double[] originalMin; // without eps
		double[] realMax;
		double[] realMin;
		boolean empty;
		List<Integer> innerPoints= new LinkedList<Integer>();
		/** box with boundarys,
		 *  can be filled with (double[dim]) Points 
		 */
		private Box(double[]max,double[]min,int dim) {
			originalMax=max;
			originalMin=min;
			realMax=new double[dim];
			realMin=new double[dim];
			empty=true;
		}
		/** adds a Point to the box if possible 
		 *  updates the real bounding
		 *  returns succes
		 */
		private boolean addPointIfPossible(int point){
			double[] p=points[point];
			if(!inBetween(originalMax, originalMin, eps, p))return false;
			if (empty){
				for (int i = 0; i < dim; i++) {
					realMax[i]=realMin[i]=p[i];
				}
				empty=false;
			}
			else{
				for (int i = 0; i < dim; i++) {
					if(p[i]>=realMax[i]) realMax[i]=p[i];
					if(p[i]<=realMin[i]) realMin[i]=p[i];
				}
			}
			innerPoints.add(point);
			numOfPoints++;
			return true;
		}
		private double getSize(){
			double size=0;
			for (int i = 0; i < dim; i++) 
				if(realMax[i]-realMin[i]>size)
					size=realMax[i]-realMin[i];
			return size;
		}
	}

//	post calculation -------------------------------------------------------

	private void postCalulation(){
		newTables();
		geo.setNumPoints(numNewVerts);
		if (geo instanceof IndexedFaceSet) {
			IndexedFaceSet ifs = (IndexedFaceSet) geo;
			ifs.setNumFaces(((IndexedFaceSet)source).getNumFaces());
		}
		if (geo instanceof IndexedLineSet) {
			IndexedLineSet ils = (IndexedLineSet) geo;
			ils.setNumEdges(((IndexedLineSet)source).getNumEdges());
		}
		newDatalists();
		newIndices();
	} 
	/** calculates referenceTable 
	 * new Vertices 
	 * (unused Vertices will be taken out) 
	 */
	private void newTables(){
		// remove Table:
		removeReferenceTable= new int[points.length];
		int numUsedVerts=0;
		int pos=0;
		for (int i = 0; i < points.length; i++)
			if (mergeReferenceTable[i]==i){
				removeReferenceTable[i]=numUsedVerts;
				numUsedVerts++;
			}
			else{
				removeReferenceTable[i]=-1;
			}
		// direct referenceTable:
		referenceTable= new int[points.length];
		for (int i = 0; i < points.length; i++) {
			referenceTable[i]=removeReferenceTable[mergeReferenceTable[i]];
		}
		numNewVerts=numUsedVerts;
		// sublist Table:
		sublistTable= new int[numUsedVerts];
		pos=0;
		for (int i = 0; i < points.length; i++) 
			if(removeReferenceTable[i]!=-1){
				sublistTable[pos]=i;
				pos++;
			}
	}

	/** 
	 * @param oldReferences (for ecx.: face indices)
	 * @param referenceTable (result of start)
	 * @return new references (for ecx.: new face indices)
	 */
	private void newIndices(){
		// face Indices
		if (geo instanceof IndexedFaceSet) {
			IndexedFaceSet ifs = (IndexedFaceSet) geo;
			IndexedFaceSet sou = (IndexedFaceSet) source;
			DataList data=sou.getFaceAttributes(Attribute.INDICES);
			if(data!=null && data.size()>0 ){
				int[][] fIndis=data.toIntArrayArray(null);
				int[][] result= new int[fIndis.length][];
				for (int i = 0; i < result.length; i++) {
					result[i]=newIndices(fIndis[i], referenceTable);
				}
				ifs.setFaceAttributes(Attribute.INDICES,new IntArrayArray.Array(result));
			}	
		}
		// edge Indices
		if (geo instanceof IndexedLineSet) {
			IndexedLineSet ils = (IndexedLineSet) geo;
			IndexedLineSet sou = (IndexedLineSet) source;
			DataList data=sou.getEdgeAttributes(Attribute.INDICES);
			if(data!=null && data.size()>0 ){
				int[][] eIndis=data.toIntArrayArray(null);
				int[][] result= new int[eIndis.length][];
				for (int i = 0; i < result.length; i++) {
					result[i]=newIndices(eIndis[i], referenceTable);
				}
				ils.setEdgeAttributes(Attribute.INDICES,new IntArrayArray.Array(result));
			}	
		}
	}
	private static int[] newIndices(int[] oldReferences, int[] referenceTable){
		int[] result= new int[oldReferences.length];
		for (int i = 0; i < oldReferences.length; i++) {
			result[i]=referenceTable[oldReferences[i]];
		}
		return result;
	} 
	private void newDatalists() {
		DataListSet datas=source.getVertexAttributes();
		Set<Attribute> atts=(Set<Attribute>) datas.storedAttributes();
		// VertexDatalists
		for(Attribute at : atts){
			DataList dl=datas.getList(at);
			if (dl instanceof DoubleArrayArray) {DoubleArrayArray dd = (DoubleArrayArray) dl;
			geo.setVertexAttributes(at, RemoveDuplicateInfo.getSublist(dd, sublistTable));
			}
			if (dl instanceof DoubleArray) {DoubleArray dd = (DoubleArray) dl;
			geo.setVertexAttributes(at, RemoveDuplicateInfo.getSublist(dd, sublistTable));
			}
			if (dl instanceof IntArrayArray) {IntArrayArray dd = (IntArrayArray) dl;
			geo.setVertexAttributes(at, RemoveDuplicateInfo.getSublist(dd, sublistTable));
			}
			if (dl instanceof IntArray) {IntArray dd = (IntArray) dl;
			geo.setVertexAttributes(at, RemoveDuplicateInfo.getSublist(dd, sublistTable));
			}
			if (dl instanceof StringArrayArray) {
				StringArrayArray dd = (StringArrayArray) dl;
				geo.setVertexAttributes(at, RemoveDuplicateInfo.getSublist(dd, sublistTable));
			}
			if (dl instanceof StringArray) {StringArray dd = (StringArray) dl;
			geo.setVertexAttributes(at, RemoveDuplicateInfo.getSublist(dd, sublistTable));
			}
		}
		if (geo instanceof IndexedFaceSet) {
			IndexedFaceSet ifs = (IndexedFaceSet) geo;
			ifs.setFaceAttributes(((IndexedFaceSet)source).getFaceAttributes());	
		}
		if (geo instanceof IndexedLineSet) {
			IndexedLineSet ils = (IndexedLineSet) geo;
			ils.setEdgeAttributes(((IndexedLineSet)source).getEdgeAttributes());
		}
		geo.setGeometryAttributes(source.getGeometryAttributes());
		geo.setGeometryAttributes("quadMesh",null);
	}
	// getter / setter ---------------------------------------- 
	/** get Tolerance for equality */
	public double getEps() {
		return eps;
	}
	/** set Tolerance for equality*/
	public void setEps(double eps) {
		this.eps = eps;
	}
	public int[] getReferenceTable() {
		return referenceTable;
	}
	/** removes vertices which are not used by faces.
	 * changes faceIndices.
	 * @param vertices
	 * @param faces
	 * @return vertices
	 */
	public static double[][] removeNoFaceVertices(double[][] vertices, int[][] faces){
		int numVOld=vertices.length;
		int numF=faces.length;
		boolean[] usedVertices= new boolean[numVOld];
		for (int i = 0; i < numVOld; i++) 
			usedVertices[i]=false;
		// remember all vertices used in faces
		for (int i = 0; i < numF; i++) 
			for (int j = 0; j < faces[i].length; j++) 
				usedVertices[faces[i][j]]=true;	
		int count=0; 
		int[] referenceTabel= new int[numVOld];
		for (int i = 0; i < numVOld; i++) {
			if(usedVertices[i]){
				referenceTabel[i]=count;
				vertices[count]=vertices[i];// vertices gleich richtig einschreiben
				count++;
			}
			else{
				referenceTabel[i]=-1;
			}
		}
		// faces umindizieren
		for (int i = 0; i < numF; i++) 
			for (int j = 0; j < faces[i].length; j++) 
				faces[i][j]=referenceTabel[faces[i][j]];
		// VertexListe erneuern
		double[][] newVertices= new double[count][];
		System.arraycopy(vertices, 0, newVertices, 0, count);
		return newVertices;
	}
	/** a face definition can repeat the first index at the end  
	 * excample: {1,2,3,4,1} or {1,2,3,4}
	 * in first case: the last index will be removed
	 */
	public static void removeCycleDefinition(int[][] faces){
		for (int i = 0; i < faces.length; i++) {
			int len=faces[i].length;
			if(len>1)
				if(faces[i][len-1]==faces[i][0]){
					int[] newIndis= new int[len-1];
					System.arraycopy(faces[i], 0, newIndis, 0, len-1);
					faces[i]=newIndis;
				}
		}
	}
	// ------------------ sublists -----------------------------
	private static DataList getSublist(DoubleArrayArray dd, int[] referenceTable){
		if(dd.getLength()==0)return dd;
		return getSublist(dd.toDoubleArrayArray(null), referenceTable);
	} 
	private static DataList getSublist(double[][] dd, int[] referenceTable){
		if (dd.length==0)return new DoubleArrayArray.Array(new double[][]{{}});
		int dim=dd[0].length;
		double[][] newList=new double[referenceTable.length][dim];
		for (int i = 0; i < newList.length; i++) 
			for (int j = 0; j < dim; j++) 
				newList[i][j]=dd[referenceTable[i]][j];
		return new DoubleArrayArray.Array(newList);
	} 
	private static DataList getSublist(DoubleArray d, int[] referenceTable){
		if(d.getLength()==0)return d;
		return getSublist(d.toDoubleArray(null), referenceTable);
	} 
	private static DataList getSublist(double[] d, int[] referenceTable){
		if (d.length==0)return new DoubleArray(new double[]{});
		double[] newList=new double[referenceTable.length];
		for (int i = 0; i < newList.length; i++) 
			newList[i]=d[referenceTable[i]];
		return new DoubleArray(newList);
	} 
	private static DataList getSublist(IntArrayArray dd, int[] referenceTable){
		if(dd.getLength()==0)return dd;
		return getSublist(dd.toIntArrayArray(null), referenceTable);
	} 
	private static DataList getSublist(int[][] dd, int[] referenceTable){
		if (dd.length==0)return new IntArrayArray.Array(new int[][]{{}});
		int dim=dd[0].length;
		int[][] newList=new int[referenceTable.length][dim];
		for (int i = 0; i < newList.length; i++) 
			for (int j = 0; j < dim; j++) 
				newList[i][j]=dd[referenceTable[i]][j];
		return new IntArrayArray.Array(newList);
	} 
	private static DataList getSublist(IntArray d, int[] referenceTable){
		if(d.getLength()==0)return d;
		return getSublist(d.toIntArray(null), referenceTable);
	} 
	private static DataList getSublist(int[] d, int[] referenceTable){
		if (d.length==0)return new IntArray(new int[]{});
		int[] newList=new int[referenceTable.length];
		for (int i = 0; i < newList.length; i++) 
			newList[i]=d[referenceTable[i]];
		return new IntArray(newList);
	} 
	private static DataList getSublist(StringArrayArray dd, int[] referenceTable){
		if(dd.getLength()==0)return dd;
		return getSublist(dd.toStringArrayArray(null), referenceTable);
	} 
	private static DataList getSublist(String[][] dd, int[] referenceTable){
		if (dd.length==0)return new StringArrayArray.Array(new String[][]{{}});
		int dim=dd[0].length;
		String[][] newList=new String[referenceTable.length][dim];
		for (int i = 0; i < newList.length; i++) 
			for (int j = 0; j < dim; j++) 
				newList[i][j]=dd[referenceTable[i]][j];
		return new StringArrayArray.Array(newList);
	} 
	private static DataList getSublist(StringArray d, int[] referenceTable){
		if(d.getLength()==0)return d;
		return getSublist(d.toStringArray(null), referenceTable);
	} 
	private static DataList getSublist(String[] d, int[] referenceTable){
		if (d.length==0)return new StringArray(new String[]{});
		String[] newList=new String[referenceTable.length];
		for (int i = 0; i < newList.length; i++) 
			newList[i]=d[referenceTable[i]];
		return new StringArray(newList);
	}

	/** simplifys the SceneGraphComponent tree recursivly
	 * 
	 *  Nodes with cameras, geometrys and lights will not be changed
	 *  Nodes with transformations and  Appearances will only be deleted if no
	 *   geometry, camera or light will be effected.
	 *  Nodes which have only one Child(which has a Geometry, camera or Light in subtree)
	 *   and no geometry, Camera or light 
	 *   will be deleted if it has 
	 *    no Appearence or Transformation
	 *    or if the Tranformation / Appearance can be shifted into the child node 
	 */
	public static void simplifySceneTree(SceneGraphComponent g){
		// author bernd
		simplifyLeafs(null,g);
		simplifyBridges(null,g);
	}
	private static void simplifyBridges(SceneGraphComponent parent,SceneGraphComponent g){
		// author bernd
		//TODO: delete bridgeNodes if they have Appearance or Transformation
		// and merge these into childNode Trafo / App ??
		if(g==null) return;
		// check and simplify Children first
		List<SceneGraphComponent> children=getComponents(g);
		for (SceneGraphComponent c: children)		simplifyBridges(g,c);
		// is removeable?
		if(parent==null) return;
		boolean possible=true;
		boolean shiftApp=false;
		boolean shiftTrafo=false;
		if(isBridgeComponent(g)){
			// collect infos:
			if(g.getAppearance()!=null)
				if(g.getChildComponent(0).getAppearance()!=null) possible=false;
				else shiftApp=true;
			if(g.getTransformation()!=null)
				if(g.getChildComponent(0).getTransformation()!=null) possible=false;
				else shiftTrafo=true;
			// remove:
			if (possible) {
				SceneGraphComponent child=g.getChildComponent(0);//ist ja nur eine!
				parent.removeChild(g);
				parent.addChild(child);
				if (shiftApp) child.setAppearance(g.getAppearance());
				if (shiftTrafo) child.setTransformation(g.getTransformation());
			}
		}	
	}
	private static void simplifyLeafs(SceneGraphComponent parent,SceneGraphComponent g){
		// author bernd
		if(g==null) return;
		// check and simplify Children first
		List<SceneGraphComponent> children=getComponents(g);
		for (SceneGraphComponent c: children)		simplifyLeafs(g,c);
		// is removeable?
		if(parent==null) return;
		if(isEmptyComponent(g))		parent.removeChild(g);
	}	
	private static List<SceneGraphComponent> getComponents(SceneGraphComponent g){
		// author bernd
		List<SceneGraphComponent> list= new LinkedList<SceneGraphComponent>();
		for (Object o:g.getChildNodes()){
			if(o instanceof SceneGraphComponent)
				list.add((SceneGraphComponent)o);
		}
		SceneGraphComponent[] result=new  SceneGraphComponent[list.size()];
		int i=0;
		for (SceneGraphComponent c:list) {
			result[i]=c;
			i++;
		}
		return list;
	}
	private static boolean isEmptyComponent(SceneGraphComponent g){
		// author bernd
		if(	g.getGeometry()!=null 
				||g.getChildComponentCount()>0
				||g.getCamera()!=null
				||g.getLight()!=null) return false;
		if(g.getTools()!=null && g.getTools().size()>0) return false;
		return true;	
	}
	private static boolean isBridgeComponent(SceneGraphComponent g){
		// author bernd
		if(	g.getGeometry()!=null
				||g.getChildComponentCount()!=1
				||g.getCamera()!=null
				||g.getLight()!=null) return false;
		if(g.getTools()!=null && g.getTools().size()>0) return false;
		return true;	
	}
	// TODO dont use this method, use "instanceof" instead
	private static IndexedFaceSet pointSetToIndexedFaceSet(PointSet p){
		if (p instanceof IndexedFaceSet)
			return (IndexedFaceSet) p;
		if (p instanceof IndexedLineSet)
			return indexedLineSetToIndexedFaceSet((IndexedLineSet)p);
		IndexedFaceSet f= new IndexedFaceSet(p.getNumPoints(),0);
		f.setGeometryAttributes(p.getGeometryAttributes());
		f.setVertexAttributes(p.getVertexAttributes());
		return f;
	}
	private static IndexedFaceSet indexedLineSetToIndexedFaceSet(IndexedLineSet l){
		if (l instanceof IndexedFaceSet)
			return (IndexedFaceSet) l;
		IndexedFaceSet f= new IndexedFaceSet(l.getNumPoints(),0);
		f.setGeometryAttributes(l.getGeometryAttributes());
		f.setVertexAttributes(l.getVertexAttributes());
		f.setEdgeCountAndAttributes(l.getEdgeAttributes());
		return f;
	}

}
