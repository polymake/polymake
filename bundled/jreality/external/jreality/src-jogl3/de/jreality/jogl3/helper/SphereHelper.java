package de.jreality.jogl3.helper;

import java.util.HashMap;

import javax.media.opengl.GL3;

import de.jreality.jogl3.shader.GLVBO;
import de.jreality.jogl3.shader.GLVBOFloat;

public class SphereHelper {
	
	private HashMap<Integer, GLVBO> sphereVBOs = new HashMap<Integer, GLVBO>();
	
	public GLVBO getSphereVBO(GL3 gl, int detail){
		if(sphereVBOs.containsKey(new Integer(detail))){
			return sphereVBOs.get(new Integer(detail));
		}else{
			GLVBOFloat newVBO = new GLVBOFloat(gl, getSphereCoordinatesArray(detail), "tubeCoords");
			sphereVBOs.put(new Integer(detail), newVBO);
			return newVBO;
		}
	}
	
	public GLVBO getHalfSphereVBO(GL3 gl, int detail){
		if(sphereVBOs.containsKey(new Integer(detail))){
			return sphereVBOs.get(new Integer(detail));
		}else{
			GLVBOFloat newVBO = new GLVBOFloat(gl, getHalfSphereCoordinatesArray(detail), "tubeCoords");
			sphereVBOs.put(new Integer(detail), newVBO);
			return newVBO;
		}
	}
	
	public static float[] getSphereCoordinatesArray(int detail){
		return getAngleSphereCoordinatesArray(detail, 2);
	}
	
	public static float[] getHalfSphereCoordinatesArray(int detail){
		return getAngleSphereCoordinatesArray(detail, 1);
	}
	
	//returns the part of a tube or radius 1 from (0,0,0) to (1,0,0) which has z < 0;
	//returns only vertex coordinates
	public static float[] getAngleSphereCoordinatesArray(int detail, int halves){
		int n = detail+2;
		
		//number of floats per section (slice of a melon)
		int m = 4*3*(2*n-2);
		//4 floats per vertex, 3 vertices per triangle, (2*n-2)*n*halves triangles
		float[] sphere = new float[m*n*halves];
		
		double a = Math.PI/n;
		
		for(int j = 0; j < n*halves; j++){
			//first triangle
			sphere[j*m + 0] = 0;
			sphere[j*m + 1] = 1;
			sphere[j*m + 2] = 0;
			sphere[j*m + 3] = 1;
			
			sphere[j*m + 4] = (float)(Math.cos(a*(j))*Math.sin(a));
			sphere[j*m + 5] = (float)Math.cos(a);
			sphere[j*m + 6] = (float)(-Math.sin(a*(j))*Math.sin(a));
			sphere[j*m + 7] = 1;
			
			sphere[j*m + 8] = (float)(Math.cos(a*(j+1))*Math.sin(a));
			sphere[j*m + 9] = (float)Math.cos(a);
			sphere[j*m + 10] = (float)(-Math.sin(a*(j+1))*Math.sin(a));
			sphere[j*m + 11] = 1;
			
			//a quad
			for(int i = 1; i < n-1; i++){
				int offset = j*m+(i-1)*24+12;
				//first triangle of quad
				sphere[offset+0] = (float)(Math.cos(a*(j+1))*Math.sin(a*i));
				sphere[offset+1] = (float)Math.cos(a*i);
				sphere[offset+2] = (float)(-Math.sin(a*(j+1))*Math.sin(a*i));
				sphere[offset+3] = 1;
				
				sphere[offset+4] = (float)(Math.cos(a*(j))*Math.sin(a*i));
				sphere[offset+5] = (float)Math.cos(a*i);
				sphere[offset+6] = (float)(-Math.sin(a*(j))*Math.sin(a*i));
				sphere[offset+7] = 1;
				
				sphere[offset+8] = (float)(Math.cos(a*(j))*Math.sin(a*(i+1)));
				sphere[offset+9] = (float)Math.cos(a*(i+1));
				sphere[offset+10] = (float)(-Math.sin(a*(j))*Math.sin(a*(i+1)));
				sphere[offset+11] = 1;
				//second triangle of quad
				sphere[offset+12] = (float)(Math.cos(a*(j))*Math.sin(a*(i+1)));
				sphere[offset+13] = (float)Math.cos(a*(i+1));
				sphere[offset+14] = (float)(-Math.sin(a*(j))*Math.sin(a*(i+1)));
				sphere[offset+15] = 1;
				
				sphere[offset+16] = (float)(Math.cos(a*(j+1))*Math.sin(a*(i+1)));
				sphere[offset+17] = (float)Math.cos(a*(i+1));
				sphere[offset+18] = (float)(-Math.sin(a*(j+1))*Math.sin(a*(i+1)));
				sphere[offset+19] = 1;
				
				sphere[offset+20] = (float)(Math.cos(a*(j+1))*Math.sin(a*(i)));
				sphere[offset+21] = (float)Math.cos(a*(i));
				sphere[offset+22] = (float)(-Math.sin(a*(j+1))*Math.sin(a*(i)));
				sphere[offset+23] = 1;
			}
			
			//last triangle
			sphere[j*m+(n-2)*24+12] = (float)(Math.cos(a*(j+1))*Math.sin(a*(n-1)));
			sphere[j*m+(n-2)*24+13] = (float)Math.cos(a*(n-1));
			sphere[j*m+(n-2)*24+14] = (float)(-Math.sin(a*(j+1))*Math.sin(a*(n-1)));
			sphere[j*m+(n-2)*24+15] = 1;
			
			sphere[j*m+(n-2)*24+16] = (float)(Math.cos(a*(j))*Math.sin(a*(n-1)));
			sphere[j*m+(n-2)*24+17] = (float)Math.cos(a*(n-1));
			sphere[j*m+(n-2)*24+18] = (float)(-Math.sin(a*(j))*Math.sin(a*(n-1)));
			sphere[j*m+(n-2)*24+19] = 1;
			
			sphere[j*m+(n-2)*24+20] = 0;
			sphere[j*m+(n-2)*24+21] = -1;
			sphere[j*m+(n-2)*24+22] = 0;
			sphere[j*m+(n-2)*24+23] = 1;
		}
		return sphere;
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
