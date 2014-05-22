package de.jreality.vr;

import java.util.Arrays;

import de.jreality.geometry.QuadMeshFactory;
import de.jreality.scene.IndexedFaceSet;

public class BigMesh {
	
	public static IndexedFaceSet bigMesh(int discretization, double cameraHeight, double size) {
		int n = discretization;
		QuadMeshFactory factory = new QuadMeshFactory();
		factory.setULineCount(n);
		factory.setVLineCount(n);
		factory.setGenerateEdgesFromFaces(true);
		factory.setGenerateTextureCoordinates(false);
		double totalAngle = Math.atan(size/cameraHeight);
		double dt = 2 * totalAngle/(n-1);
		double[] normal = new double[]{0,0,-1};
		double[][] normals = new double[n*n][];
		Arrays.fill(normals, normal);
		
		double[][][] coords = new double[n][n][3];

		for (int i=0; i<n; i++) {
			double y = cameraHeight * Math.tan(-totalAngle + i * dt);
			for (int j=0; j<n; j++) {
				coords[i][j][0] = cameraHeight * Math.tan(-totalAngle + j * dt);
				coords[i][j][1] = y;
			}
		}
		
		double[][][] texCoords = new double[n][n][2];
		for (int i=0; i<n; i++) {
			for (int j=0; j<n; j++) {
				texCoords[i][j][0] = coords[i][j][0];
				texCoords[i][j][1] = coords[i][j][1];
			}
		}
		
		factory.setVertexCoordinates(coords);
		factory.setVertexNormals(normals);
		factory.setVertexTextureCoordinates(texCoords);
		factory.update();
		
		return factory.getIndexedFaceSet();
	}
}