package de.jreality.jogl3.shader;

import java.nio.ByteBuffer;
import java.nio.FloatBuffer;
import java.util.Set;
import java.util.WeakHashMap;

import javax.media.opengl.GL3;

import de.jreality.shader.ImageData;
import de.jreality.shader.Texture2D;

public class Texture2DLoader {
	private static WeakHashMap<ImageData, Integer> textureLookup = new WeakHashMap<ImageData, Integer>();
	
	public static void deleteTextures(GL3 gl){
		Set<ImageData> keys = textureLookup.keySet();
		int[] texs = new int[keys.size()];
		int i = 0;
		for(ImageData id : keys){
			texs[i] = textureLookup.get(id);
			i++;
		}
//		for(int j = 0; j < texs.length; j++)
//			System.err.println("deleting texid no " + texs[j]);
		if(texs.length != 0)
			gl.glDeleteTextures(texs.length, texs, 0);
		textureLookup = new WeakHashMap<ImageData, Integer>();
	}
	
	private static int createTextureID(GL3 gl) 
	{ 
	   int[] tmp = new int[1]; 
	   gl.glGenTextures(1, tmp, 0);

	   return tmp[0]; 
	}
	
	public static int getID(Texture2D tex){
		return textureLookup.get(tex.getImage());
	}
	
	@SuppressWarnings("static-access")
	public static void load(GL3 gl, Texture2D tex, int texUnit){
		ImageData image = tex.getImage();
    	if (image == null) return;
    	int width = image.getWidth(), height = image.getHeight();
    	
    	Integer texid = textureLookup.get(image);
    	gl.glEnable(gl.GL_TEXTURE_2D);
    	gl.glActiveTexture(texUnit);
    	// create the texture ID for this texture
    	if (texid != null) {
    		gl.glBindTexture(gl.GL_TEXTURE_2D, texid);
    		//do nothing because texture is already loaded
    	}else{
    		//System.out.println("creating new texture");
    		//load texture into the gl
    		texid = createTextureID(gl);
//    		System.err.println("creating new texid no " + texid);
    		textureLookup.put(image, texid);
    		
    		gl.glBindTexture(gl.GL_TEXTURE_2D, texid);
    		//TODO texture matrix...
    		int srcPixelFormat = tex.getPixelFormat();
    	    
    	    
    	    
//    	    gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_WRAP_S, tex.getRepeatS()); 
//    	    gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_WRAP_T, tex.getRepeatT()); 
    	    gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_MIN_FILTER, tex.getMinFilter()); 
    	    gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_MAG_FILTER, tex.getMagFilter());
//    	    gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_MAG_FILTER, gl.GL_NEAREST_MIPMAP_NEAREST);
//        	gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_MIN_FILTER, gl.GL_NEAREST_MIPMAP_NEAREST);
        	
        	gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_WRAP_S, tex.getRepeatS());
        	gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_WRAP_T, tex.getRepeatT());
        	
        	//gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_, tex.getRepeatS());
    	    //tex.getRepeatS()
        	//TODO convert to shader...
//    	    float[] texcolor = tex.getBlendColor().getRGBComponents(null);
//    	    texcolor[3] = 1f - texcolor[3];
    	    
//    	    gl.glTexParameterfv(gl.GL_TEXTURE_ENV, gl.GL_TEXTURE_ENV_COLOR, texcolor, 0);
//    	    gl.glTexParameterf(gl.GL_TEXTURE_ENV, GglL2.GL_TEXTURE_ENV_MODE, tex.getApplyMode());
//    	    if (tex.getApplyMode() == Texture2D.GL_COMBINE) 
//    	    {
//    	    	gl.glTexParameterf(GL2.GL_TEXTURE_ENV, GL2.GL_COMBINE_RGB, tex.getCombineMode());
//    	    	gl.glTexParameterf(GL2.GL_TEXTURE_ENV, GL2.GL_SOURCE0_RGB, tex.getSource0Color()); 
//    	    	gl.glTexParameterf(GL2.GL_TEXTURE_ENV, GL2.GL_OPERAND0_RGB, tex.getOperand0Color()); 
//    	    	gl.glTexParameterf(GL2.GL_TEXTURE_ENV, GL2.GL_SOURCE1_RGB, tex.getSource1Color()); 
//    	    	gl.glTexParameterf(GL2.GL_TEXTURE_ENV, GL2.GL_OPERAND1_RGB, tex.getOperand1Color()); 
//    	    	gl.glTexParameterf(GL2.GL_TEXTURE_ENV, GL2.GL_SOURCE2_RGB, tex.getSource2Color()); 
//    	    	gl.glTexParameterf(GL2.GL_TEXTURE_ENV, GL2.GL_OPERAND2_RGB,tex.getOperand2Color()); 
//    	    	gl.glTexParameterf(GL2.GL_TEXTURE_ENV, GL2.GL_COMBINE_ALPHA, tex.getCombineModeAlpha());
//    	    	gl.glTexParameterf(GL2.GL_TEXTURE_ENV, GL2.GL_SOURCE0_ALPHA, tex.getSource0Alpha()); 
//    	    	gl.glTexParameterf(GL2.GL_TEXTURE_ENV, GL2.GL_OPERAND0_ALPHA, tex.getOperand0Alpha()); 
//    	    	gl.glTexParameterf(GL2.GL_TEXTURE_ENV, GL2.GL_SOURCE1_ALPHA, tex.getSource1Alpha()); 
//    	    	gl.glTexParameterf(GL2.GL_TEXTURE_ENV, GL2.GL_OPERAND1_ALPHA, tex.getOperand1Alpha()); 
//    	    	gl.glTexParameterf(GL2.GL_TEXTURE_ENV, GL2.GL_SOURCE2_ALPHA, tex.getSource2Alpha()); 
//    	    	gl.glTexParameterf(GL2.GL_TEXTURE_ENV, GL2.GL_OPERAND2_ALPHA,tex.getOperand2Alpha()); 
//    	    }    
    	     // create either a series of mipmaps of a single texture image based on
    	    // what's loaded
    	    byte[] data = image.getByteArray();
    	    //boolean mipmapped = tex.getMipmapMode();
    	    
            gl.glPixelStorei(gl.GL_UNPACK_ROW_LENGTH, image.getWidth());
        	gl.glPixelStorei(gl.GL_UNPACK_SKIP_ROWS, 0);
        	gl.glPixelStorei(gl.GL_UNPACK_SKIP_PIXELS, 0);
        	//gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_GENERATE_MIPMAP, gl.GL_FALSE);
        	gl.glTexImage2D(gl.GL_TEXTURE_2D, 0, gl.GL_RGBA, 
        	image.getWidth(), image.getHeight(), 0, srcPixelFormat,
    	    gl.GL_UNSIGNED_BYTE, ByteBuffer.wrap(data));
    		
        	gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_BASE_LEVEL, 0);
        	gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_MAX_LEVEL, 10);
        	
        	FloatBuffer maxAnisotropy = FloatBuffer.allocate(1);
        	gl.glGetFloatv(gl.GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
	        //System.out.println("max anisotropy "+ maxAnisotropy.get(0));
        	gl.glTexParameterf(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy.get(0));
//        	gl.glTexParameterf(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_MAX_ANISOTROPY_EXT, 0);
        	gl.glGenerateMipmap(gl.GL_TEXTURE_2D);

    	}
    		
	}
	
	
}
