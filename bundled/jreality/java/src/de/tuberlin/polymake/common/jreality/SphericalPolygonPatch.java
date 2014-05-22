package de.tuberlin.polymake.common.jreality;

import java.awt.Color;

import de.jreality.geometry.GeometryUtility;
import de.jreality.geometry.IndexedFaceSetFactory;
import de.jreality.geometry.IndexedFaceSetUtility;
import de.jreality.math.Rn;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.StorageModel;

public class SphericalPolygonPatch {

	private IndexedFaceSet patch = null;

	public SphericalPolygonPatch(double[][] vertices, int refinements) {
		IndexedFaceSetFactory ifsf = new IndexedFaceSetFactory();
		ifsf.setGenerateEdgesFromFaces(true);
		ifsf.setGenerateFaceNormals(true);
		ifsf.setVertexCount(vertices.length);
		ifsf.setVertexCoordinates(vertices);
		int[][] indices = new int[1][vertices.length];
		for(int j = 0; j < vertices.length; ++j){
			indices[0][j] = j;
		}
		ifsf.setFaceCount(1);
		ifsf.setVertexNormals(vertices);
		ifsf.setFaceColors(new Color[]{new Color(1.0f,0.0f,0.0f,1.0f)});
		ifsf.setFaceIndices(indices);
		ifsf.update();
		patch = ifsf.getIndexedFaceSet();
		if(vertices.length != 3) {
			IndexedFaceSetUtility.triangulateBarycentric(patch);
			double[][] verts = patch.getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray(null);
			int vlength = GeometryUtility.getVectorLength(patch);
			Rn.normalize(verts, verts);
			patch.setVertexAttributes(Attribute.COORDINATES, StorageModel.DOUBLE_ARRAY.array(vlength).createReadOnly(verts));
		}
		for(int i = 0; i < refinements; ++i) {
			refine();
		}
		IndexedFaceSetUtility.calculateAndSetNormals(patch);
	}
	
	public void refine() {
		IndexedFaceSet nifs = IndexedFaceSetUtility.binaryRefine(patch);
		double[][] verts = nifs.getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray(null);
		int vlength = GeometryUtility.getVectorLength(nifs);
		Rn.normalize(verts, verts);
		nifs.setVertexAttributes(Attribute.COORDINATES, StorageModel.DOUBLE_ARRAY.array(vlength).createReadOnly(verts));
		patch = nifs;
	}
	
	public IndexedFaceSet getPatch() {
		return patch;
	}
	
	public static void main(String[] args) {
		JRViewer v = new JRViewer();
		v.addContentUI();
		v.addBasicUI();
		double[][] coords = new double[][]{
		                               {0.0,0.0,1.0},
		                               {0.0,1.0,0.0},
		                               {1.0,0.0,0.0}};
		SphericalPolygonPatch spp = new SphericalPolygonPatch(coords, 5);
		v.setContent(spp.getPatch());
		v.startup();
	}
}
