package de.jreality.jogl3.geom;

import java.util.HashMap;
import java.util.Set;

import javax.media.opengl.GL3;

import de.jreality.jogl3.shader.GLVBO;
import de.jreality.scene.Sphere;
import de.jreality.scene.event.GeometryEvent;

public class JOGLSphereEntity extends JOGLGeometryEntity {

	//private GLVBOFloat normalVBO = null;
	private HashMap<String, GLVBO> vbos = new HashMap<String, GLVBO>();
	
	public int getNumVBOs(){
		return vbos.size();
	}
	public GLVBO getVBO(String s){
		return vbos.get(s);
	}
	public GLVBO[] getAllVBOs(){
		GLVBO[] ret = new GLVBO[vbos.size()];
		Set<String> keys = vbos.keySet();
		int i = 0;
		for(String s : keys){
			ret[i] = vbos.get(s);
			i++;
		}
		return ret;
	}
	
	public JOGLSphereEntity(Sphere node) {
		super(node);
	}
	
	//replace state to gl
	public boolean updateData(GL3 gl) {
		return false;
	}
	
	@Override
	public void geometryChanged(GeometryEvent ev) {
		// TODO Auto-generated method stub
		
	}
}
