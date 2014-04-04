package de.jreality.jogl3;

import java.nio.FloatBuffer;

import javax.media.opengl.GL3;

public class GLvboVertex {
	
	private float[] vertdata = null;
	private int[] vertindex = new int[1];
	
	
	public int getID(){
		return vertindex[0];
	}
	
	public int getLength(){
		return vertdata.length;
	}
	
	GLvboVertex(float[] vertdata){
		if(vertdata == null)
			System.out.println("Error: no vertex data specified");
		this.vertdata = vertdata;
	}
	
	
	public void load(GL3 gl){
		gl.glGenBuffers(1, vertindex, 0);
		gl.glBindBuffer(gl.GL_ARRAY_BUFFER, vertindex[0]);
		gl.glBufferData(gl.GL_ARRAY_BUFFER, 4*vertdata.length, FloatBuffer.wrap(vertdata), gl.GL_DYNAMIC_READ);
	}
	
	
}
