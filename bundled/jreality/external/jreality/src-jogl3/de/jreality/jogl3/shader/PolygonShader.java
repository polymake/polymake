package de.jreality.jogl3.shader;

import java.util.LinkedList;
import java.util.List;

import javax.media.opengl.GL3;

import de.jreality.jogl3.JOGLRenderState;
import de.jreality.jogl3.geom.JOGLFaceSetEntity;
import de.jreality.jogl3.GlTexture;
import de.jreality.jogl3.geom.JOGLGeometryInstance.GlUniform;
import de.jreality.jogl3.glsl.GLShader;
import de.jreality.jogl3.glsl.GLShader.ShaderVar;
import de.jreality.math.Rn;
import de.jreality.shader.EffectiveAppearance;

public class PolygonShader{
	
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
	
	public static void render(JOGLFaceSetEntity fse, LinkedList<GlUniform> c, GlTexture tex, de.jreality.jogl3.geom.GlReflectionMap reflMap, GLShader shader, JOGLRenderState state){

		
		//TODO replace by fsi or renderState
		//GLShader shader = GLShader.defaultPolygonShader;
		GL3 gl = state.getGL();
		
		state.getLightHelper().loadLocalLightTexture(state.getLocalLightCollection(), gl);
		
		float[] projection = Rn.convertDoubleToFloatArray(state.getProjectionMatrix());
		float[] modelview = Rn.convertDoubleToFloatArray(state.getModelViewMatrix());
		float[] inverseCamMatrix = Rn.convertDoubleToFloatArray(state.inverseCamMatrix);
		shader.useShader(gl);
		
    	//matrices
		ShaderVarHash.bindUniformMatrix(shader, "projection", projection, gl);
		ShaderVarHash.bindUniformMatrix(shader, "modelview", modelview, gl);
		ShaderVarHash.bindUniformMatrix(shader, "_inverseCamRotation", inverseCamMatrix, gl);
//		gl.glUniformMatrix4fv(gl.glGetUniformLocation(shader.shaderprogram, "projection"), 1, true, projection, 0);
//    	gl.glUniformMatrix4fv(gl.glGetUniformLocation(shader.shaderprogram, "modelview"), 1, true, modelview, 0);
//    	gl.glUniformMatrix4fv(gl.glGetUniformLocation(shader.shaderprogram, "_inverseCamRotation"), 1, true, inverseCamMatrix, 0);
    	
		//global lights in a texture
    	ShaderVarHash.bindUniform(shader, "sys_globalLights", 0, gl);
    	ShaderVarHash.bindUniform(shader, "sys_numGlobalDirLights", state.getLightHelper().getNumGlobalDirLights(), gl);
    	ShaderVarHash.bindUniform(shader, "sys_numGlobalPointLights", state.getLightHelper().getNumGlobalPointLights(), gl);
    	ShaderVarHash.bindUniform(shader, "sys_numGlobalSpotLights", state.getLightHelper().getNumGlobalSpotLights(), gl);
//		gl.glUniform1i(gl.glGetUniformLocation(shader.shaderprogram, "sys_globalLights"), 0);
//		gl.glUniform1i(gl.glGetUniformLocation(shader.shaderprogram, "sys_numGlobalDirLights"), state.getLightHelper().getNumGlobalDirLights());
//		gl.glUniform1i(gl.glGetUniformLocation(shader.shaderprogram, "sys_numGlobalPointLights"), state.getLightHelper().getNumGlobalPointLights());
//		gl.glUniform1i(gl.glGetUniformLocation(shader.shaderprogram, "sys_numGlobalSpotLights"), state.getLightHelper().getNumGlobalSpotLights());
		
		//local lights in a texture
		ShaderVarHash.bindUniform(shader, "sys_localLights", 1, gl);
		ShaderVarHash.bindUniform(shader, "sys_numLocalDirLights", state.getLightHelper().getNumLocalDirLights(), gl);
		ShaderVarHash.bindUniform(shader, "sys_numLocalPointLights", state.getLightHelper().getNumLocalPointLights(), gl);
		ShaderVarHash.bindUniform(shader, "sys_numLocalSpotLights", state.getLightHelper().getNumLocalSpotLights(), gl);
//		gl.glUniform1i(gl.glGetUniformLocation(shader.shaderprogram, "sys_localLights"), 1);
//		gl.glUniform1i(gl.glGetUniformLocation(shader.shaderprogram, "sys_numLocalDirLights"), state.getLightHelper().getNumLocalDirLights());
//		gl.glUniform1i(gl.glGetUniformLocation(shader.shaderprogram, "sys_numLocalPointLights"), state.getLightHelper().getNumLocalPointLights());
//		gl.glUniform1i(gl.glGetUniformLocation(shader.shaderprogram, "sys_numLocalSpotLights"), state.getLightHelper().getNumLocalSpotLights());
		
		
		//bind shader uniforms
		//TODOhave to set default values here for shader uniforms not present in the appearance
//		System.err.println("uniforms for " + fse.getNode().getName());
		for(GlUniform u : c){
//			System.err.println(u.name);
			u.bindToShader(shader, gl);
		}

		tex.bind(shader, gl);
		reflMap.bind(shader, gl);
		
    	//bind vbos to corresponding shader variables
    	List<ShaderVar> l = shader.vertexAttributes;
    	for(ShaderVar v : l){
    		GLVBO vbo = fse.getVBO(v.getName());
    		if(vbo != null){
    			//System.out.println(v.getName());
    			ShaderVarHash.bindUniform(shader, "has_" + v.getName(), 1, gl);
//    			gl.glUniform1i(gl.glGetUniformLocation(shader.shaderprogram, "has_" + v.getName()), 1);
    			gl.glBindBuffer(gl.GL_ARRAY_BUFFER, vbo.getID());
            	gl.glVertexAttribPointer(gl.glGetAttribLocation(shader.shaderprogram, v.getName()), vbo.getElementSize(), vbo.getType(), false, 0, 0);
            	gl.glEnableVertexAttribArray(gl.glGetAttribLocation(shader.shaderprogram, v.getName()));
    		}else{
    			ShaderVarHash.bindUniform(shader, "has_" + v.getName(), 0, gl);
//    			gl.glUniform1i(gl.glGetUniformLocation(shader.shaderprogram, "has_" + v.getName()), 0);
    		}
    	}

    	//new way to do lights
		state.getLightHelper().bindGlobalLightTexture(gl);
		
    	//actual draw command
    	gl.glDrawArrays(gl.GL_TRIANGLES, 0, fse.getVBO("vertex_coordinates").getLength()/4);
		
    	//disable all vbos
    	for(ShaderVar v : l){
    		GLVBO vbo = fse.getVBO(v.getName());
    		if(vbo != null){
    			gl.glDisableVertexAttribArray(gl.glGetAttribLocation(shader.shaderprogram, v.getName()));
    		}
    	}
		shader.dontUseShader(gl);
	}
	
	public static void renderDepth(JOGLFaceSetEntity fse, GLShader shader, JOGLRenderState state, int width, int height) {
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
    	GLVBO vbo = fse.getVBO("vertex_coordinates");
    	gl.glBindBuffer(gl.GL_ARRAY_BUFFER, vbo.getID());
    	gl.glVertexAttribPointer(gl.glGetAttribLocation(shader.shaderprogram, vbo.getName()), vbo.getElementSize(), vbo.getType(), false, 0, 0);
    	gl.glEnableVertexAttribArray(gl.glGetAttribLocation(shader.shaderprogram, vbo.getName()));
    	//actual draw command
    	gl.glDrawArrays(gl.GL_TRIANGLES, 0, fse.getVBO("vertex_coordinates").getLength()/4);
    	
    	gl.glDisableVertexAttribArray(gl.glGetAttribLocation(shader.shaderprogram, vbo.getName()));
    	
    	shader.dontUseShader(gl);
	}

	public static void addOneLayer(JOGLFaceSetEntity fse, LinkedList<GlUniform> c, GlTexture tex, de.jreality.jogl3.geom.GlReflectionMap reflMap, GLShader shader, JOGLRenderState state, int width, int height, float transparency) {
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

		tex.bind(shader, gl);
		reflMap.bind(shader, gl);
		
    	//bind vbos to corresponding shader variables
    	List<ShaderVar> l = shader.vertexAttributes;
    	for(ShaderVar v : l){
    		GLVBO vbo = fse.getVBO(v.getName());
    		if(vbo != null){
    			//System.out.println(v.getName());
    			gl.glUniform1i(gl.glGetUniformLocation(shader.shaderprogram, "has_" + v.getName()), 1);
    			gl.glBindBuffer(gl.GL_ARRAY_BUFFER, vbo.getID());
            	gl.glVertexAttribPointer(gl.glGetAttribLocation(shader.shaderprogram, v.getName()), vbo.getElementSize(), vbo.getType(), false, 0, 0);
            	gl.glEnableVertexAttribArray(gl.glGetAttribLocation(shader.shaderprogram, v.getName()));
    		}else{
    			gl.glUniform1i(gl.glGetUniformLocation(shader.shaderprogram, "has_" + v.getName()), 0);
    		}
    	}

    	//new way to do lights
		state.getLightHelper().bindGlobalLightTexture(gl);
		
    	//actual draw command
    	gl.glDrawArrays(gl.GL_TRIANGLES, 0, fse.getVBO("vertex_coordinates").getLength()/4);
    	
    	//disable all vbos
    	for(ShaderVar v : l){
    		GLVBO vbo = fse.getVBO(v.getName());
    		if(vbo != null){
    			gl.glDisableVertexAttribArray(gl.glGetAttribLocation(shader.shaderprogram, v.getName()));
    		}
    	}
		shader.dontUseShader(gl);
	}
}
