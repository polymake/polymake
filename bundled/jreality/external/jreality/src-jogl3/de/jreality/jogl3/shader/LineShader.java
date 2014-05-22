package de.jreality.jogl3.shader;

import java.util.LinkedList;
import java.util.List;

import javax.media.opengl.GL3;

import de.jreality.jogl3.JOGLRenderState;
import de.jreality.jogl3.geom.JOGLGeometryInstance.GlUniform;
import de.jreality.jogl3.geom.JOGLLineSetEntity;
import de.jreality.jogl3.glsl.GLShader;
import de.jreality.jogl3.glsl.GLShader.ShaderVar;
import de.jreality.jogl3.helper.TransparencyHelper;
import de.jreality.math.Rn;

public class LineShader{
	
	public static void render(JOGLLineSetEntity lse, LinkedList<GlUniform> c, GLShader shader, JOGLRenderState state, float lineWidth){
		//System.out.println("LineShader.render()");
		
		GL3 gl = state.getGL();
		float[] projection = Rn.convertDoubleToFloatArray(state.getProjectionMatrix());
		float[] modelview = Rn.convertDoubleToFloatArray(state.getModelViewMatrix());
			shader.useShader(gl);
			
        	//matrices
        	gl.glUniformMatrix4fv(gl.glGetUniformLocation(shader.shaderprogram, "projection"), 1, true, projection, 0);
        	gl.glUniformMatrix4fv(gl.glGetUniformLocation(shader.shaderprogram, "modelview"), 1, true, modelview, 0);
        	
			//width
        	gl.glLineWidth(TransparencyHelper.getSupersample()*lineWidth);
			//bind shader uniforms
			for(GlUniform u : c){
				u.bindToShader(shader, gl);
			}
			//TODO all the other types
			
        	//bind vbos to corresponding shader variables
        	List<ShaderVar> l = shader.vertexAttributes;
        	for(ShaderVar v : l){
        		GLVBO vbo = lse.getLineVBO(v.getName());
        		if(vbo != null){
        			gl.glUniform1i(gl.glGetUniformLocation(shader.shaderprogram, "has_" + v.getName()), 1);
        			gl.glBindBuffer(gl.GL_ARRAY_BUFFER, vbo.getID());
                	gl.glVertexAttribPointer(gl.glGetAttribLocation(shader.shaderprogram, v.getName()), vbo.getElementSize(), vbo.getType(), false, 0, 0);
                	gl.glEnableVertexAttribArray(gl.glGetAttribLocation(shader.shaderprogram, v.getName()));
        		}else{
        			gl.glUniform1i(gl.glGetUniformLocation(shader.shaderprogram, "has_" + v.getName()), 0);
        		}
        	}
        	
        	//actual draw command
        	//TODO is lse.getLineVBO("vertex_coordinates").getLength()/4 maybe enough?
        	gl.glDrawArrays(gl.GL_LINES, 0, lse.getLineVBO("vertex_coordinates").getLength()/4);
        	
        	//disable all vbos
        	for(ShaderVar v : l){
        		GLVBO vbo = lse.getLineVBO(v.getName());
        		if(vbo != null){
        			gl.glDisableVertexAttribArray(gl.glGetAttribLocation(shader.shaderprogram, v.getName()));
        		}
        	}
			shader.dontUseShader(gl);
	}
}
