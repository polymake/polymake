/* Copyright (c) 1997-2020
   Ewgenij Gawrilow, Michael Joswig, and the polymake team
   Technische Universität Berlin, Germany
   https://polymake.org

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version: http://www.gnu.org/licenses/gpl.txt.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
--------------------------------------------------------------------------------
*/

package de.tuberlin.polymake.common.jreality;

import java.util.LinkedList;
import java.util.List;
import java.util.Set;

import de.jreality.geometry.IndexedFaceSetFactory;
import de.jreality.math.Matrix;
import de.jreality.math.MatrixBuilder;
import de.jreality.math.Rn;
import de.jreality.scene.Appearance;
import de.jreality.scene.Geometry;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.PointSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphNode;
import de.jreality.scene.Transformation;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.DataList;
import de.jreality.scene.data.DataListSet;
import de.jreality.scene.data.DoubleArrayArray;
import de.jreality.util.SceneGraphUtility;

public class Utils {

	public static void explodeGeometry(SceneGraphComponent geom, double scale) {
		int n_comp = geom.getChildComponentCount();
		if (!(n_comp == 0)) {

			SceneGraphComponent center = geom.getChildComponent(0);
			double[] barycenter = new double[3];
			int l = 0;
			for(double[] pt : collectPoints(center)) {
				Rn.add(barycenter, barycenter, pt);
				++l;
			}
			Rn.times(barycenter, 1.0/l,  barycenter);
			
			for (int k = 1; k < n_comp; ++k) {
				
				l = 0;
				double[] center1 = new double[3];
				SceneGraphComponent childComponent = geom.getChildComponent(k);
				for(double[] pt : collectPoints(childComponent)) {
					Rn.add(center1, center1, pt);
					++l;
				}
				Rn.times(center1, 1.0/l, center1);
				double[] t = new double[3];
				Rn.subtract(t, center1, barycenter);

				Matrix m = MatrixBuilder.euclidean().translate(Rn.times(null, scale, t)).getMatrix();

				double[] mat = m.getArray();
				Transformation trans = new Transformation(mat);
				if(childComponent.getName().equalsIgnoreCase("CoordinateSystem")){
				    continue;
				}
				childComponent.setTransformation(trans);
			}
		}
	}
	
	private static List<double[]> collectPoints(SceneGraphComponent sgc) {
		System.err.println(sgc.getName());
		List<double[]> pointsList = new LinkedList<double[]>();
		Geometry scene_geom = sgc.getGeometry();
		if (scene_geom instanceof de.jreality.scene.PointSet) {
			PointSet pts = (PointSet) scene_geom;
			double[][] coords = ((DoubleArrayArray) pts.getVertexAttributes(Attribute.COORDINATES)).toDoubleArrayArray(null);
			for (int i = 0; i < coords.length; i++) {
				pointsList.add(coords[i]);
			}
		}
		for(SceneGraphComponent child : sgc.getChildComponents()) {
		    if(!child.getName().equalsIgnoreCase("BAS") && !child.getName().equalsIgnoreCase("CoordinateSystem")) {
				pointsList.addAll(collectPoints(child));
			} 
		}
		return pointsList;
	}

	public static void refineGeometry(SceneGraphComponent geom, List<SceneGraphComponent> children, int level, boolean sphere) {
		if(children == null) {
			children = geom.getChildComponents();
		}
		for(SceneGraphComponent child : children) {
			Geometry geometry = child.getGeometry();
			if(geometry == null) {
				refineGeometry(child, null, level, sphere);
			} else if(geometry instanceof IndexedFaceSet) {
				SceneGraphUtility.removeChildNode(geom, child);
				IndexedFaceSet ifs = (IndexedFaceSet) geometry;
				int[][] indices = ifs.getFaceAttributes(Attribute.INDICES).toIntArrayArray().toIntArrayArray(null);
				double[][] coords = ifs.getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray(null);
//				double[][] colors = ifs.getFaceAttributes(Attribute.COLORS).toDoubleArrayArray(null);
				SceneGraphComponent cone = new SceneGraphComponent(child.getName() + " (Spherical cone)");
				for(int i = 0; i < indices.length; ++i) {
					double[][] patchCoords = new double[indices[i].length][coords[0].length];
					int j = 0;
					boolean containsOrigin = false;
					for (j = 0; j < patchCoords.length; j++) {
						System.arraycopy(coords[indices[i][j]], 0, patchCoords[j], 0, coords[0].length);
						boolean isOrigin = Rn.euclideanNormSquared(patchCoords[j]) < 1E-3;
						containsOrigin |= isOrigin;
						if( !isOrigin && (Math.abs(Rn.euclideanNormSquared(patchCoords[j])-1) > 1E-3)) {
							break;
						}
					}
					if(j == patchCoords.length) {
//						double[] color = null;
//						if(colors != null) {
//							color = colors[i];
//						}
						if(containsOrigin) {
							SphericalSectionPolygon ssp = new SphericalSectionPolygon(patchCoords,level);
							SceneGraphComponent sgc = new SceneGraphComponent();
							sgc.setGeometry(ssp.getPatch());
							cone.addChild(sgc);
						} else if(sphere) {
							SphericalPolygonPatch spp = new SphericalPolygonPatch(patchCoords, level);
							SceneGraphComponent sgc = new SceneGraphComponent();
							sgc.setGeometry(spp.getPatch());
							cone.addChild(sgc);
						}
					}
				}
				geom.addChild(cone);
//				if(cone.getChildComponentCount() != 0) {
//			        GeometryMergeFactory mergeFact= new GeometryMergeFactory();             
//			        // you can set some defaults:
//			        List<Attribute> defaultAtts= new LinkedList<Attribute>();
//			        List<List<double[]>> defaultAttValue= new LinkedList<List<double[]>>();
//			        List<double[]> value= new LinkedList<double[]>();
//			        defaultAtts.add(Attribute.COLORS);
//			        defaultAttValue.add(value);
//			        value.add(new double[]{1,0,0,1});// remember: only 4d colors
//			        mergeFact.setDefaultFaceAttributes(defaultAtts,defaultAttValue );
//			        // merge a list of geometrys:
//			        //IndexedFaceSet[] list= new IndexedFaceSet[]{box2,zyl};
//			        //IndexedFaceSet result=mergeFact.mergeIndexedFaceSets(list);
//			        // or  a complete tree:
//			        IndexedFaceSet result = mergeFact.mergeGeometrySets(cone);
//			        SceneGraphComponent sgc = new SceneGraphComponent("Spherical Cone");
//			        sgc.setGeometry(result);
//					geom.addChild(sgc);
//				}
				
			} 
		}
	}

	public static void clearAttributes(SceneGraphComponent sgc) {
		de.jreality.scene.Geometry geom = sgc.getGeometry();
		if(geom != null) {
			for(String cat : geom.getGeometryAttributeCathegories()) {
				DataListSet attMap = geom.getAttributes(cat);
				for(Attribute att : new LinkedList<Attribute>(attMap.storedAttributes())) {
					if(att.equals(Attribute.COLORS)) { 
					   
						geom.setAttributes(cat,att,null);
					}
					//maybe clear RELATIVE_RADII
					if(att.equals(Attribute.RELATIVE_RADII)) {
						geom.setAttributes(cat,att,null);
					}
				}
			}
		}
		for(SceneGraphComponent child : sgc.getChildComponents()) {
			clearAttributes(child);
		}
	}
	
	public static void clearAppearance(SceneGraphComponent sgc) {
		Appearance oldAppearance = sgc.getAppearance();
		if(oldAppearance != null) {
			sgc.setAppearance(new Appearance());
		}
		for(SceneGraphComponent child : sgc.getChildComponents()) {
			clearAppearance(child);
		}
	}
	public void setTransparency(SceneGraphComponent geom, String name, double val) {	
		
		SceneGraphComponent g = null;
		int k = geom.getChildComponentCount();
		
		for(int j = 0; j < k ; ++j) {
			SceneGraphComponent sgc = geom.getChildComponent(j);
			if(sgc.getName() == name) {
				g = sgc;
				break;
			}
		}
			
		if(g!=null) {
			Appearance app = g.getAppearance();
			if(app == null)
				app = new Appearance();
			app.setAttribute("polygonShader.transparency", val);
			//System.err.println("POLYMAKE: Setting transparency:" + val);
			g.setAppearance(app);
			geom.setAppearance(app);
		} else {
			//System.err.println("POLYMAKE: geometry " + name + " does not exist!");
		}
		
	}

	/**
	 * @param i_sgc
	 * @return
	 */
	public static SceneGraphComponent copy(SceneGraphComponent i_sgc) {
		SceneGraphComponent sgc = new SceneGraphComponent();
		sgc.setAppearance((Appearance)SceneGraphUtility.copy(i_sgc.getAppearance()));
		sgc.setGeometry(new IndexedFaceSet());
		Set<String> categories = sgc.getGeometry().getGeometryAttributeCathegories();
		for (String category : categories) {
			DataListSet dls = i_sgc.getGeometry().getAttributes(category);
			for (Attribute attr : dls.storedAttributes()) {
				Object dl = new Object();
				dls.getList(attr).copyTo(dls.getList(attr).getStorageModel(),dl);
				sgc.getGeometry().setAttributes(category, attr,(DataList)dl);	
			}
		}
		return sgc;
	}

	public static void splitAllIndexedFaceSetsGeometry(SceneGraphComponent geom) {
		for(SceneGraphComponent child : new LinkedList<SceneGraphComponent>(geom.getChildComponents())) {
			splitAllIndexedFaceSetsGeometry(child);
			splitIndexedFaceSet(child);
		}
	}

	public static void splitIndexedFaceSet(SceneGraphComponent sgc) {
		Geometry geometry = sgc.getGeometry();
		if(geometry instanceof IndexedFaceSet) {
			Appearance appearance = sgc.getAppearance();
//			SceneGraphUtility.removeChildNode(parent, child);
			for(SceneGraphNode node : sgc.getChildNodes()) {
				if(node instanceof Geometry) {
					SceneGraphUtility.removeChildNode(sgc, node);
				} else if(node instanceof Appearance) {
					SceneGraphUtility.removeChildNode(sgc, node);
				}
			}
			IndexedFaceSet ifs = (IndexedFaceSet) geometry;
			int[][] indices = ifs.getFaceAttributes(Attribute.INDICES).toIntArrayArray().toIntArrayArray(null);
			double[][] coords = ifs.getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray(null);
			for(int i = 0; i < indices.length; ++i) {
				
				SceneGraphComponent face = new SceneGraphComponent("Face "+i);
				int length = indices[i].length;
				int[][] fincid = new int[1][length];
				double[][] fcoord = new double[length][coords[0].length];
				for(int j = 0; j < indices[i].length; ++j) {
					fincid[0][j] = j; 
					System.arraycopy(coords[indices[i][j]], 0, fcoord[j], 0, coords[0].length);
				}
				IndexedFaceSetFactory ifsf = new IndexedFaceSetFactory();
				ifsf.setGenerateFaceNormals(true);
				ifsf.setGenerateEdgesFromFaces(true);
				ifsf.setVertexCount(fcoord.length);
				ifsf.setFaceCount(1);
				ifsf.setVertexCoordinates(fcoord);
				ifsf.setFaceIndices(fincid);
				ifsf.update();
				face.setGeometry(ifsf.getGeometry());
				if(appearance != null) {
					face.setAppearance(SceneGraphUtility.copy(appearance));
				} else {
					face.setAppearance(new Appearance());
				}
				sgc.addChild(face);
			}
		}
	}

	
}
