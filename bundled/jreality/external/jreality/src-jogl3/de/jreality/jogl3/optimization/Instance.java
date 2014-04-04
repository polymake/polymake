package de.jreality.jogl3.optimization;

import de.jreality.jogl3.JOGLRenderState;
import de.jreality.jogl3.geom.JOGLFaceSetEntity;
import de.jreality.jogl3.geom.JOGLFaceSetInstance;
import de.jreality.jogl3.shader.GLVBO;

public class Instance {
	
	public Instance(InstanceCollection ic, JOGLFaceSetInstance ins, JOGLRenderState state, int posInVBOs){
		this.collection = ic;
		this.fsi = ins;
		this.state = state;
		this.posInVBOs = posInVBOs;
		dead = false;
		JOGLFaceSetEntity fse = (JOGLFaceSetEntity)fsi.getEntity();
		if(fse == null){
			System.err.println("FSI has no entity");
		}
		if(fse.getVBO("vertex_coordinates") == null){
			System.err.println("FSE has no vertex coordinates vbo, but");
		}
		GLVBO[] vbos = fse.getAllVBOs();
//		System.out.println("FSE contains:");
//		for(GLVBO vbo : vbos){
//			System.out.println(vbo.getName());
//		}
		
		length = fse.getVBO("vertex_coordinates").getLength();
//		System.out.println("instance length = " + length);
//		System.out.println("length = " + length);
	}
	
	public void kill(){
		dead = true;
		state = null;
	}
	
	public boolean isDead(){
		return dead;
	}
	
	public boolean isAlive(){
		return !dead;
	}
	
	public InstanceCollection collection;
	public JOGLFaceSetInstance fsi;
	public JOGLRenderState state;
	/**
	 * number of floats or integers
	 */
	public int length;
	/**
	 * position in VBO in floats
	 */
	public int posInVBOs;
	/**
	 * dead actually means, that it needs to be nulled in GPU, if it's not removed by defragmentation or merging
	 */
	private boolean dead = false;
	public boolean posUpToDate = true;
	public boolean appChanged = true;
	public int id = 0;
}
