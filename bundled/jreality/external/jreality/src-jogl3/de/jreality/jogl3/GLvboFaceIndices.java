package de.jreality.jogl3;

import java.nio.IntBuffer;

import javax.media.opengl.GL3;

public class GLvboFaceIndices {
	
	private int[] indices = null;
	private int[] index = new int[1];
	
	public int getID(){
		return index[0];
	}
	
	public int getLength(){
		return indices.length;
	}
	
	GLvboFaceIndices(int[] indices){
		if(indices == null)
			System.out.println("Error: no vertex data specified");
		this.indices = indices;
	}
	
	
	public void load(GL3 gl){
		gl.glGenBuffers(1, index, 0);
		gl.glBindBuffer(gl.GL_ELEMENT_ARRAY_BUFFER, index[0]);
		gl.glBufferData(gl.GL_ELEMENT_ARRAY_BUFFER, 4*indices.length, IntBuffer.wrap(indices), gl.GL_DYNAMIC_READ);
	}
}
