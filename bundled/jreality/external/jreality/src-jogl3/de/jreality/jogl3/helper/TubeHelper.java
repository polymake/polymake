package de.jreality.jogl3.helper;

import java.util.HashMap;

import javax.media.opengl.GL3;

import de.jreality.jogl3.shader.GLVBO;
import de.jreality.jogl3.shader.GLVBOFloat;

public class TubeHelper {
	
	private HashMap<Integer, GLVBO> tubeVBOs = new HashMap<Integer, GLVBO>();
	
	public GLVBO getLineVBO(GL3 gl, int detail){
		if(tubeVBOs.containsKey(new Integer(detail))){
			return tubeVBOs.get(new Integer(detail));
		}else{
			GLVBOFloat newVBO = new GLVBOFloat(gl, getTubeCoordinatesArray(detail), "tubeCoords");
			tubeVBOs.put(new Integer(detail), newVBO);
			return newVBO;
		}
	}
	
	public static float[] getTubeCoordinatesArray(int detail){
		return getAngleTubeCoordinatesArray(detail, 2);
	}
	
	public static float[] getHalfTubeCoordinatesArray(int detail){
		return getAngleTubeCoordinatesArray(detail, 1);
	}
	
	//returns the part of a tube or radius 1 from (0,0,0) to (1,0,0) which has z < 0;
	//returns only vertex coordinates
	private static float[] getAngleTubeCoordinatesArray(int detail, int halves){
		int n = detail+2;
		//4 floats per vertex, 6 vertices per quad, n quads
		float[] tube = new float[4*6*n*halves];
		for(int j = 0; j < n*halves; j++){
			//do a quad
			tube[j*24 + 0] = 0;
			tube[j*24 + 1] = (float)Math.cos((Math.PI*j)/n);
			tube[j*24 + 2] = -(float)Math.sin((Math.PI*j)/n);
			tube[j*24 + 3] = 1;
			
			tube[j*24 + 4] = 0;
			tube[j*24 + 5] = (float)Math.cos((Math.PI*(j+1))/n);
			tube[j*24 + 6] = -(float)Math.sin((Math.PI*(j+1))/n);
			tube[j*24 + 7] = 1;
			
			tube[j*24 + 8] = 1;
			tube[j*24 + 9] = (float)Math.cos((Math.PI*(j+1))/n);
			tube[j*24 + 10] = -(float)Math.sin((Math.PI*(j+1))/n);
			tube[j*24 + 11] = 1;
			
			tube[j*24 + 12] = 1;
			tube[j*24 + 13] = (float)Math.cos((Math.PI*(j+1))/n);
			tube[j*24 + 14] = -(float)Math.sin((Math.PI*(j+1))/n);
			tube[j*24 + 15] = 1;
			
			tube[j*24 + 16] = 1;
			tube[j*24 + 17] = (float)Math.cos((Math.PI*j)/n);
			tube[j*24 + 18] = -(float)Math.sin((Math.PI*j)/n);
			tube[j*24 + 19] = 1;
			
			tube[j*24 + 20] = 0;
			tube[j*24 + 21] = (float)Math.cos((Math.PI*j)/n);
			tube[j*24 + 22] = -(float)Math.sin((Math.PI*j)/n);
			tube[j*24 + 23] = 1;
		}
		return tube;
	}
	
	//returns the part of a tube or radius 1 from (0,0,0) to (1,0,0) which has z < 0;
	//returns only vertex normals
//	public static float[] getAngleTubeNormalsArray(int detail, int halves){
//		
//		int n = detail+2;
//		//4 floats per vertex, 6 vertices per quad, n quads
//		float[] tube = new float[4*6*n*halves];
//		for(int j = 0; j < n*halves; j++){
//			//do a quad
//			//this is really simple now: just take the same values as for the coordinates,
//			//and only set x to 0
//			tube[j*24 + 0] = 0;
//			tube[j*24 + 1] = (float)Math.cos((Math.PI*j)/n);
//			tube[j*24 + 2] = -(float)Math.sin((Math.PI*j)/n);
//			tube[j*24 + 3] = 1;
//			
//			tube[j*24 + 4] = 0;
//			tube[j*24 + 5] = (float)Math.cos((Math.PI*(j+1))/n);
//			tube[j*24 + 6] = -(float)Math.sin((Math.PI*(j+1))/n);
//			tube[j*24 + 7] = 1;
//			
//			tube[j*24 + 8] = 0;
//			tube[j*24 + 9] = (float)Math.cos((Math.PI*(j+1))/n);
//			tube[j*24 + 10] = -(float)Math.sin((Math.PI*(j+1))/n);
//			tube[j*24 + 11] = 1;
//			
//			tube[j*24 + 12] = 0;
//			tube[j*24 + 13] = (float)Math.cos((Math.PI*(j+1))/n);
//			tube[j*24 + 14] = -(float)Math.sin((Math.PI*(j+1))/n);
//			tube[j*24 + 15] = 1;
//			
//			tube[j*24 + 16] = 0;
//			tube[j*24 + 17] = (float)Math.cos((Math.PI*j)/n);
//			tube[j*24 + 18] = -(float)Math.sin((Math.PI*j)/n);
//			tube[j*24 + 19] = 1;
//			
//			tube[j*24 + 20] = 0;
//			tube[j*24 + 21] = (float)Math.cos((Math.PI*j)/n);
//			tube[j*24 + 22] = -(float)Math.sin((Math.PI*j)/n);
//			tube[j*24 + 23] = 1;
//		}
//		return tube;
//	}
}
