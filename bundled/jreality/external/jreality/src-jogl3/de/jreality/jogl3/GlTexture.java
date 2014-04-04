package de.jreality.jogl3;

import javax.media.opengl.GL3;

import de.jreality.jogl3.glsl.GLShader;
import de.jreality.jogl3.shader.ShaderVarHash;
import de.jreality.jogl3.shader.Texture2DLoader;
import de.jreality.shader.Texture2D;



/**
 * The 1st standard texture of a geometry.
 * @author Benjamin
 */

public class GlTexture{
	private boolean hasTexture = false;
	public boolean hasTexture(){
		return hasTexture;
	}
	public GlTexture(){
		
	}
	private Texture2D tex = null;
	public int combineMode = 0;
	public Texture2D getTexture2D(){
		return tex;
	}
	public void setTexture(Texture2D tex){
		this.tex = tex;
		hasTexture = true;
	}
	public void removeTexture(){
		hasTexture = false;
	}
    /**
     * loads the texture specified in {@link #setTexture(Texture2D)} and binds it to the glsl shader
     * @param shader the glsl shader that uses this texture
     */
	public void bind(GLShader shader, GL3 gl){
		if(hasTexture){
			//GL_TEXTURE0 and GL_TEXTURE1 reserved for lights.
			Texture2DLoader.load(gl, tex, gl.GL_TEXTURE8);
			ShaderVarHash.bindUniform(shader, "image", 8, gl);
			ShaderVarHash.bindUniform(shader, "has_Tex", 1, gl);
		}else{
			ShaderVarHash.bindUniform(shader, "has_Tex", 0, gl);
		}
	}
}
