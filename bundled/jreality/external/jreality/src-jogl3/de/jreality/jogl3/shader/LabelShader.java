package de.jreality.jogl3.shader;

import java.awt.Color;
import java.awt.Font;
import java.awt.image.BufferedImage;
import javax.media.opengl.GL3;

import de.jreality.backends.label.LabelUtility;
import de.jreality.jogl3.JOGLRenderState;
import de.jreality.jogl3.geom.JOGLGeometryInstance.LabelRenderData;
import de.jreality.jogl3.geom.Label;
import de.jreality.jogl3.glsl.GLShader;
import de.jreality.math.Rn;
import de.jreality.scene.Appearance;
import de.jreality.scene.data.AttributeEntityUtility;
import de.jreality.shader.ImageData;
import de.jreality.shader.Texture2D;

public class LabelShader {

	private static GLShader overlayShader = new GLShader("overlay.v", "overlay.f");
	private static GLVBOFloat vbo;
	private static GLVBOFloat vbo2;
	public static void init(GL3 gl){
		vbo = new GLVBOFloat(gl, new float[]{1, -1, 0f, 1,
				1, 0, 0f, 1,
				0, 0, 0f, 1,
				0, 0, 0f, 1,
				0, -1, 0f, 1,
				1, -1, 0f, 1}, "vertices");
		vbo2 = new GLVBOFloat(gl, new float[]{1, 1, 0f, 1,
				1, 0, 0f, 1,
				0, 0, 0f, 1,
				0, 0, 0f, 1,
				0, 1, 0f, 1,
				1, 1, 0f, 1}, "vertices");
		overlayShader.init(gl);
		shader.init(gl);
		shaderDepth.init(gl);
		shaderTransp.init(gl);
	}
	public static Texture2D tex = (Texture2D) AttributeEntityUtility.createAttributeEntity(Texture2D.class, "", new Appearance(), true);
	public static BufferedImage buf;
	public static ImageData img;
	public static void renderOverlay(String text, GL3 gl){
		
		
		
		buf = LabelUtility.createImageFromString(text, new Font("Arial", Font.PLAIN, 30), Color.BLACK, Color.WHITE);
		img = new ImageData(buf);
		
		tex.setImage(img);
		Texture2DLoader.load(gl, tex, gl.GL_TEXTURE2);
		
		overlayShader.useShader(gl);
		
		gl.glEnable(gl.GL_BLEND);
		gl.glBlendFuncSeparate(gl.GL_SRC_ALPHA, gl.GL_ONE_MINUS_SRC_ALPHA,
				gl.GL_ONE, gl.GL_ONE_MINUS_SRC_ALPHA);
		
		
		gl.glUniform1i(gl.glGetUniformLocation(overlayShader.shaderprogram, "tex"), 2);
		
		
		
		
		gl.glBindBuffer(gl.GL_ARRAY_BUFFER, vbo.getID());
    	gl.glVertexAttribPointer(gl.glGetAttribLocation(overlayShader.shaderprogram, vbo.getName()), vbo.getElementSize(), vbo.getType(), false, 0, 0);
    	gl.glEnableVertexAttribArray(gl.glGetAttribLocation(overlayShader.shaderprogram, vbo.getName()));
		
    	
    	
    	//actual draw command
    	gl.glDrawArrays(gl.GL_TRIANGLES, 0, vbo.getLength()/4);
    	
		
    	gl.glDisableVertexAttribArray(gl.glGetAttribLocation(overlayShader.shaderprogram, vbo.getName()));
    	
		
    	overlayShader.dontUseShader(gl);
	}
	
	private static GLShader shader = new GLShader("label.v", "label.f");
	private static GLShader shaderDepth = new GLShader("transp/labelDepth.v", "transp/labelDepth.f");
	private static GLShader shaderTransp = new GLShader("label.v", "transp/labelTransp.f");
	public static void render(LabelRenderData labelData, Label[] labels, JOGLRenderState state){
		if(labels == null || labels.length == 0)
			return;
		GL3 gl = state.getGL();
		
		float[] projection = Rn.convertDoubleToFloatArray(state.getProjectionMatrix());
		float[] modelview = Rn.convertDoubleToFloatArray(state.getModelViewMatrix());
		
		
		
		
		
		for(int L = 0; L < labelData.tex.length; L++){
		
			Texture2DLoader.load(gl, labelData.tex[L], gl.GL_TEXTURE2);
			
			shader.useShader(gl);
			
			gl.glEnable(gl.GL_BLEND);
			gl.glBlendFuncSeparate(gl.GL_SRC_ALPHA, gl.GL_ONE_MINUS_SRC_ALPHA,
					gl.GL_ONE, gl.GL_ONE_MINUS_SRC_ALPHA);
			
			
			gl.glUniform4fv(gl.glGetUniformLocation(shader.shaderprogram, "xyAlignmentTotalWH"), 1, labelData.xyAlignmentTotalWH[L], 0);
			gl.glUniform4fv(gl.glGetUniformLocation(shader.shaderprogram, "xyzOffsetScale"), 1, labelData.xyzOffsetScale, 0);
			
			gl.glUniform1i(gl.glGetUniformLocation(shader.shaderprogram, "tex"), 2);
			ShaderVarHash.bindUniformMatrix(shader, "projection", projection, gl);
			ShaderVarHash.bindUniformMatrix(shader, "modelview", modelview, gl);
			
			
			
			
			gl.glBindBuffer(gl.GL_ARRAY_BUFFER, vbo2.getID());
	    	gl.glVertexAttribPointer(gl.glGetAttribLocation(shader.shaderprogram, vbo2.getName()), vbo2.getElementSize(), vbo2.getType(), false, 0, 0);
	    	gl.glEnableVertexAttribArray(gl.glGetAttribLocation(shader.shaderprogram, vbo2.getName()));
			
	    	
	    	GLVBO p = labelData.points[L];
	    	gl.glBindBuffer(gl.GL_ARRAY_BUFFER, p.getID());
			gl.glVertexAttribPointer(gl.glGetAttribLocation(shader.shaderprogram, p.getName()), p.getElementSize(), p.getType(), false, 4*p.getElementSize(), 0);
	    	gl.glEnableVertexAttribArray(gl.glGetAttribLocation(shader.shaderprogram, p.getName()));
	    	//important here: we advance to the next element only after all of tube_coords have been drawn.
	    	gl.glVertexAttribDivisor(gl.glGetAttribLocation(shader.shaderprogram, p.getName()), 1);
	    	
	    	GLVBO l = labelData.ltwh[L];
	    	gl.glBindBuffer(gl.GL_ARRAY_BUFFER, l.getID());
			gl.glVertexAttribPointer(gl.glGetAttribLocation(shader.shaderprogram, l.getName()), l.getElementSize(), l.getType(), false, 4*l.getElementSize(), 0);
	    	gl.glEnableVertexAttribArray(gl.glGetAttribLocation(shader.shaderprogram, l.getName()));
	    	//important here: we advance to the next element only after all of tube_coords have been drawn.
	    	gl.glVertexAttribDivisor(gl.glGetAttribLocation(shader.shaderprogram, l.getName()), 1);
	    	
	    	
	    	//actual draw command
	    	//gl.glDrawArrays(gl.GL_TRIANGLES, 0, vbo.getLength()/4);
	    	
	    	gl.glDrawArraysInstanced(gl.GL_TRIANGLES, 0, vbo2.getLength()/4, p.getLength()/4);
	    	
			
	    	gl.glDisableVertexAttribArray(gl.glGetAttribLocation(shader.shaderprogram, vbo2.getName()));
	    	
	    	gl.glDisableVertexAttribArray(gl.glGetAttribLocation(shader.shaderprogram, p.getName()));
			gl.glVertexAttribDivisor(gl.glGetAttribLocation(shader.shaderprogram, p.getName()), 0);
			gl.glDisableVertexAttribArray(gl.glGetAttribLocation(shader.shaderprogram, l.getName()));
			gl.glVertexAttribDivisor(gl.glGetAttribLocation(shader.shaderprogram, l.getName()), 0);
			
			shader.dontUseShader(gl);
		}
	}
	
	public static void renderDepth(LabelRenderData labelData, Label[] labels, JOGLRenderState state, int width, int height) {
		if(labels == null || labels.length == 0)
			return;
		GL3 gl = state.getGL();
		
		float[] projection = Rn.convertDoubleToFloatArray(state.getProjectionMatrix());
		float[] modelview = Rn.convertDoubleToFloatArray(state.getModelViewMatrix());
		
		for(int L = 0; L < labelData.tex.length; L++){
		
			//Texture2DLoader.load(gl, labelData.tex[L], gl.GL_TEXTURE2);
			
			shaderDepth.useShader(gl);
			
			gl.glEnable(gl.GL_BLEND);
			gl.glBlendFuncSeparate(gl.GL_SRC_ALPHA, gl.GL_ONE_MINUS_SRC_ALPHA,
					gl.GL_ONE, gl.GL_ONE_MINUS_SRC_ALPHA);
			
			
			gl.glUniform4fv(gl.glGetUniformLocation(shaderDepth.shaderprogram, "xyAlignmentTotalWH"), 1, labelData.xyAlignmentTotalWH[L], 0);
			gl.glUniform4fv(gl.glGetUniformLocation(shaderDepth.shaderprogram, "xyzOffsetScale"), 1, labelData.xyzOffsetScale, 0);
			
			ShaderVarHash.bindUniformMatrix(shaderDepth, "projection", projection, gl);
			ShaderVarHash.bindUniformMatrix(shaderDepth, "modelview", modelview, gl);
			gl.glUniform1i(gl.glGetUniformLocation(shaderDepth.shaderprogram, "image"), 0);
	    	gl.glUniform1i(gl.glGetUniformLocation(shaderDepth.shaderprogram, "width"), width);
	    	gl.glUniform1i(gl.glGetUniformLocation(shaderDepth.shaderprogram, "height"), height);
			
			
			
			
			gl.glBindBuffer(gl.GL_ARRAY_BUFFER, vbo2.getID());
	    	gl.glVertexAttribPointer(gl.glGetAttribLocation(shaderDepth.shaderprogram, vbo2.getName()), vbo2.getElementSize(), vbo2.getType(), false, 0, 0);
	    	gl.glEnableVertexAttribArray(gl.glGetAttribLocation(shaderDepth.shaderprogram, vbo2.getName()));
			
	    	
	    	GLVBO p = labelData.points[L];
	    	gl.glBindBuffer(gl.GL_ARRAY_BUFFER, p.getID());
			gl.glVertexAttribPointer(gl.glGetAttribLocation(shaderDepth.shaderprogram, p.getName()), p.getElementSize(), p.getType(), false, 4*p.getElementSize(), 0);
	    	gl.glEnableVertexAttribArray(gl.glGetAttribLocation(shaderDepth.shaderprogram, p.getName()));
	    	//important here: we advance to the next element only after all of tube_coords have been drawn.
	    	gl.glVertexAttribDivisor(gl.glGetAttribLocation(shaderDepth.shaderprogram, p.getName()), 1);
	    	
	    	GLVBO l = labelData.ltwh[L];
	    	gl.glBindBuffer(gl.GL_ARRAY_BUFFER, l.getID());
			gl.glVertexAttribPointer(gl.glGetAttribLocation(shaderDepth.shaderprogram, l.getName()), l.getElementSize(), l.getType(), false, 4*l.getElementSize(), 0);
	    	gl.glEnableVertexAttribArray(gl.glGetAttribLocation(shaderDepth.shaderprogram, l.getName()));
	    	//important here: we advance to the next element only after all of tube_coords have been drawn.
	    	gl.glVertexAttribDivisor(gl.glGetAttribLocation(shaderDepth.shaderprogram, l.getName()), 1);
	    	
	    	
	    	//actual draw command
	    	//gl.glDrawArrays(gl.GL_TRIANGLES, 0, vbo.getLength()/4);
	    	
	    	gl.glDrawArraysInstanced(gl.GL_TRIANGLES, 0, vbo2.getLength()/4, p.getLength()/4);
	    	
			
	    	gl.glDisableVertexAttribArray(gl.glGetAttribLocation(shaderDepth.shaderprogram, vbo2.getName()));
	    	
	    	gl.glDisableVertexAttribArray(gl.glGetAttribLocation(shaderDepth.shaderprogram, p.getName()));
			gl.glVertexAttribDivisor(gl.glGetAttribLocation(shaderDepth.shaderprogram, p.getName()), 0);
			gl.glDisableVertexAttribArray(gl.glGetAttribLocation(shaderDepth.shaderprogram, l.getName()));
			gl.glVertexAttribDivisor(gl.glGetAttribLocation(shaderDepth.shaderprogram, l.getName()), 0);
			
			shaderDepth.dontUseShader(gl);
		}
	}

	public static void addOneLayer(LabelRenderData labelData, Label[] labels, JOGLRenderState state, int width, int height) {
		if(labels == null || labels.length == 0)
			return;
		GL3 gl = state.getGL();
		
		float[] projection = Rn.convertDoubleToFloatArray(state.getProjectionMatrix());
		float[] modelview = Rn.convertDoubleToFloatArray(state.getModelViewMatrix());
		
		
		
		
		
		for(int L = 0; L < labelData.tex.length; L++){
		
			Texture2DLoader.load(gl, labelData.tex[L], gl.GL_TEXTURE2);
			
			shaderTransp.useShader(gl);
			
			gl.glEnable(gl.GL_BLEND);
			gl.glBlendFuncSeparate(gl.GL_SRC_ALPHA, gl.GL_ONE_MINUS_SRC_ALPHA,
					gl.GL_ONE, gl.GL_ONE_MINUS_SRC_ALPHA);
			
			
			gl.glUniform4fv(gl.glGetUniformLocation(shaderTransp.shaderprogram, "xyAlignmentTotalWH"), 1, labelData.xyAlignmentTotalWH[L], 0);
			gl.glUniform4fv(gl.glGetUniformLocation(shaderTransp.shaderprogram, "xyzOffsetScale"), 1, labelData.xyzOffsetScale, 0);
			
			gl.glUniform1i(gl.glGetUniformLocation(shaderTransp.shaderprogram, "tex"), 2);
			ShaderVarHash.bindUniformMatrix(shaderTransp, "projection", projection, gl);
			ShaderVarHash.bindUniformMatrix(shaderTransp, "modelview", modelview, gl);
			//transparency related uniforms
			gl.glUniform1i(gl.glGetUniformLocation(shaderTransp.shaderprogram, "_depth"), 9);
			gl.glUniform1i(gl.glGetUniformLocation(shaderTransp.shaderprogram, "_width"), width);
			gl.glUniform1i(gl.glGetUniformLocation(shaderTransp.shaderprogram, "_height"), height);
			
			
			
			gl.glBindBuffer(gl.GL_ARRAY_BUFFER, vbo2.getID());
	    	gl.glVertexAttribPointer(gl.glGetAttribLocation(shaderTransp.shaderprogram, vbo2.getName()), vbo2.getElementSize(), vbo2.getType(), false, 0, 0);
	    	gl.glEnableVertexAttribArray(gl.glGetAttribLocation(shaderTransp.shaderprogram, vbo2.getName()));
			
	    	
	    	GLVBO p = labelData.points[L];
	    	gl.glBindBuffer(gl.GL_ARRAY_BUFFER, p.getID());
			gl.glVertexAttribPointer(gl.glGetAttribLocation(shaderTransp.shaderprogram, p.getName()), p.getElementSize(), p.getType(), false, 4*p.getElementSize(), 0);
	    	gl.glEnableVertexAttribArray(gl.glGetAttribLocation(shaderTransp.shaderprogram, p.getName()));
	    	//important here: we advance to the next element only after all of tube_coords have been drawn.
	    	gl.glVertexAttribDivisor(gl.glGetAttribLocation(shaderTransp.shaderprogram, p.getName()), 1);
	    	
	    	GLVBO l = labelData.ltwh[L];
	    	gl.glBindBuffer(gl.GL_ARRAY_BUFFER, l.getID());
			gl.glVertexAttribPointer(gl.glGetAttribLocation(shaderTransp.shaderprogram, l.getName()), l.getElementSize(), l.getType(), false, 4*l.getElementSize(), 0);
	    	gl.glEnableVertexAttribArray(gl.glGetAttribLocation(shaderTransp.shaderprogram, l.getName()));
	    	//important here: we advance to the next element only after all of tube_coords have been drawn.
	    	gl.glVertexAttribDivisor(gl.glGetAttribLocation(shaderTransp.shaderprogram, l.getName()), 1);
	    	
	    	
	    	//actual draw command
	    	//gl.glDrawArrays(gl.GL_TRIANGLES, 0, vbo.getLength()/4);
	    	
	    	gl.glDrawArraysInstanced(gl.GL_TRIANGLES, 0, vbo2.getLength()/4, p.getLength()/4);
	    	
			
	    	gl.glDisableVertexAttribArray(gl.glGetAttribLocation(shaderTransp.shaderprogram, vbo2.getName()));
	    	
	    	gl.glDisableVertexAttribArray(gl.glGetAttribLocation(shaderTransp.shaderprogram, p.getName()));
			gl.glVertexAttribDivisor(gl.glGetAttribLocation(shaderTransp.shaderprogram, p.getName()), 0);
			gl.glDisableVertexAttribArray(gl.glGetAttribLocation(shaderTransp.shaderprogram, l.getName()));
			gl.glVertexAttribDivisor(gl.glGetAttribLocation(shaderTransp.shaderprogram, l.getName()), 0);
			
			shaderTransp.dontUseShader(gl);
		}
	}
	
}
