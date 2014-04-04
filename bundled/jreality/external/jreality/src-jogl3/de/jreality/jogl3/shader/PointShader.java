package de.jreality.jogl3.shader;

import java.awt.Color;
import java.util.LinkedList;
import java.util.List;

import javax.media.opengl.GL3;

import de.jreality.jogl.shader.ShadedSphereImage;
import de.jreality.jogl3.JOGLRenderState;
import de.jreality.jogl3.geom.JOGLGeometryInstance.GlUniform;
import de.jreality.jogl3.geom.JOGLPointSetEntity;
import de.jreality.jogl3.glsl.GLShader;
import de.jreality.jogl3.glsl.GLShader.ShaderVar;
import de.jreality.math.Rn;
import de.jreality.scene.Appearance;
import de.jreality.scene.data.AttributeEntityUtility;
import de.jreality.shader.Texture2D;

public class PointShader {
	//private static GLShaderHelper shader;
	private static Texture2D spriteTexture;// = (Texture2D) AttributeEntityUtility.createAttributeEntity(Texture2D.class, "", new Appearance(), true);
	
	public static void init(GL3 gl){
		//gl.glPointSize(50.0f);
		//due to a driver bug we have to call this. In openGL3 POINT_SPRITE
		//mode should be the unchangeable default
		//constant for GL2.GL_POINT_SPRITE
		
		spriteTexture = (Texture2D) AttributeEntityUtility.createAttributeEntity(Texture2D.class, "", new Appearance(), true);
		spriteTexture.setImage(ShadedSphereImage.shadedSphereImage(
				new double[]{1,-1,2},Color.GRAY, Color.WHITE, 60.0, 128, true, null));
		
	}
	public static void render(JOGLPointSetEntity pse, LinkedList<GlUniform> c, GLShader shader, JOGLRenderState state){
		
		GL3 gl = state.getGL();
		//public static void render(GL3 gl, GLVBOFloat vbo, float[] modelview, float[] projection){
		//GLShader shader = GLShader.defaultPointShader;
		Texture2DLoader.load(gl, spriteTexture, gl.GL_TEXTURE2);
		
		//gl.glPointSize(50.0f);
		//due to a driver bug we have to call this. In openGL3 POINT_SPRITE
		//mode should be the unchangeable default
		//constant for GL2.GL_POINT_SPRITE
		gl.glEnable(34913);
		gl.glEnable(gl.GL_PROGRAM_POINT_SIZE);
		gl.glPointParameterf(gl.GL_POINT_SPRITE_COORD_ORIGIN, gl.GL_UPPER_LEFT);
		
		shader.useShader(gl);
		
		//TODO automatic uniforms
		//gl.glUniform1i(gl.glGetUniformLocation(shader.shaderprogram, "pointSize"), pointSize);
        gl.glUniform1i(gl.glGetUniformLocation(shader.shaderprogram, "sys_tex"), 2);
		
		
        float[] projection = Rn.convertDoubleToFloatArray(state.getProjectionMatrix());
		float[] modelview = Rn.convertDoubleToFloatArray(state.getModelViewMatrix());
		
        //matrices
    	gl.glUniformMatrix4fv(gl.glGetUniformLocation(shader.shaderprogram, "projection"), 1, true, projection, 0);
    	gl.glUniformMatrix4fv(gl.glGetUniformLocation(shader.shaderprogram, "modelview"), 1, true, modelview, 0);
    	gl.glUniform1f(gl.glGetUniformLocation(shader.shaderprogram, "screenSize"), state.screenSize);
		gl.glUniform1f(gl.glGetUniformLocation(shader.shaderprogram, "screenSizeInSceneOverScreenSize"), (float)(state.screenSizeInScene/state.screenSize));
		
    	for(GlUniform u : c){
			//System.out.println("point shader render: uniform name is " + u.name);
    		u.bindToShader(shader, gl);
		}
    	
		//bind vbos to corresponding shader variables
    	List<ShaderVar> l = shader.vertexAttributes;
    	for(ShaderVar v : l){
    		GLVBO vbo = pse.getPointVBO(v.getName());
    		if(vbo != null){
    			gl.glUniform1i(gl.glGetUniformLocation(shader.shaderprogram, "has_" + v.getName()), 1);
    			gl.glBindBuffer(gl.GL_ARRAY_BUFFER, vbo.getID());
            	gl.glVertexAttribPointer(gl.glGetAttribLocation(shader.shaderprogram, v.getName()), vbo.getElementSize(), vbo.getType(), false, 0, 0);
            	gl.glEnableVertexAttribArray(gl.glGetAttribLocation(shader.shaderprogram, v.getName()));
    		}else{
    			gl.glUniform1i(gl.glGetUniformLocation(shader.shaderprogram, "has_" + v.getName()), 0);
    		}
    	}
		
		if(pse.getPointVBO("vertex_coordinates") != null)
			gl.glDrawArrays(gl.GL_POINTS, 0, pse.getPointVBO("vertex_coordinates").getLength()/4);
		
		//disable all vbos
    	for(ShaderVar v : l){
    		GLVBO vbo = pse.getPointVBO(v.getName());
    		if(vbo != null){
    			gl.glDisableVertexAttribArray(gl.glGetAttribLocation(shader.shaderprogram, v.getName()));
    		}
    	}
		
		shader.dontUseShader(gl);
        
	}
}
