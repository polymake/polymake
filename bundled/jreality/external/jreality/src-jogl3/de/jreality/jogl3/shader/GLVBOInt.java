package de.jreality.jogl3.shader;

import java.nio.FloatBuffer;
import java.nio.IntBuffer;

import javax.media.opengl.GL3;

public class GLVBOInt extends GLVBO{
	
	//TODO data array only for debugging I believe
	private int[] data;
	
	public int[] getData(){
		return data;
	}
	/**
	 * 
	 * @param gl
	 * @param subdata
	 * @param begin
	 * @param length The length in Floats.
	 */
	public void updateSubData(GL3 gl, int[] subdata, int begin, int length){
		System.arraycopy(subdata, 0, this.data, begin, length);
		gl.glBindBuffer(gl.GL_ARRAY_BUFFER, index);
//		System.out.println("begin = " + begin);
		gl.glBufferSubData(gl.GL_ARRAY_BUFFER, 4*begin, 4*length, IntBuffer.wrap(subdata));
	}
	public void updateData(GL3 gl, int[] data){
		System.arraycopy(data, 0, this.data, 0, data.length);
		gl.glBindBuffer(gl.GL_ARRAY_BUFFER, index);
		gl.glBufferData(gl.GL_ARRAY_BUFFER, 4*data.length, IntBuffer.wrap(data), gl.GL_STATIC_READ);
	}
	public GLVBOInt(GL3 gl, int[] vertdata, String name, int arraySize){
		this(gl, vertdata, name);
		this.arraySize = arraySize;
	}
	public GLVBOInt(GL3 gl, int[] vertdata, String name){
		data = new int[vertdata.length];
		System.arraycopy(vertdata, 0, data, 0, vertdata.length);
		this.name = name;
		int[] vertindex = new int[1];
		gl.glGenBuffers(1, vertindex, 0);
		gl.glBindBuffer(gl.GL_ARRAY_BUFFER, vertindex[0]);
		gl.glBufferData(gl.GL_ARRAY_BUFFER, 4*vertdata.length, IntBuffer.wrap(vertdata), gl.GL_STATIC_DRAW);
		index = vertindex[0];
		length = vertdata.length;
	}

	@Override
	public int getType() {
		// TODO Auto-generated method stub
		return GL3.GL_INT;
	}
}
