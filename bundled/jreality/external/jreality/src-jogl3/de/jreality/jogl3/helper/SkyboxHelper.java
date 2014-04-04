package de.jreality.jogl3.helper;

import javax.media.opengl.GL3;

import de.jreality.jogl3.JOGLTexture2D;
import de.jreality.jogl3.glsl.GLShader;
import de.jreality.jogl3.shader.GLVBOFloat;
import de.jreality.jogl3.shader.Texture2DLoader;
import de.jreality.math.Rn;
import de.jreality.scene.Appearance;
import de.jreality.scene.Camera;
import de.jreality.scene.data.AttributeEntityUtility;
import de.jreality.shader.CubeMap;
import de.jreality.shader.ImageData;
import de.jreality.shader.Texture2D;
import de.jreality.shader.TextureUtility;

public class SkyboxHelper {
	// TODO straighten out nomenclature on faces
	private static float[] cubeVerts3 =  
		{
			 1,1,-1, 1,-1,1,-1, 1, -1,-1,-1, 1,-1,-1,-1,1, 1,-1,-1,1 ,1,1,-1,1,			// front
			 -1,1,1,1, 1,1,1,1, 1,-1,1,1,1,-1,1,1,-1,-1,1,1,-1,1,1,1,				// back
			 -1, 1, -1,1, -1, 1, 1,1,-1,-1,1,1,-1,-1,1,1, -1,-1,-1,1,-1, 1, -1,1, 	// left
			 1,1,1,1, 1,1,-1,1, 1, -1, -1,1,1, -1, -1,1, 1, -1, 1,1,1,1,1	,1,		// right
			 -1, 1,-1,1,  1, 1,-1,1,1, 1,1,1,1, 1,1,1, -1, 1,1,1,-1, 1,-1,1,		//up
			 -1,-1,1,1,1,-1,1,1,1,-1,-1,1,1,-1,-1,1, -1,-1,-1,1,-1,-1,1,1			//down
		 };
	private static GLVBOFloat verts;
	private static GLVBOFloat texCoordsVBO;
	private static GLVBOFloat texNoVBO;
  // TODO figure out texture coordinates 
	private static float[] texCoords = {0,0,0,1,0,0,1,1,0,1,1,0,0,1,0,0,0,0,
		0,0,1,1,0,1,1,1,1,1,1,1,0,1,1,0,0,1,
		0,0,2,1,0,2,1,1,2,1,1,2,0,1,2,0,0,2,
		0,0,3,1,0,3,1,1,3,1,1,3,0,1,3,0,0,3,
		0,0,4,1,0,4,1,1,4,1,1,4,0,1,4,0,0,4,
		0,0,5,1,0,5,1,1,5,1,1,5,0,1,5,0,0,5};
	private static float[] texNo = {0,0,0,0,0,0,
		1,1,1,1,1,1,
		2,2,2,2,2,2,
		3,3,3,3,3,3,
		4,4,4,4,4,4,
		5,5,5,5,5,5};
//		1,1,1,1,1,1,
//		2,2,2,2,2,2,
//		3,3,3,3,3,3,
//		4,4,4,4,4,4,
//		5,5,5,5,5,5};
	static Appearance[] a= new Appearance[6];
	//static Texture2D[] tex = new Texture2D[6];
	static JOGLTexture2D[] jogltex = new JOGLTexture2D[6];
	static {
		for(int i = 0; i < 6; i++){
			a[i] = new Appearance();
			Texture2D tex=(Texture2D) AttributeEntityUtility.createAttributeEntity(Texture2D.class, "", a[i], true);
			tex.setRepeatS(de.jreality.shader.Texture2D.GL_CLAMP_TO_EDGE);
			tex.setRepeatT(de.jreality.shader.Texture2D.GL_CLAMP_TO_EDGE);
			jogltex[i] = new JOGLTexture2D(tex);
		}
	}
	private static GLShader shader;
	public static void init(GL3 gl){
		//init shaders
		shader = new GLShader("nontransp/cubemap.v", "nontransp/cubemap.f");
		shader.init(gl);
	    //init vbos
		verts = new GLVBOFloat(gl, cubeVerts3, "vertex_coordinates");
		texCoordsVBO = new GLVBOFloat(gl, texCoords, "vertex_texturecoordinates");
		texNoVBO = new GLVBOFloat(gl, texNo, "vertex_tex_no");
	}
	
//	public static void bindSamplers(GL3 gl, GLShader _shader){
//		for(int i = 0; i < 6; i++){
//			  String name = "right";
//			  if(i == 1)
//				  name = "left";
//			  else if(i == 2)
//				  name = "up";
//			  else if(i == 3)
//				  name = "down";
//			  else if(i == 4)
//				  name = "back";
//			  else if(i == 5)
//				  name = "front";
//			  gl.glUniform1i(gl.glGetUniformLocation(_shader.shaderprogram, name), i+2);
//			  //jogltex[i].setBlendColor(cm.getBlendColor());
//			  //jogltex[i].setImage(imgs[i]);
//			  Texture2DLoader.load(gl, jogltex[i], gl.GL_TEXTURE2+i);
//		  }
//	}
	

	//TODO simplify this. We don't really need the camera here. Could probably turn off depth test and
	//somwhow calculate the texture coordinates from modelview, projection matrices.
	private static double[] modelview, projection;
	private static CubeMap cm;
	private static Camera cam;
	public static void setup(double[] modelview, double[] projection, CubeMap cm, Camera cam){
		SkyboxHelper.modelview = modelview;
		SkyboxHelper.projection = projection;
		SkyboxHelper.cm = cm;
		SkyboxHelper.cam = cam;
	}
  @SuppressWarnings("static-access")
public static void render(GL3 gl)	{
    if(cm == null)
    	return;
    
    gl.glDisable(gl.GL_BLEND);
	gl.glDisable(gl.GL_DEPTH_TEST);
	
	shader.useShader(gl);
    
	  ImageData[] imgs=TextureUtility.getCubeMapImages(cm);
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
		  gl.glUniform1i(gl.glGetUniformLocation(shader.shaderprogram, name), i);

		  jogltex[i].setBlendColor(cm.getBlendColor());
		  jogltex[i].setImage(imgs[i]);
		  Texture2DLoader.load(gl, jogltex[i], gl.GL_TEXTURE0+i);
		  
	  }
	float scale =(float) (cam.getNear() + cam.getFar())/2;
	//System.out.println(scale);
	
	//matrices
	gl.glUniformMatrix4fv(gl.glGetUniformLocation(shader.shaderprogram, "projection"), 1, true, Rn.convertDoubleToFloatArray(projection), 0);
	gl.glUniformMatrix4fv(gl.glGetUniformLocation(shader.shaderprogram, "modelview"), 1, true, Rn.convertDoubleToFloatArray(modelview), 0);
	
	gl.glUniform1f(gl.glGetUniformLocation(shader.shaderprogram, "scale"), scale);
	
	//bind vbos to corresponding shader variables
	gl.glBindBuffer(gl.GL_ARRAY_BUFFER, texCoordsVBO.getID());
    gl.glVertexAttribPointer(gl.glGetAttribLocation(shader.shaderprogram, texCoordsVBO.getName()), 3, texCoordsVBO.getType(), false, 0, 0);
    gl.glEnableVertexAttribArray(gl.glGetAttribLocation(shader.shaderprogram, texCoordsVBO.getName()));
	
    gl.glBindBuffer(gl.GL_ARRAY_BUFFER, verts.getID());
    gl.glVertexAttribPointer(gl.glGetAttribLocation(shader.shaderprogram, verts.getName()), 4, verts.getType(), false, 0, 0);
    gl.glEnableVertexAttribArray(gl.glGetAttribLocation(shader.shaderprogram, verts.getName()));
	
    gl.glBindBuffer(gl.GL_ARRAY_BUFFER, texNoVBO.getID());
    gl.glVertexAttribPointer(gl.glGetAttribLocation(shader.shaderprogram, texNoVBO.getName()), 1, texNoVBO.getType(), false, 0, 0);
    gl.glEnableVertexAttribArray(gl.glGetAttribLocation(shader.shaderprogram, texNoVBO.getName()));
	
	
	//actual draw command
	gl.glDrawArrays(gl.GL_TRIANGLES, 0, verts.getLength()/4);

	//disable all vbos
	gl.glDisableVertexAttribArray(gl.glGetAttribLocation(shader.shaderprogram, verts.getName()));
	gl.glDisableVertexAttribArray(gl.glGetAttribLocation(shader.shaderprogram, texCoordsVBO.getName()));
	gl.glDisableVertexAttribArray(gl.glGetAttribLocation(shader.shaderprogram, texNoVBO.getName()));
	
	shader.dontUseShader(gl);
	
  }
}
