package de.tuberlin.polymake.common.jreality;

import java.awt.Color;

import de.jreality.geometry.IndexedFaceSetFactory;
import de.jreality.geometry.IndexedFaceSetUtility;
import de.jreality.math.Rn;
import de.jreality.plugin.JRViewer;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.data.Attribute;

public class SphericalSectionPolygon {
	
	private IndexedFaceSet patch = null;
	
	//TODO VertexNormals = coords!
	
	public SphericalSectionPolygon(double[][] vertices, int refinements) {
		patch = createPatch(vertices);
		for(int i = 0; i < refinements; ++i) {
			refine();
		}
		IndexedFaceSetUtility.calculateAndSetNormals(patch);
	}

	private IndexedFaceSet createPatch(double[][] vertices) {
		IndexedFaceSetFactory ifsf = new IndexedFaceSetFactory();
		ifsf.setGenerateEdgesFromFaces(true);
		ifsf.setGenerateFaceNormals(true);
		ifsf.setVertexCount(vertices.length);
		ifsf.setVertexCoordinates(vertices);
		ifsf.setVertexNormals(vertices);
		int[][] indices = new int[1][vertices.length];
		for(int j = 0; j < vertices.length; ++j){
			indices[0][j] = j;
		}
		ifsf.setFaceCount(1);
		ifsf.setFaceColors(new Color[]{new Color(1.0f,0.0f,0.0f,1.0f)});
		ifsf.setFaceIndices(indices);
		ifsf.update();
		return ifsf.getIndexedFaceSet();
	}

	public void refine() {
		double[][] verts = patch.getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray(null);
		double[][] newVerts = new double[2*verts.length-2][verts[0].length];
		int[][] indices = patch.getFaceAttributes(Attribute.INDICES).toIntArrayArray(null);
		int j = 0;
		for(int i = 0; i < indices[0].length; ++i) {
			System.arraycopy(verts[indices[0][i]], 0, newVerts[j], 0, verts[indices[0][i]].length);
			++j;
			if( Math.abs(Rn.euclideanNormSquared(verts[indices[0][(i+1)%indices[0].length]])-1) < 1E-3 &&
				Math.abs(Rn.euclideanNormSquared(verts[indices[0][i]])-1) < 1E-3) {
				newVerts[j] = Rn.normalize(null,Rn.linearCombination(null, .5, verts[indices[0][i]], .5, verts[indices[0][(i+1)%indices[0].length]]));
				++j;
			}
		}
		patch = createPatch(newVerts);
	}

	public IndexedFaceSet getPatch() {
		return patch;
	}

	public static void main(String[] args) {
		JRViewer v = new JRViewer();
		v.addContentUI();
		v.addBasicUI();
		double[][] coords = new double[][]{
				{0.0,0.0,0.0},
				{0.0,1.0,0.0},
				{1.0,0.0,0.0}};
		SphericalSectionPolygon ssp = new SphericalSectionPolygon(coords, 5);
		v.setContent(ssp.getPatch());
		v.startup();
	}
}
