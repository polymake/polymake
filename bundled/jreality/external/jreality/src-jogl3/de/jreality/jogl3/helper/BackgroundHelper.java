package de.jreality.jogl3.helper;

import java.awt.Color;

import javax.media.opengl.GL3;

import de.jreality.jogl3.glsl.GLShader;
import de.jreality.jogl3.shader.GLVBOFloat;
import de.jreality.jogl3.shader.Texture2DLoader;
import de.jreality.scene.Appearance;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.Texture2D;
import de.jreality.shader.TextureUtility;

public class BackgroundHelper {
	
	/**
	 * @param args
	 */
	GLShader backgroundShader;
	GLShader backgroundShaderTex;
	private float[] quadVerts = {1, 1, 0, 1,
							-1, 1, 0, 1,
							-1, -1, 0, 1,
							-1, -1, 0, 1,
							1, -1, 0, 1,
							1, 1, 0, 1};
	private float[] colorValues = new float[24];/*{
			0.88235295f, 0.88235295f, 0.88235295f, 1.0f,
			1.0f, 0.88235295f, 0.7058824f, 1.0f,
			1.0f, 0.88235295f, 0.7058824f, 1.0f,
			1.0f, 0.88235295f, 0.7058824f, 1.0f,
			0.88235295f, 0.88235295f, 0.88235295f, 1.0f,
			0.88235295f, 0.88235295f, 0.88235295f, 1.0f
	};*/
	private float[] texCoords = new float[24];
	GLVBOFloat quad;
	GLVBOFloat colors;
	GLVBOFloat texcoords;
	private Appearance pseudoAp = new Appearance();
	private boolean hasTexture = false, hasColors = false;
	private Texture2D tex;
	public void updateBackground(GL3 gl, Appearance topAp, int width, int height){
		//retrieve information from jReality
		//JOGLRenderingState openGLState = jr.renderingState;
		Object bgo = null;
		float[] backgroundColor = new float[4];
		if (topAp == null)
			topAp = pseudoAp;
		// return;
//		for (int i = 0; i < 6; ++i) {
//			gl.glDisable(i + gl.GL_CLIP_PLANE0);
//		}
		if (topAp != null)
			bgo = topAp.getAttribute(CommonAttributes.BACKGROUND_COLOR);
		if (bgo != null && bgo instanceof java.awt.Color)
			((java.awt.Color) bgo).getRGBComponents(backgroundColor);
		else
			backgroundColor = CommonAttributes.BACKGROUND_COLOR_DEFAULT
					.getRGBComponents(null);
		gl.glClearColor(backgroundColor[0], backgroundColor[1],
				backgroundColor[2], backgroundColor[3]);
		// Here is where we clear the screen and set the color mask
		// It's a bit complicated by the various color masking required by
		// color-channel stereo (see JOGLRenderer#display() ).
		// System.err.println("clearbufferbits = "+jr.openGLState.clearBufferBits);
		// System.err.println("colormask = "+jr.openGLState.colorMask);
		// first set the color mask for the clear
		// LoggingSystem.getLogger(JOGLRendererHelper.class).finest("JOGLRRH cbb = "+
		// openGLState.clearBufferBits);
		// set color mask for the clear
//		if ((openGLState.clearBufferBits & GL.GL_COLOR_BUFFER_BIT) != 0) {
//			gl.glColorMask(true, true, true, true);
//		}
//		gl.glClear(openGLState.clearBufferBits);
		 //now set the color mask for pixel writing
//		int cm = openGLState.colorMask;
//		gl.glColorMask((cm & 1) != 0, (cm & 2) != 0, (cm & 4) != 0,
//				(cm & 8) != 0);

		Object obj = topAp.getAttribute(CommonAttributes.SKY_BOX);
		// only draw background colors or texture if the skybox isn't there
		if (obj == Appearance.INHERITED) {
			
			double textureAR = 1.0;
			obj = TextureUtility.getBackgroundTexture(topAp);
			tex = null;
			if (obj != null) {
				tex = (Texture2D) obj;
				textureAR = tex.getImage().getWidth()
						/ ((double) tex.getImage().getHeight());
				hasTexture = true;
				Texture2DLoader.load(gl, tex, gl.GL_TEXTURE0);
			}
			// bgo = topAp.getAttribute(BACKGROUND_TEXTURE2D);
			// if (bgo != null && bgo instanceof List) {
			// tex = (Texture2D) ((List)bgo).get(0);
			// }
			double ar = width / ((double) height) / textureAR;
			double xl = 0, xr = 1, yb = 0, yt = 1;
			if (ar > 1.0) {
				xl = 0.0;
				xr = 1.0;
				yb = .5 * (1 - 1 / ar);
				yt = 1.0 - yb;
			} else {
				yb = 0.0;
				yt = 1.0;
				xl = .5 * (1 - ar);
				xr = 1.0 - xl;
			}
//			if (jr.offscreenMode) {
//				int numTiles = jr.offscreenRenderer.getNumTiles();
//				double xmin = ((double) jr.whichTile[0]) / numTiles;
//				double xmax = ((double) jr.whichTile[0] + 1) / numTiles;
//				double ymin = ((double) jr.whichTile[1]) / numTiles;
//				double ymax = ((double) jr.whichTile[1] + 1) / numTiles;
//				double nxl, nxr, nyb, nyt;
//				nxr = xr + xmin * (xl - xr);
//				nxl = xr + xmax * (xl - xr);
//				nyt = yt + ymin * (yb - yt);
//				nyb = yt + ymax * (yb - yt);
//				xl = nxl;
//				xr = nxr;
//				yb = nyb;
//				yt = nyt;
//			}
			//double[][] texcoords = { { xl, yb }, { xr, yb }, { xr, yt },
				//	{ xl, yt } };
			
			texCoords[0+0] = (float)xl;
			texCoords[0+1] = (float)yb;
			texCoords[2+0] = (float)xr;
			texCoords[2+1] = (float)yb;
			texCoords[4+0] = (float)xr;
			texCoords[4+1] = (float)yt;
			
			texCoords[6+0] = (float)xr;
			texCoords[6+1] = (float)yt;
			texCoords[8+0] = (float)xl;
			texCoords[8+1] = (float)yt;
			texCoords[10+0] = (float)xl;
			texCoords[10+1] = (float)yb;
			
			texcoords.updateData(gl, texCoords);
				
			//float[][] cornersf = new float[4][];
			if (!hasTexture) {
				bgo = topAp.getAttribute(CommonAttributes.BACKGROUND_COLORS);
				if (bgo != null && bgo instanceof Color[]) {
					Color[] backgroundCorners = (Color[]) bgo;
					for(int j = 0; j < 4; j++){
						
						colorValues[0+j] = backgroundCorners[0].getRGBComponents(null)[j];
						colorValues[4+j] = backgroundCorners[1].getRGBComponents(null)[j];
						colorValues[8+j] = backgroundCorners[2].getRGBComponents(null)[j];
						colorValues[12+j] = backgroundCorners[2].getRGBComponents(null)[j];
						colorValues[16+j] = backgroundCorners[3].getRGBComponents(null)[j];
						colorValues[20+j] = backgroundCorners[0].getRGBComponents(null)[j];
						System.out.println(colorValues[0+j]);
					}
				} else {
					for (int i = 0; i < 6; ++i)
						for(int j = 0; j < 4; j++)
							colorValues[i*4+j] = backgroundColor[j];
				}
				hasColors = true;
				colors.updateData(gl, colorValues);
			}
			
		}
		//TODO do fog as below
//		bgo = topAp.getAttribute(CommonAttributes.FOG_ENABLED);
//		boolean doFog = CommonAttributes.FOG_ENABLED_DEFAULT;
//		if (bgo instanceof Boolean)
//			doFog = ((Boolean) bgo).booleanValue();
//		jr.renderingState.fogEnabled = doFog;
//		if (doFog) {
//			gl.glEnable(GL2.GL_FOG);
//			bgo = topAp.getAttribute(CommonAttributes.FOG_COLOR);
//			float[] fogColor = backgroundColor;
//			if (bgo != null && bgo instanceof Color) {
//				fogColor = ((Color) bgo).getRGBComponents(null);
//			}
//			gl.glFogi(GL2.GL_FOG_MODE, GL2.GL_EXP);
//			gl.glFogfv(GL2.GL_FOG_COLOR, fogColor, 0);
//			bgo = topAp.getAttribute(CommonAttributes.FOG_DENSITY);
//			float density = (float) CommonAttributes.FOG_DENSITY_DEFAULT;
//			if (bgo != null && bgo instanceof Double) {
//				density = (float) ((Double) bgo).doubleValue();
//			}
//			gl.glFogf(GL2.GL_FOG_DENSITY, density);
//		} else {
//			gl.glDisable(GL2.GL_FOG);
//			gl.glFogf(GL2.GL_FOG_DENSITY, 0f);
//		}
	}
	
	public void draw(GL3 gl){

		if(hasColors){
			backgroundShader.useShader(gl);
			
			gl.glBindBuffer(gl.GL_ARRAY_BUFFER, quad.getID());
    		gl.glVertexAttribPointer(gl.glGetAttribLocation(backgroundShader.shaderprogram, "vertex_coordinates"), quad.getElementSize(), quad.getType(), false, 0, 0);
    		gl.glEnableVertexAttribArray(gl.glGetAttribLocation(backgroundShader.shaderprogram, "vertex_coordinates"));
    		
    		gl.glBindBuffer(gl.GL_ARRAY_BUFFER, colors.getID());
    		gl.glVertexAttribPointer(gl.glGetAttribLocation(backgroundShader.shaderprogram, "vertex_colors"), colors.getElementSize(), colors.getType(), false, 0, 0);
    		gl.glEnableVertexAttribArray(gl.glGetAttribLocation(backgroundShader.shaderprogram, "vertex_colors"));
    	
    		gl.glDrawArrays(gl.GL_TRIANGLES, 0, quad.getLength()/4);
    		
    		gl.glDisableVertexAttribArray(gl.glGetAttribLocation(backgroundShader.shaderprogram, "vertex_coordinates"));
    		gl.glDisableVertexAttribArray(gl.glGetAttribLocation(backgroundShader.shaderprogram, "vertex_colors"));
    		backgroundShader.dontUseShader(gl);
		}
		if(hasTexture){
			backgroundShaderTex.useShader(gl);
			
			Texture2DLoader.load(gl, tex, gl.GL_TEXTURE0);
			gl.glUniform1i(gl.glGetUniformLocation(backgroundShaderTex.shaderprogram, "image"), 0);
			
			gl.glBindBuffer(gl.GL_ARRAY_BUFFER, quad.getID());
    		gl.glVertexAttribPointer(gl.glGetAttribLocation(backgroundShaderTex.shaderprogram, "vertex_coordinates"), quad.getElementSize(), quad.getType(), false, 0, 0);
    		gl.glEnableVertexAttribArray(gl.glGetAttribLocation(backgroundShaderTex.shaderprogram, "vertex_coordinates"));
    		
    		gl.glBindBuffer(gl.GL_ARRAY_BUFFER, texcoords.getID());
    		gl.glVertexAttribPointer(gl.glGetAttribLocation(backgroundShaderTex.shaderprogram, "tex_coords"), texcoords.getElementSize(), texcoords.getType(), false, 0, 0);
    		gl.glEnableVertexAttribArray(gl.glGetAttribLocation(backgroundShaderTex.shaderprogram, "tex_coords"));
    	
    		gl.glDrawArrays(gl.GL_TRIANGLES, 0, quad.getLength()/4);
    		
    		gl.glDisableVertexAttribArray(gl.glGetAttribLocation(backgroundShaderTex.shaderprogram, "vertex_coordinates"));
    		gl.glDisableVertexAttribArray(gl.glGetAttribLocation(backgroundShaderTex.shaderprogram, "tex_coords"));
    		
    		backgroundShaderTex.dontUseShader(gl);
		}
	}
	
	public void initializeBackground(GL3 gl){
		quad = new GLVBOFloat(gl, quadVerts, "vertex_coordinates");
		colors = new GLVBOFloat(gl, colorValues, "vertex_colors");
		texcoords = new GLVBOFloat(gl, texCoords, "texCoords", 2);
		backgroundShader = new GLShader("nontransp/background.v", "nontransp/background.f");
		backgroundShader.init(gl);
		backgroundShaderTex = new GLShader("nontransp/backgroundTex.v", "nontransp/backgroundTex.f");
		backgroundShaderTex.init(gl);
	}
}
