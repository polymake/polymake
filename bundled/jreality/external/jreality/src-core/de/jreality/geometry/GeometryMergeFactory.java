package de.jreality.geometry;
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


import java.awt.Color;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;

import de.jreality.math.Matrix;
import de.jreality.math.P3;
import de.jreality.math.Rn;
import de.jreality.scene.Appearance;
import de.jreality.scene.Geometry;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.IndexedLineSet;
import de.jreality.scene.PointSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.DataList;
import de.jreality.scene.data.DataListSet;
import de.jreality.scene.data.DoubleArrayArray;
import de.jreality.scene.data.IntArrayArray;
import de.jreality.scene.data.StorageModel;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.DefaultGeometryShader;
import de.jreality.shader.DefaultLineShader;
import de.jreality.shader.DefaultPointShader;
import de.jreality.shader.DefaultPolygonShader;
import de.jreality.shader.EffectiveAppearance;
import de.jreality.shader.ShaderUtility;
import de.jreality.util.CopyVisitor;

/** this Factory merges IndexedFaceSets, Indexed Line Sets, PointSets
 * or scenegraphs containing such things to a single geometry.
 * <p>
 * <ul>its default behavior:
 * 	<li> only <code>doubleArrayArray</code> Attributes are supported 
 *      other attributes will be ignored with
 *      two exceptions : FaceIndices and EdgeIndices </li>
 *  <li> an attribute which appears in every 'Set will be merged</li>
 *  <li> the merged geometry has allways color and coordinates dimension 4
 *       Problems may appear if the input dimension is not 3 or 4!;
 *       and problems may appear if the default color dimension is not 4</li>  
 *  <li> an attribute which do not appear in every 'Set wil be ignored</li>
 * </ul>
 * <p>
 * <ul> special settings   
 *  <li> with <code>respectFaces respectEdges</code> 
 *       you can control to have no faceAttributes or edgeAttributes in the 
 *       merged Geometry.
 *       </li>
 *  <li> with <code>generateFaceNormals generateVertexNormals</code>
 *       you can fill the undefined holes of the Normal-Attributes </li>
 *  <li> with <code>defaultFaceAttributes defaultFaceAttributeValues </code>
 *  	you can enrich partialy(not totaly) undefined Attributes with 
 *      default(per Attribute). This defaults can then be given per geometry
 *      (as list) or vor all geomertys(a list with a single entry).
 *      this defaults do not overwride allready defined Attributes,
 *      and wil not be used if no geometry has this attribute.
 *       </li>
 *  <li> with <code>importantFaceDefaultAttributes</code> you can later garante
 *  	that a defaultAttribute also is used if no geometry has this Attribute
 *  	(not yet implemented)  </li>
 *  <li>edge and vertex defaults analog</li>
 *  <li>transformations in the scenegraph will not be ignored and change VertexCoordinates, 
 *      EdgeNormals and FaceNormals</li>
 *  <li>diffuseColors of appearances in the scenegraph will not be igored
 *      and will be set in faceColors,EdgeColors and VertexColors</li>
 * </ul>
 * 
 * For an example of this factory, see  
 * <a href=http://www3.math.tu-berlin.de/jreality/mediawiki/index.php/Use_a_geometry_merge_factory> this tutorial</a>.
 *
 * TODO Problems:
 * 	ifs and ils together
 * <p>
 * TODO: merge only IndexedFaceSets of a SceneGraph
 * <p>
 * TODO: merge FaceSets and linesets seperate to avoid "holes" in the AttributeList 
 *       and gather in the end 
 *  <p>
 *  TODO: refactor to conform to the jReality factory pattern (use update() to regenerate current state).
 *  
 * @author gonska    
 * @see RemoveDuplicateInfo if you wish also to combine duplicated vertex, edge, or face data.
 */


public class GeometryMergeFactory {
	private boolean respectFaces=true;// 
	private boolean respectEdges=true;//	
	private boolean respectVertices=true;// more dificult 'cause of coordinates 

	private boolean respectFacesIntern=true;// 
	private boolean respectEdgesIntern=true;//	
	private boolean respectVerticesIntern=true;// more dificult 'cause of coordinates 

	private boolean generateFaceNormals=true;// 
	private boolean generateVertexNormals=true;// 
	
	private boolean hasFaceColors=true;// 
	private boolean hasVertexColors=true;// 
	private boolean hasEdgeColors=true;// 
	

	// attribute-values which are set if the named 
	// attribute is not supported in a geometry. 
	// will be ignored if no geometry supports them.
	private List<Attribute> defaultFaceAttributes=new LinkedList<Attribute>();//	
	private List<List<double[]>>defaultFaceAttributeValues=new LinkedList<List<double[]>>();// 
	private List<Attribute> defaultEdgeAttributes=new LinkedList<Attribute>();//	
	private List<List<double[]>>defaultEdgeAttributeValues=new LinkedList<List<double[]>>();// 
	private List<Attribute> defaultVertexAttributes=new LinkedList<Attribute>();//	
	private List<List<double[]>>defaultVertexAttributeValues=new LinkedList<List<double[]>>();// 
	private final int FACE_ATTR=2,EDGE_ATTR=1,VERT_ATTR=0;
    // defaults which have to be set even if no geometry supports them.
	// must allready be listed above.
	private List<Attribute> importantFaceDefaultAttributes= new LinkedList<Attribute>();
	private List<Attribute> importantEdgeDefaultAttributes= new LinkedList<Attribute>();
	private List<Attribute> importantVertexDefaultAttributes= new LinkedList<Attribute>();

	public GeometryMergeFactory() {
		importantFaceDefaultAttributes.add(Attribute.COLORS);
		importantEdgeDefaultAttributes.add(Attribute.COLORS);
		importantVertexDefaultAttributes.add(Attribute.COLORS);
	}
	
	private static List<Attribute> collectAttributes(List<List<Attribute>> atLists, List<Attribute> defAtt, List<Attribute> impAtt){
		//	keep in mind unnesscesary defaults 
		List<Attribute> badDefaults= new LinkedList<Attribute>();
		if(defAtt!=null)
			for(Attribute defa :defAtt){
				boolean hit=false;
				for(List<Attribute> list : atLists)
					if (list.contains(defa)) hit=true;
				if(!hit) badDefaults.add(defa);
			}
		// only remain everywhere defined attributes 
		List<Attribute> goodAttr= new LinkedList<Attribute>();
		List<Attribute> firstList=new LinkedList<Attribute>();
		if(atLists.size()>0)firstList=atLists.get(0);
		
		for(Attribute firstA : firstList){
			boolean hitAll=true;// everywhere defined attrib. ?
			if(atLists!=null)
				for(List<Attribute> list : atLists){	 
					boolean hit=false; // in this list defined attr. ?
					if(list!=null){
						if(list.contains(firstA))
							hit=true;
					}
					else hitAll=false;
					if(!hit) {
						hitAll=false;
						break;
					}
				}
			else hitAll=false;
			if(hitAll) goodAttr.add((Attribute)firstA);
		}
		// gather:
		if (defAtt!= null)
			for(Attribute dAt : defAtt){
				boolean isIn=false;
				if(goodAttr.contains(dAt)){
					isIn=true;
					break;
				}
				if (!isIn)	goodAttr.add(dAt);
			}
		// eliminate unnescecary attributes
		for (Attribute bad : badDefaults)
			goodAttr.remove(bad);
		// add important attributes
		for (Attribute imp : impAtt )
			if(!goodAttr.contains(imp))
				if(defAtt.contains(imp))
					goodAttr.add(imp);
		return  goodAttr;
	} 
	/**
	 * @param result The Pointset filled with attributes corresponding to <i>typ</i>
	 * @param defaultAttributes  Attributes which have default values
	 * @param defaultAttributeValues the default values to the Attributes
	 * @param dls  a list of all AttributeSets of the corresponding <i>typ</i>
	 * @param typ  0/1/2 works for Vertex/Edge/FaceAttributes
	 */
	private void mergeDoubleArrayArrayAttributes(PointSet result,
			List<Attribute> defaultAttributes,List<List<double[]>>defaultAttributeValues,
			List<Attribute> impAtts,
			DataListSet[] dls ,int typ) {
		// attr liste erstellen :
		// aber nur Attributenehmen 
		List<List<Attribute>> Atts= new LinkedList<List<Attribute>>();
		for (int i = 0; i < dls.length; i++) {
			if(dls[i]!=null && dls[i].getListLength()>0){
				Object[] o= dls[i].storedAttributes().toArray();
				LinkedList<Attribute> list=new LinkedList<Attribute>();
				if(o!=null)
					for ( Object oo : o)
						list.add((Attribute)oo);
				Atts.add(list);
			}
		}
//		 liste bis auf wesentliche (!=null && len!=0 )
		// kuerzen:		
		List<Attribute> Attr= collectAttributes(Atts, defaultAttributes, impAtts);
		for ( Attribute at : Attr){
			try {
				int k=-1;
				if( defaultAttributes!=null)
					k=defaultAttributes.indexOf(at);
				DataList dataList;
				// default supportet:
				if(k>=0)  dataList= new DoubleArrayArray.Array(mergeDoubleArrayArrayAttribute(dls, at, defaultAttributeValues.get(k)));
				// no default supportet:
				else     dataList= new DoubleArrayArray.Array(mergeDoubleArrayArrayAttribute(dls, at, null));
				if (dataList!=null){
					// create Datalist
					if(typ==VERT_ATTR){
						if(result.getVertexAttributes().getListLength()==0){
							result.setVertexCountAndAttributes(at,dataList);
						}
						else result.setVertexAttributes( at, dataList );
					}
					if(typ==EDGE_ATTR)
						((IndexedLineSet)result).setEdgeAttributes( at, dataList );
					if(typ==FACE_ATTR)
						((IndexedFaceSet)result).setFaceAttributes( at, dataList );
				}
			} catch (Exception e) {}				
		}
	}

	/**
	 * Merges the double attributes into a single trivial type array.
	 * If a single entry of the array fails to have the prescribed attribute(and no default is given) 
	 * null is returned
	 * @param Datalists
	 * @param the attribute which is to merge
	 * @param the list of defaults
	 * 			size==ifs.size : possible default is given for every single ifs    
	 * 			size==1 : all ifs have same defaultValue  
	 * 			null: no default is supported 
	 * @return array containing all data of given attribute  
	 */
	private static double [][] mergeDoubleArrayArrayAttribute( 
			DataListSet [] list , Attribute attr, List<double[]> defaults) {
		// total len of list (all elements)
		int totalLen=0; 
		for (int i = 0; i < list.length; i++)
			totalLen+=list[i].getListLength();	
		// result
		double [][] result = new double[totalLen][];

		int n=0;// current position 
		for( int i=0; i < list.length; i++ ) {
			if(list[i].getListLength()>0){
				double[][] values; // part of result
				// if we have to less Data  
				if (list[i].getList( attr )==null && (defaults==null||defaults.size()==0)){
					return null; 
				}
				// if default values are nescesarry
				DataList l=list[i].getList( attr );
				if (l==null){
					values= new double[list[i].getListLength()][];
					double[] d;
					if(defaults.size()>i){
						d=defaults.get(i);
					}
					else d= defaults.get(0);
					for (int j = 0; j < values.length; j++){
						values[j]=d;
					}
				}
				else{
					// if datas are given
					if (!(l instanceof DoubleArrayArray)){
						return null; 
					}
					values = l.toDoubleArrayArray(null);
				}
				System.arraycopy(values, 0, result, n, values.length ); 
				n += list[i].getListLength();
			}
		}
		return result;  
	}
	/** if some entrys of dls have len=0 or are null they are not respected
	 * @param ifs array of indexed face sets.
	 * @param attr a face attribute, e.g., @link de.jreality.scene.data.Attribute.NORMALS
	 * @return array containing all data of face attribute of an array of indexed face set.
	 */

	private static int [][] mergeIntArrayArrayAttribute( DataListSet [] dls , Attribute attr) {
		//  total len of list (all elements)
		int totalLen=0;
		for (int i=0;i<dls.length;i++){
			if(dls[i]!=null)
				totalLen+=dls[i].getListLength();
		}
		// result
		int [][] result = new int[totalLen][];
		int n=0;// current position 
		for( int i=0; i < dls.length; i++ ) {
			if (dls[i]!=null && dls[i].getList( attr )!=null){
				int[][] values = dls[i].getList( attr ).toIntArrayArray(null);
				System.arraycopy(values, 0, result, n, values.length );
				n += dls[i].getListLength();
			}
		}
		return result;
	}

//	-----------------new Merge--------------------
	private static DefaultGeometryShader dgs;
	private static DefaultPolygonShader dps;
	private static DefaultLineShader dls;
	private static DefaultPointShader dvs;

	private void assignTransformation(IndexedFaceSet ifs, double[] matrix){
		double[] flipit = P3.makeStretchMatrix(null, new double[] {-1,0, -1,0, -1.0});
		if (ifs.getVertexAttributes(Attribute.COORDINATES) == null) return;
		double[][] v = ifs.getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray(null);
		double[][] nv = Rn.matrixTimesVector(null, matrix, v);
		ifs.setVertexAttributes(Attribute.COORDINATES, StorageModel.DOUBLE_ARRAY.array(nv[0].length).createWritableDataList(nv));
		double[] mat = Rn.transpose(null, matrix);          	
		mat[12] = mat[13] = mat[14] = 0.0;
		Rn.inverse(mat, mat);
		if (respectFacesIntern){
			if(ifs.getNumFaces()>0){
				if (ifs.getFaceAttributes(Attribute.NORMALS) != null)	{
					v = ifs.getFaceAttributes(Attribute.NORMALS).toDoubleArrayArray(null);
					nv = Rn.matrixTimesVector(null, mat, v);
					if (Rn.determinant(matrix) < 0.0)
						nv = Rn.matrixTimesVector(null, flipit, nv);
					ifs.setFaceAttributes(Attribute.NORMALS, StorageModel.DOUBLE_ARRAY.array(nv[0].length).createWritableDataList(nv));
				} 
				else{
					if(generateFaceNormals)
						IndexedFaceSetUtility.calculateAndSetFaceNormals(ifs);
				}
			}
		}
		if(respectVerticesIntern){
			if(ifs.getNumPoints()>0){
				if (ifs.getVertexAttributes(Attribute.NORMALS) != null)	{
					v = ifs.getVertexAttributes(Attribute.NORMALS).toDoubleArrayArray(null);
					nv = Rn.matrixTimesVector(null, mat, v);
					if (Rn.determinant(matrix) < 0.0)	
						nv = Rn.matrixTimesVector(null, flipit, nv);
					ifs.setVertexAttributes(Attribute.NORMALS, StorageModel.DOUBLE_ARRAY.array(nv[0].length).createWritableDataList(nv));
				} 
				else{
					if(generateVertexNormals){
						IndexedFaceSetUtility.calculateAndSetVertexNormals(ifs);
						if (!defaultVertexAttributes.contains(Attribute.NORMALS)){
							defaultVertexAttributes.add(Attribute.NORMALS);
							// avoid definition-holes by generating vertexnormals without faces: 
							List<double[]> zeroNormals=new LinkedList<double[]>();
							zeroNormals.add(new double[]{0,0,0});
							defaultVertexAttributeValues.add(zeroNormals);
						}
					}
				}
			}
		}
	}

	private void collectMergeData(
			List<IndexedFaceSet> g,
			List<double[]> tra,
			List<double[]> fCol,
			List<double[]> eCol,
			List<double[]> vCol,
			SceneGraphComponent cmp,// sgc to handle
			SceneGraphPath p,
			EffectiveAppearance parentEA){

		// manage EApp and SGPath
		Appearance app =cmp.getAppearance();
		EffectiveAppearance	eApp=parentEA;
		if (app!=null){
			eApp=parentEA.create(app);
		};
		p.push(cmp);
		updateShaders(eApp);
		// insert data
		Geometry myG=cmp.getGeometry();
		if(myG!= null && myG instanceof PointSet) {

			// geometry 
			IndexedFaceSet myface;
			PointSet myPs = (PointSet) myG;
			if (myPs instanceof IndexedLineSet) {
				IndexedLineSet myLSet = (IndexedLineSet) myPs;	
				if (myPs instanceof IndexedFaceSet) {
					myface = (IndexedFaceSet) myPs;
				}else myface=indexedLineSetToIndexedFaceSet(myLSet);
			}else myface=pointSetToIndexedFaceSet(myPs); 
			g.add(myface);
			// color
			Color Mycol;
			checkColorNeed(eApp,myface);
			Mycol=dps.getDiffuseColor();
			fCol.add(new double[]{((double)Mycol.getRed())/255,((double)Mycol.getGreen())/255,((double)Mycol.getBlue())/255,((double)Mycol.getAlpha())/255});
			Mycol=dls.getDiffuseColor();
			if (dls.getTubeDraw()){
				try {
					Color c=((DefaultPolygonShader)(dls.getPolygonShader())).getDiffuseColor();
					if(c!=null) Mycol=c;
				} catch (Exception e) {}
			}
			eCol.add(new double[]{((double)Mycol.getRed())/255,((double)Mycol.getGreen())/255,((double)Mycol.getBlue())/255,((double)Mycol.getAlpha())/255});
			Mycol=dvs.getDiffuseColor();
			if (dvs.getSpheresDraw()){
				try {
					Color c=((DefaultPolygonShader)(dvs.getPolygonShader())).getDiffuseColor();
					if(c!=null) Mycol=c;
				} catch (Exception e) {}
			}
			vCol.add(new double[]{((double)Mycol.getRed())/255,((double)Mycol.getGreen())/255,((double)Mycol.getBlue())/255,((double)Mycol.getAlpha())/255});
			// transformation
			double[] mat=p.getMatrix(new Matrix().getArray());
			tra.add(mat);
		}
		// append subnode Data
		for (SceneGraphComponent child : cmp.getChildComponents()) {
				collectMergeData(g,tra,fCol,eCol,vCol,child,p,eApp);
		}
		
		// undo scenegraphpath extention
		p.pop();
	}
	private static void updateShaders(EffectiveAppearance eap) {
		dgs =ShaderUtility.createDefaultGeometryShader(eap);
		if (dgs.getPointShader() instanceof DefaultPointShader){	
			dvs = (DefaultPointShader) dgs.getPointShader();
		}
		else dvs = null;
		if (dgs.getLineShader() instanceof DefaultLineShader){ 
			dls = (DefaultLineShader) dgs.getLineShader();
		}
		else dls = null;
		if (dgs.getPolygonShader() instanceof DefaultPolygonShader){
			dps = (DefaultPolygonShader) dgs.getPolygonShader();
		}
		else dps = null;
	}
	private void checkColorNeed(EffectiveAppearance eApp, IndexedFaceSet ifs) {
		if(eApp.getAttribute(CommonAttributes.DIFFUSE_COLOR, Appearance.INHERITED)!=Appearance.INHERITED){
			hasEdgeColors=true;
			hasFaceColors=true;
			hasVertexColors=true;
		}
		if(eApp.getAttribute(CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, Appearance.INHERITED)!=Appearance.INHERITED)
			hasFaceColors=true;
		if(	eApp.getAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, Appearance.INHERITED)!=Appearance.INHERITED
				||eApp.getAttribute(CommonAttributes.LINE_SHADER+"."+CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, Appearance.INHERITED)!=Appearance.INHERITED)
			hasEdgeColors=true;
		if(	eApp.getAttribute(CommonAttributes.VERTEX_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, Appearance.INHERITED)!=Appearance.INHERITED
				||eApp.getAttribute(CommonAttributes.VERTEX_SHADER+"."+CommonAttributes.POLYGON_SHADER+"."+CommonAttributes.DIFFUSE_COLOR, Appearance.INHERITED)!=Appearance.INHERITED)
			hasVertexColors=true;
		if(ifs.getVertexAttributes(Attribute.COLORS)!=null)
			hasVertexColors=true;
		if(ifs.getEdgeAttributes(Attribute.COLORS)!=null)
			hasEdgeColors=true;
		if(ifs.getFaceAttributes(Attribute.COLORS)!=null)
			hasFaceColors=true;
		
	}
	// --------- public methods ------------
	/** merges all IndexedFaceSets, IndexeedLineSets and PointSets
	 *  of the given SceneGraph to one IndexedFaceSet
	 *  Attention: several values can be set bevorhand
	 * @param RootNode 
	 */
	public IndexedFaceSet mergeGeometrySets(SceneGraphComponent cmp){
		// init Eap
		Appearance app = cmp.getAppearance();
		if (app == null) app = new Appearance();
		EffectiveAppearance eApp= EffectiveAppearance.create();
		eApp= eApp.create(app);

		// collect Data
		List<IndexedFaceSet> geos= new LinkedList<IndexedFaceSet>();
		List<double[]> fCol=new LinkedList<double[]>();
		List<double[]> eCol=new LinkedList<double[]>();
		List<double[]> vCol=new LinkedList<double[]>();
		List<double[]> trafos= new LinkedList<double[]>();
		hasEdgeColors=false;
		hasFaceColors=false;
		hasVertexColors=false;
		collectMergeData(geos, trafos,fCol,eCol,vCol, cmp,new SceneGraphPath(),eApp);

		///// geather and compute:
		// strukturieren
		int num=geos.size();
		IndexedFaceSet[] faces= new IndexedFaceSet[num];
		int i=0;
		for (Iterator iter = geos.iterator(); iter.hasNext();i++) {
			faces[i]= (IndexedFaceSet) iter.next();
			CopyVisitor cv = new CopyVisitor();
			cv.visit(faces[i]);
			faces[i]= (IndexedFaceSet) cv.getCopy();

		}
		//  anpassen (trafo)
		i=0;
		for (Iterator iter = trafos.iterator(); iter.hasNext();i++) {
			double[] m = (double[]) iter.next();
			assignTransformation(faces[i], m);
		}		
		//  mergen
		IndexedFaceSet f= new IndexedFaceSet();

		if(hasFaceColors&&respectFacesIntern){
				if(!defaultFaceAttributes.contains(Attribute.COLORS)){
					defaultFaceAttributes.add(Attribute.COLORS);
					defaultFaceAttributeValues.add(fCol);
				}
				else defaultFaceAttributeValues.set(
						defaultFaceAttributes.indexOf(Attribute.COLORS)	, fCol);
		}
		if(hasEdgeColors&&respectEdgesIntern){
				if(!defaultEdgeAttributes.contains(Attribute.COLORS)){
					defaultEdgeAttributes.add(Attribute.COLORS);
					defaultEdgeAttributeValues.add(eCol);
				}
				else defaultEdgeAttributeValues.set(
						defaultEdgeAttributes.indexOf(Attribute.COLORS)	, eCol);
		}
		if(hasVertexColors){
			if(!defaultVertexAttributes.contains(Attribute.COLORS)){
				defaultVertexAttributes.add(Attribute.COLORS);
				defaultVertexAttributeValues.add(vCol);
			}
			else defaultVertexAttributeValues.set(
					defaultVertexAttributes.indexOf(Attribute.COLORS)	, vCol);
		}
		f=mergeIndexedFaceSets(faces);
		return f;
	}  
	
	/** merges all IndexedFaceSets to one IndexedFaceSet
	 *  Attention: several values can be set bevorhand
	 * @param IndexedFaceSets to merge 
	 */
	public IndexedFaceSet mergeIndexedFaceSets( PointSet[] geo) {
		if(geo== null)return null;
		if(geo.length==0)return null;
		IndexedFaceSet[] ifs=new IndexedFaceSet[geo.length];
		// convert entrys to IndexedFaceSets
		for (int i = 0; i < geo.length; i++) {
			ifs[i]=pointSetToIndexedFaceSet(geo[i]);
			if(generateFaceNormals && ifs[i].getNumFaces()>0 && ifs[i].getVertexAttributes(Attribute.NORMALS)==null)
				IndexedFaceSetUtility.calculateAndSetFaceNormals(ifs[i]);
			if (generateFaceNormals && ifs[i].getNumFaces()>0 && ifs[i].getFaceAttributes(Attribute.NORMALS)==null)
				IndexedFaceSetUtility.calculateAndSetVertexNormals(ifs[i]);
		}
		//
		indexedFaceSetTo4DColorNormalsAndCoords(ifs);
		IndexedFaceSet result = new IndexedFaceSet();
		DataListSet[] faceDls= new DataListSet [ifs.length];
		DataListSet[] edgeDls= new DataListSet [ifs.length];
		DataListSet[] vertDls= new DataListSet [ifs.length];
		if (respectFacesIntern){
			for (int j = 0; j < faceDls.length; j++)
				faceDls[j]=ifs[j].getFaceAttributes();
			// evtl null (egal)
			int [][] faceIndices = mergeIntArrayArrayAttribute( faceDls, Attribute.INDICES );
			int n=ifs[0].getNumPoints();
			int k=ifs[0].getNumFaces();
			// angaben ok (keine Liste liefert 0)
			for( int i=1 ; i<ifs.length;  i++ ) {
				for( int f=0; f<ifs[i].getNumFaces(); f++ ) {
					final int [] face = faceIndices[k];
					for( int j=0; j<face.length; j++)	
						face[j] += n;
					k++;
				}
				n += ifs[i].getNumPoints();
			}
			if(faceIndices==null) {respectFacesIntern=false;} 
			else
				result.setFaceCountAndAttributes(
						Attribute.INDICES,
						new IntArrayArray.Array( faceIndices )
				);
		}
		if(respectEdgesIntern){
			for (int j = 0; j < edgeDls.length; j++) 
				edgeDls[j]=ifs[j].getEdgeAttributes();
			int [][] edgeIndices = mergeIntArrayArrayAttribute(edgeDls, Attribute.INDICES );
			int n=ifs[0].getNumPoints();
			int k=ifs[0].getNumEdges();
			for( int i=1; i<ifs.length; i++ ) {
				for( int f=0; f<ifs[i].getNumEdges(); f++ ) {
					final int [] edge = edgeIndices[k];
					for( int j=0; j<edge.length; j++) 
						edge[j] += n;
					k++;;
				}	
				n += ifs[i].getNumPoints();
			}
			if(edgeIndices==null) {respectEdgesIntern=false;}
			else
				result.setEdgeCountAndAttributes(
					Attribute.INDICES,
					new IntArrayArray.Array( edgeIndices )
			);
		}
		for (int j = 0; j < vertDls.length; j++) 
			vertDls[j]=ifs[j].getVertexAttributes();
		
		mergeDoubleArrayArrayAttributes(result,defaultVertexAttributes,defaultVertexAttributeValues,importantVertexDefaultAttributes,vertDls,VERT_ATTR);
		if(respectEdgesIntern)mergeDoubleArrayArrayAttributes(result,defaultEdgeAttributes,defaultEdgeAttributeValues,importantEdgeDefaultAttributes,edgeDls,EDGE_ATTR);
		if(respectFacesIntern)mergeDoubleArrayArrayAttributes(result,defaultFaceAttributes,defaultFaceAttributeValues,importantFaceDefaultAttributes,faceDls,FACE_ATTR);
		respectEdgesIntern=respectEdges;
		respectFacesIntern=respectFaces;
		return result;
	}
	/** merges all IndexeedLineSets to one IndexedLineSet
	 *  Attention: several values can be set bevorhand
	 * @param IndexedLineSets to merge
	 */
	public IndexedLineSet mergeIndexedLineSets( IndexedLineSet [] ils){
		respectFacesIntern=false;
		IndexedFaceSet f=mergeIndexedFaceSets(ils);
		IndexedLineSet l=indexedFaceSetToIndexedLineSet(f);
		return l;
	}
	/** merges all PointSets to one PointSet
	 *  Attention: several values can be set bevorhand
	 * @param Pointsets to merge 
	 */

	public PointSet mergePointSets( PointSet [] ps){
		respectFacesIntern=false;
		respectEdgesIntern=false;
		IndexedFaceSet f=mergeIndexedFaceSets(ps);
		PointSet p=indexedFaceSetToPointSet(f);
		return p;
	}
	/** merges all IndexedFaceSets, IndexeedLineSets and PointSets
	 *  of the given SceneGraph to one IndexedFaceSet
	 *  EdgeAttributes will be ignored.
	 *  Attention: several values can be set bevorhand
	 * @param RootNode 
	 */
	public IndexedFaceSet mergeIndexedFaceSets(SceneGraphComponent cmp){
		IndexedFaceSet f=mergeGeometrySets(cmp);
		return f;
	}
	/** merges all IndexedFaceSets, IndexeedLineSets and PointSets
	 *  of the given SceneGraph to one IndexedLineSet
	 *  FaceAttributes will be ignored.
	 *  Attention: several values can be set bevorhand
	 * @param RootNode 
	 */
	public IndexedLineSet mergeIndexedLineSets(SceneGraphComponent cmp){
		respectFacesIntern= false;
		IndexedLineSet l=indexedFaceSetToIndexedLineSet(mergeGeometrySets(cmp));
		return l;
	}
	/** merges all IndexedFaceSets, IndexeedLineSets and PointSets
	 *  of the given SceneGraph to one PointSet
	 *  Face and Edge-Attributes will be ignored.
	 *  Attention: several values can be set bevorhand
	 * @param RootNode 
	 */
	public PointSet mergePointSets(SceneGraphComponent cmp){
		respectFacesIntern= false;		respectEdgesIntern= false;
		PointSet p=indexedFaceSetToPointSet(mergeGeometrySets(cmp));
		return p;
	}
	// ------------- setters ----------

	// -------------- setter --------------
	/**Attributes which will be set to the given default, 
	* if not yet supported in the geometry.
	* must be <code>doubleArrayArray</code> Attributes !!
	* @param defaultFaceAttributes  list of default Attributes    
	* @param defaultAttributeValues 
	* 		must have the same length. 
	* 		Each entry can individualy have
	* 			- just a single entry (default for all is the same)
	* 		    - multiple entrys (one for each geometry)
	* 			- null or empty (works like no default is given)
	*/
	public void setDefaultFaceAttributes(
	List<Attribute> defaultAttributes
	,List<List <double[]>> defaultAttributeValues) {
	this.defaultFaceAttributeValues= defaultAttributeValues;
	this.defaultFaceAttributes= defaultAttributes;
	}
	/** see <code>setDefaultFaceAttributes</code> */
	public void setDefaultEdgeAttributes(
	List<Attribute> defaultAttributes
	,List<List <double[]>> defaultAttributeValues) {
	this.defaultFaceAttributeValues= defaultAttributeValues;
	this.defaultFaceAttributes= defaultAttributes;
	}
	/** see <code>setDefaultFaceAttributes</code> */
	public void setDefaultVertexAttributes(
	List<Attribute> defaultAttributes
	,List<List <double[]>> defaultAttributeValues) {
	this.defaultFaceAttributeValues= defaultAttributeValues;
	this.defaultFaceAttributes= defaultAttributes;
	}
	/**default Attributes wich are defined will normaly not be used if 
	* no Geometry supports them. 
	* Attributes which are also listed here will although be used.
	* 
	* @param importantDefaultAttributes
	*/
	public void setImportantFaceDefaultAttributes(
	List<Attribute> importantFaceDefaultAttributes) {
	this.importantFaceDefaultAttributes = importantFaceDefaultAttributes;
	}
	public void setImportantEdgeDefaultAttributes(
	List<Attribute> importantEdgeDefaultAttributes) {
	this.importantEdgeDefaultAttributes = importantEdgeDefaultAttributes;
	}
	public void setImportantVertexDefaultAttributes(
	List<Attribute> importantVertexDefaultAttributes) {
	this.importantVertexDefaultAttributes = importantVertexDefaultAttributes;
	}
	/** generates FaceNormals if not already given 
	* @param generateVertexNormals
	*/
	public void setGenerateFaceNormals(boolean generateFaceNormals) {
	this.generateFaceNormals = generateFaceNormals;
	}
	/** generates VertexNormals if not already given 
	* @param generateVertexNormals
	*/
	public void setGenerateVertexNormals(boolean generateVertexNormals) {
	this.generateVertexNormals = generateVertexNormals;
	}
	/**Edge Attributes will not be ignored  
	* @param respectEdges
	*/
	public void setRespectEdges(boolean respectEdges) {
		this.respectEdges = respectEdges;
		this.respectEdgesIntern = respectEdges;
		}
	/**Face Attributes will not be ignored  
	* @param respectEdges
	*/
	public void setRespectFaces(boolean respectFaces) {
		this.respectFaces = respectFaces;
		this.respectFacesIntern = respectFaces;
		}
	/**Verrtex Attributes will not be ignored  
	* @param respectEdges
	* @deprecated use not yet implemented 
	*/
	public void setRespectVertices(boolean respectVertices) {
		this.respectVertices = respectVertices;
		this.respectVerticesIntern = respectVertices;
		}
	// ----------- getters ------------
	public List<Attribute> getDefaultEdgeAttributes() {
	return defaultEdgeAttributes;
	}
	public List<List<double[]>> getDefaultEdgeAttributeValues() {
	return defaultEdgeAttributeValues;
	}
	public List<Attribute> getDefaultFaceAttributes() {
	return defaultFaceAttributes;
	}
	public List<List<double[]>> getDefaultFaceAttributeValues() {
	return defaultFaceAttributeValues;
	}
	public List<Attribute> getDefaultVertexAttributes() {
	return defaultVertexAttributes;
	}
	public List<List<double[]>> getDefaultVertexAttributeValues() {
	return defaultVertexAttributeValues;
	}
	public List<Attribute> getImportantFaceDefaultAttributes() {
		return importantFaceDefaultAttributes;
	}
	public List<Attribute> getImportantEdgeDefaultAttributes() {
		return importantEdgeDefaultAttributes;
	}
	public List<Attribute> getImportantVertexDefaultAttributes() {
		return importantVertexDefaultAttributes;
	}
	public boolean isGenerateFaceNormals() {
	return generateFaceNormals;
	}
	public boolean isGenerateVertexNormals() {
	return generateVertexNormals;
	}
	public boolean isRespectEdges() {
	return respectEdges;
	}
	public boolean isRespectFaces() {
	return respectFaces;
	}
	/** @deprecated use not yet implemented */
	public boolean isRespectVertices() {
	return respectVertices;
	}
	
	//---------problems with 3 and 4 dimensional Coordinates and colors-------
	// soltion: generate the 4 netry with "1"
	
	private static void indexedFaceSetTo4DColorNormalsAndCoords(IndexedFaceSet[] list){
		
		// TODO macht das probleme?
		
		for(IndexedFaceSet ifs: list){
			DataList d= ifs.getFaceAttributes(Attribute.NORMALS);
// it's too dangerous to allow possible mixture of 3D and 4D normals
// this code needs to check for this possibility before it gets reactivated [gunn 18.09.08]
//			if(d!=null){
//				double[][] ds=d.toDoubleArrayArray(null);
//				equalizeTo4D(ds);
//				ifs.setFaceAttributes(Attribute.NORMALS,null);
//				ifs.setFaceAttributes(Attribute.NORMALS,new DoubleArrayArray.Array(ds));
//			}
//			d= ifs.getEdgeAttributes(Attribute.NORMALS);
//			if(d!=null){
//				double[][] ds=d.toDoubleArrayArray(null);
//				equalizeTo4D(ds);
//				ifs.setEdgeAttributes(Attribute.NORMALS,null);
//				ifs.setEdgeAttributes(Attribute.NORMALS,new DoubleArrayArray.Array(ds));
//			}
//			d= ifs.getVertexAttributes(Attribute.NORMALS);
//			if(d!=null){
//				double[][] ds=d.toDoubleArrayArray(null);
//				equalizeTo4D(ds);
//				ifs.setVertexAttributes(Attribute.NORMALS,null);
//				ifs.setVertexAttributes(Attribute.NORMALS,new DoubleArrayArray.Array(ds));
//			}
			//
			d= ifs.getFaceAttributes(Attribute.COLORS);
			if(d!=null){
				double[][] ds=d.toDoubleArrayArray(null);
				equalizeTo4D(ds);
				ifs.setFaceAttributes(Attribute.COLORS,null);
				ifs.setFaceAttributes(Attribute.COLORS,new DoubleArrayArray.Array(ds));
			}
			d= ifs.getEdgeAttributes(Attribute.COLORS);
			if(d!=null){
				double[][] ds=d.toDoubleArrayArray(null);
				equalizeTo4D(ds);
				ifs.setEdgeAttributes(Attribute.COLORS,null);
				ifs.setEdgeAttributes(Attribute.COLORS,new DoubleArrayArray.Array(ds));
			}
			d= ifs.getVertexAttributes(Attribute.COLORS);
			if(d!=null){
				double[][] ds=d.toDoubleArrayArray(null);
				equalizeTo4D(ds);
				ifs.setVertexAttributes(Attribute.COLORS,null);
				ifs.setVertexAttributes(Attribute.COLORS,new DoubleArrayArray.Array(ds));
			}
			d= ifs.getVertexAttributes(Attribute.COORDINATES);
			if(d!=null){
				double[][] ds=d.toDoubleArrayArray(null);
				equalizeTo4D(ds);
				ifs.setVertexAttributes(Attribute.COORDINATES,null);
				ifs.setVertexAttributes(Attribute.COORDINATES,new DoubleArrayArray.Array(ds));
			}
		}
	}
	private static void equalizeTo4D(double[][] d){
		for (int i = 0; i < d.length; i++) 
				if(d[i].length==3)
					d[i]=new double[]{d[i][0],d[i][1],d[i][2],1};
	}
	
	// --- converter 
	// TODO make without converter, use Visitor
	private static PointSet indexedLineSetToPointSet(IndexedLineSet l){
		PointSet p= new PointSet(l.getNumPoints());
		p.setGeometryAttributes(l.getGeometryAttributes());
		p.setVertexAttributes(l.getVertexAttributes());
		return p;
	}
	
	private static IndexedLineSet pointSetToIndexedLineSet(PointSet p){
		if (p instanceof IndexedLineSet)
			return (IndexedLineSet) p;
		IndexedLineSet l= new IndexedLineSet(p.getNumPoints(),0);
		l.setGeometryAttributes(p.getGeometryAttributes());
		l.setVertexAttributes(p.getVertexAttributes());
		return l;
	}
	private static PointSet indexedFaceSetToPointSet(IndexedFaceSet f){
		PointSet p= new PointSet(f.getNumPoints());
		p.setGeometryAttributes(f.getGeometryAttributes());
		p.setVertexAttributes(f.getVertexAttributes());
		return p;
	}
	private static IndexedLineSet indexedFaceSetToIndexedLineSet(IndexedFaceSet f){
		IndexedLineSet l= new IndexedLineSet(f.getNumPoints(),f.getNumEdges());
		l.setGeometryAttributes(f.getGeometryAttributes());
		l.setVertexAttributes(f.getVertexAttributes());
		l.setEdgeAttributes(f.getEdgeAttributes());
		return l;
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
}
