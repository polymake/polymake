package de.jreality.jogl3.geom;

import javax.media.opengl.GL3;

import de.jreality.jogl3.JOGLTexture2D;
import de.jreality.jogl3.glsl.GLShader;
import de.jreality.jogl3.shader.ShaderVarHash;
import de.jreality.jogl3.shader.Texture2DLoader;
import de.jreality.scene.Appearance;
import de.jreality.scene.data.AttributeEntityUtility;
import de.jreality.shader.CubeMap;
import de.jreality.shader.ImageData;
import de.jreality.shader.Texture2D;
import de.jreality.shader.TextureUtility;

public class GlReflectionMap{
	private boolean hasReflectionMap = false;
	public float alpha = 0.5f;
	public boolean hasReflMap(){
		return hasReflectionMap;
	}
	public GlReflectionMap(){
		
	}
	private JOGLTexture2D[] jogltex = new JOGLTexture2D[6];
	public void setCubeMap(CubeMap cm){
		ImageData[] imgs=TextureUtility.getCubeMapImages(cm);
		for(int i = 0; i < 6; i++){
			  Texture2D tex=(Texture2D) AttributeEntityUtility.createAttributeEntity(Texture2D.class, "", new Appearance(), true);
			  tex.setRepeatS(de.jreality.shader.Texture2D.GL_CLAMP_TO_EDGE);
			  tex.setRepeatT(de.jreality.shader.Texture2D.GL_CLAMP_TO_EDGE);
			  jogltex[i] = new JOGLTexture2D(tex);
			  
			  //jogltex[i].setBlendColor(cm.getBlendColor());
			  jogltex[i].setImage(imgs[i]);
			  
			  
		}
		hasReflectionMap = true;
	}
	public void removeTexture(){
		hasReflectionMap = false;
	}
	public void bind(GLShader shader, GL3 gl){
		if(hasReflectionMap){
			//GL_TEXTURE0 and GL_TEXTURE1 reserved for lights.
			for(int i = 0; i < 6; i++){
				 String name = "right";
				  if(i == 1)
					  name = "left";
				  else if(i == 2)
					  name = "up";
				  else if(i == 3)
					  name = "down";
				  else if(i == 4)
					  name = "back";
				  else if(i == 5)
					  name = "front";
				ShaderVarHash.bindUniform(shader, name, 2+i, gl);
				ShaderVarHash.bindUniform(shader, "has_reflectionMap", 1, gl);
				Texture2DLoader.load(gl, jogltex[i], gl.GL_TEXTURE2+i);
			}
		}else{
			ShaderVarHash.bindUniform(shader, "has_reflectionMap", 0, gl);
		}
	}
}
