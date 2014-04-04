package de.jreality.jogl3;

import java.nio.FloatBuffer;

import javax.media.opengl.GL3;

public class GLvboNormals {
	
	private float[] normaldata = null;
	private int[] index = new int[1];
	
	
	public int getID(){
		return index[0];
	}
	
	public int getLength(){
		return normaldata.length;
	}
	
	GLvboNormals(float[] vertdata){
		if(vertdata == null)
			System.out.println("Error: no vertex data specified");
		this.normaldata = vertdata;
	}
	
	
	public void load(GL3 gl){
		gl.glGenBuffers(1, index, 0);
		gl.glBindBuffer(gl.GL_ARRAY_BUFFER, index[0]);
		gl.glBufferData(gl.GL_ARRAY_BUFFER, 4*normaldata.length, FloatBuffer.wrap(normaldata), gl.GL_DYNAMIC_READ);
	}
}
