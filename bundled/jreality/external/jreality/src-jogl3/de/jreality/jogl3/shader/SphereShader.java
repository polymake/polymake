package de.jreality.jogl3.shader;

import java.util.LinkedList;

import javax.media.opengl.GL3;

import de.jreality.jogl3.JOGLRenderState;
import de.jreality.jogl3.GlTexture;
import de.jreality.jogl3.geom.GlReflectionMap;
import de.jreality.jogl3.geom.JOGLGeometryInstance.GlUniform;
import de.jreality.jogl3.geom.JOGLSphereEntity;
import de.jreality.jogl3.glsl.GLShader;
import de.jreality.math.Rn;
import de.jreality.shader.EffectiveAppearance;

public class SphereShader{
	
	public static void printEffectiveAppearance(EffectiveAppearance eap, String name) {
		System.out.println("start");
		//System.out.println(((IndexedFaceSet)(fse.getNode())).getName());
		
		//eap.getApp().getAttributes().keySet()
		for( Object o : eap.getApp().getAttributes().keySet()){
			String s = (String)o;
			eap.getApp().getAttribute(s);
			System.out.println(s + " " + eap.getApp().getAttribute(s).getClass());
		}
		System.out.println("stop");
	}
	
	public static void addOneLayer(JOGLSphereEntity se, LinkedList<GlUniform> c, GlTexture tex, GlReflectionMap reflMap, GLShader shader, JOGLRenderState state, int width, int height, float transparency){
		System.out.println("add one layer of sphere");
		GL3 gl = state.getGL();
		
		state.getLightHelper().loadLocalLightTexture(state.getLocalLightCollection(), gl);
		
		float[] projection = Rn.convertDoubleToFloatArray(state.getProjectionMatrix());
		float[] modelview = Rn.convertDoubleToFloatArray(state.getModelViewMatrix());
		float[] inverseCamMatrix = Rn.convertDoubleToFloatArray(state.inverseCamMatrix);
		
		shader.useShader(gl);
		
		//transparency related uniforms
		gl.glUniform1i(gl.glGetUniformLocation(shader.shaderprogram, "_depth"), 9);
		gl.glUniform1i(gl.glGetUniformLocation(shader.shaderprogram, "_width"), width);
		gl.glUniform1i(gl.glGetUniformLocation(shader.shaderprogram, "_height"), height);
		gl.glUniform1f(gl.glGetUniformLocation(shader.shaderprogram, "transparency"), transparency);
		
    	//matrices
    	gl.glUniformMatrix4fv(gl.glGetUniformLocation(shader.shaderprogram, "projection"), 1, true, projection, 0);
    	gl.glUniformMatrix4fv(gl.glGetUniformLocation(shader.shaderprogram, "modelview"), 1, true, modelview, 0);
    	gl.glUniformMatrix4fv(gl.glGetUniformLocation(shader.shaderprogram, "_inverseCamRotation"), 1, true, inverseCamMatrix, 0);
    	
		//global lights in a texture
		gl.glUniform1i(gl.glGetUniformLocation(shader.shaderprogram, "sys_globalLights"), 0);
		gl.glUniform1i(gl.glGetUniformLocation(shader.shaderprogram, "sys_numGlobalDirLights"), state.getLightHelper().getNumGlobalDirLights());
		gl.glUniform1i(gl.glGetUniformLocation(shader.shaderprogram, "sys_numGlobalPointLights"), state.getLightHelper().getNumGlobalPointLights());
		gl.glUniform1i(gl.glGetUniformLocation(shader.shaderprogram, "sys_numGlobalSpotLights"), state.getLightHelper().getNumGlobalSpotLights());
		
		//local lights in a texture
		gl.glUniform1i(gl.glGetUniformLocation(shader.shaderprogram, "sys_localLights"), 1);
		gl.glUniform1i(gl.glGetUniformLocation(shader.shaderprogram, "sys_numLocalDirLights"), state.getLightHelper().getNumLocalDirLights());
		gl.glUniform1i(gl.glGetUniformLocation(shader.shaderprogram, "sys_numLocalPointLights"), state.getLightHelper().getNumLocalPointLights());
		gl.glUniform1i(gl.glGetUniformLocation(shader.shaderprogram, "sys_numLocalSpotLights"), state.getLightHelper().getNumLocalSpotLights());
		
		
		//bind shader uniforms
		//TODOhave to set default values here for shader uniforms not present in the appearance
		for(GlUniform u : c){
			u.bindToShader(shader, gl);
		}
		
		reflMap.bind(shader, gl);
		
		GLVBO sphereVBO = state.getSphereHelper().getSphereVBO(gl, 20);
		gl.glBindBuffer(gl.GL_ARRAY_BUFFER, sphereVBO.getID());
		gl.glVertexAttribPointer(gl.glGetAttribLocation(shader.shaderprogram, "vertex_coordinates"), sphereVBO.getElementSize(), sphereVBO.getType(), false, 0, 0);
    	gl.glEnableVertexAttribArray(gl.glGetAttribLocation(shader.shaderprogram, "vertex_coordinates"));
    	
    	//new way to do lights
		state.getLightHelper().bindGlobalLightTexture(gl);
		
    	//actual draw command
    	gl.glDrawArrays(gl.GL_TRIANGLES, 0, sphereVBO.getLength()/4);
    	
    	//disable all vbos
    	gl.glDisableVertexAttribArray(gl.glGetAttribLocation(shader.shaderprogram, "vertex_coordinates"));
    	
		shader.dontUseShader(gl);
	}
	
	public static void renderDepth(JOGLSphereEntity se, GLShader shader, JOGLRenderState state, int width, int height) {
		System.out.println("rendering depth");
		GL3 gl = state.getGL();
		
		float[] projection = Rn.convertDoubleToFloatArray(state.getProjectionMatrix());
		float[] modelview = Rn.convertDoubleToFloatArray(state.getModelViewMatrix());

		shader.useShader(gl);
		
    	//matrices
    	gl.glUniformMatrix4fv(gl.glGetUniformLocation(shader.shaderprogram, "projection"), 1, true, projection, 0);
    	gl.glUniformMatrix4fv(gl.glGetUniformLocation(shader.shaderprogram, "modelview"), 1, true, modelview, 0);
    	gl.glUniform1i(gl.glGetUniformLocation(shader.shaderprogram, "image"), 0);
    	gl.glUniform1i(gl.glGetUniformLocation(shader.shaderprogram, "width"), width);
    	gl.glUniform1i(gl.glGetUniformLocation(shader.shaderprogram, "height"), height);
		
    	
    	//bind vbo to corresponding shader variables
    	GLVBO sphereVBO = state.getSphereHelper().getSphereVBO(gl, 20);
		gl.glBindBuffer(gl.GL_ARRAY_BUFFER, sphereVBO.getID());
		gl.glVertexAttribPointer(gl.glGetAttribLocation(shader.shaderprogram, "vertex_coordinates"), sphereVBO.getElementSize(), sphereVBO.getType(), false, 0, 0);
    	gl.glEnableVertexAttribArray(gl.glGetAttribLocation(shader.shaderprogram, "vertex_coordinates"));
    	
    	//actual draw command
    	gl.glDrawArrays(gl.GL_TRIANGLES, 0, sphereVBO.getLength()/4);
    	
    	//disable all vbos
    	gl.glDisableVertexAttribArray(gl.glGetAttribLocation(shader.shaderprogram, "vertex_coordinates"));
    	
    	shader.dontUseShader(gl);
	}
	public static void render(JOGLSphereEntity se, LinkedList<GlUniform> c, GlTexture tex, GlReflectionMap reflMap, GLShader shader, JOGLRenderState state){
		
		//GLShader shader = GLShader.defaultPolygonShader;
		GL3 gl = state.getGL();
		
		state.getLightHelper().loadLocalLightTexture(state.getLocalLightCollection(), gl);
		
		float[] projection = Rn.convertDoubleToFloatArray(state.getProjectionMatrix());
		float[] modelview = Rn.convertDoubleToFloatArray(state.getModelViewMatrix());
		float[] inverseCamMatrix = Rn.convertDoubleToFloatArray(state.inverseCamMatrix);
		shader.useShader(gl);
		
    	//matrices
    	gl.glUniformMatrix4fv(gl.glGetUniformLocation(shader.shaderprogram, "projection"), 1, true, projection, 0);
    	gl.glUniformMatrix4fv(gl.glGetUniformLocation(shader.shaderprogram, "modelview"), 1, true, modelview, 0);
    	gl.glUniformMatrix4fv(gl.glGetUniformLocation(shader.shaderprogram, "_inverseCamRotation"), 1, true, inverseCamMatrix, 0);
    	
		//global lights in a texture
		gl.glUniform1i(gl.glGetUniformLocation(shader.shaderprogram, "sys_globalLights"), 0);
		gl.glUniform1i(gl.glGetUniformLocation(shader.shaderprogram, "sys_numGlobalDirLights"), state.getLightHelper().getNumGlobalDirLights());
		gl.glUniform1i(gl.glGetUniformLocation(shader.shaderprogram, "sys_numGlobalPointLights"), state.getLightHelper().getNumGlobalPointLights());
		gl.glUniform1i(gl.glGetUniformLocation(shader.shaderprogram, "sys_numGlobalSpotLights"), state.getLightHelper().getNumGlobalSpotLights());
		
		//local lights in a texture
		gl.glUniform1i(gl.glGetUniformLocation(shader.shaderprogram, "sys_localLights"), 1);
		gl.glUniform1i(gl.glGetUniformLocation(shader.shaderprogram, "sys_numLocalDirLights"), state.getLightHelper().getNumLocalDirLights());
		gl.glUniform1i(gl.glGetUniformLocation(shader.shaderprogram, "sys_numLocalPointLights"), state.getLightHelper().getNumLocalPointLights());
		gl.glUniform1i(gl.glGetUniformLocation(shader.shaderprogram, "sys_numLocalSpotLights"), state.getLightHelper().getNumLocalSpotLights());
		
		
		//bind shader uniforms
		//TODOhave to set default values here for shader uniforms not present in the appearance
		for(GlUniform u : c){
			u.bindToShader(shader, gl);
		}
		
		reflMap.bind(shader, gl);
		
		GLVBO sphereVBO = state.getSphereHelper().getSphereVBO(gl, 20);
		gl.glBindBuffer(gl.GL_ARRAY_BUFFER, sphereVBO.getID());
		gl.glVertexAttribPointer(gl.glGetAttribLocation(shader.shaderprogram, "vertex_coordinates"), sphereVBO.getElementSize(), sphereVBO.getType(), false, 0, 0);
    	gl.glEnableVertexAttribArray(gl.glGetAttribLocation(shader.shaderprogram, "vertex_coordinates"));
    	
    	//new way to do lights
		state.getLightHelper().bindGlobalLightTexture(gl);
		
    	//actual draw command
    	gl.glDrawArrays(gl.GL_TRIANGLES, 0, sphereVBO.getLength()/4);
    	
    	//disable all vbos
    	gl.glDisableVertexAttribArray(gl.glGetAttribLocation(shader.shaderprogram, "vertex_coordinates"));
    	
		shader.dontUseShader(gl);
	}
}
