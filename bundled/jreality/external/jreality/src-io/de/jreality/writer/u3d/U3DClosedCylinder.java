package de.jreality.writer.u3d;

import static de.jreality.scene.data.Attribute.COORDINATES;
import static de.jreality.scene.data.Attribute.INDICES;
import static de.jreality.scene.data.StorageModel.DOUBLE3_ARRAY;
import static de.jreality.scene.data.StorageModel.INT_ARRAY_ARRAY;
import static java.lang.Math.PI;
import static java.lang.Math.cos;
import static java.lang.Math.sin;
import de.jreality.geometry.IndexedFaceSetUtility;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.data.DataList;


public class U3DClosedCylinder extends IndexedFaceSet{

	private static final double[]
	    zPosNormal = {0,0,1},
	    zNegNormal = {0,0,-1};
	
	public U3DClosedCylinder() {
	}
	
	public U3DClosedCylinder(int resolution, double thickness){
		makeDisk(resolution, thickness);
		IndexedFaceSetUtility.calculateAndSetNormals(this);
	}
	
	public U3DClosedCylinder(int resolution){
		makeDiskNoThickness(resolution);
		IndexedFaceSetUtility.calculateAndSetNormals(this);
	}
	
	
	private void makeDisk(int resolution, double thickness){
		double[][] verts = new double[resolution*2 + 2][3];
		double alpha = 0;
		double delta = 2*PI / resolution;
		for (int i = 0; i < resolution*2; i += 2){
			verts[i][0] = verts[i + 1][0] = cos(alpha);
			verts[i][1] = verts[i + 1][1] = sin(alpha);
			verts[i][2] = -thickness / 2;
			verts[i + 1][2] = thickness / 2;
			alpha += delta;
		}
		verts[resolution*2 + 0][2] = -thickness / 2;
		verts[resolution*2 + 1][2] = thickness / 2;
		
		int[][] indices = new int[resolution * 3][];
		double[][] normals = new double[resolution*3][];
		int v = 0;
		alpha = delta / 2;
		for (int i = 0; i < indices.length; i+=3) {
			int[] faceUpper = indices[i+0] = new int[3];
			int[] faceLower = indices[i+1] = new int[3];
			int[] faceBorder = indices[i+2] = new int[4];
			normals[i + 0] = zNegNormal;
			normals[i + 1] = zPosNormal;
			normals[i + 2] = new double[]{cos(alpha), sin(alpha), 0.0};
			faceUpper[0] = (v + 2) % (resolution*2);
			faceUpper[1] = v;
			faceUpper[2] = resolution*2;
			faceLower[0] = resolution*2 + 1;
			faceLower[1] = v + 1;
			faceLower[2] = (v + 3) % (resolution*2);
			faceBorder[0] = faceUpper[1];
			faceBorder[1] = faceUpper[0];
			faceBorder[2] = faceLower[2];
			faceBorder[3] = faceLower[1];
			v += 2;
			alpha += delta;
		}
		setNumPoints(verts.length);
		setNumFaces(indices.length);
		DataList vList = DOUBLE3_ARRAY.createReadOnly(verts);
		setVertexCountAndAttributes(COORDINATES, vList);
		DataList iList = INT_ARRAY_ARRAY.createReadOnly(indices);
		setFaceCountAndAttributes(INDICES, iList);
		setName("disk");
	}
	
	
	
	
	private void makeDiskNoThickness(int resolution){
		double[][] verts = new double[resolution + 1][3];
		double[][] normals = new double[resolution + 1][];
		double alpha = 0;
		double delta = 2*PI / resolution;
		for (double[] p : verts){
			p[0] = cos(alpha);
			p[1] = sin(alpha);
			alpha += delta;
		}
		for (int i = 0; i < normals.length; i++)
			normals[i] = zPosNormal;
		
		int[][] indices = new int[resolution][3];
		for (int i = 0; i < indices.length; i++) {
			int[] face = indices[i];
			face[0] = resolution;
			face[1] = i;
			face[2] = (i + 1) % resolution;
		}
		setNumPoints(verts.length);
		setNumFaces(indices.length);
		DataList vList = DOUBLE3_ARRAY.createReadOnly(verts);
		setVertexCountAndAttributes(COORDINATES, vList);
		DataList iList = INT_ARRAY_ARRAY.createReadOnly(indices);
		setFaceCountAndAttributes(INDICES, iList);
	}
	

	
}
