/**
 *
 * This file is part of jReality. jReality is open source software, made
 * available under a BSD license:
 *
 * Copyright (c) 2003-2006, jReality Group: Charles Gunn, Tim Hoffmann, Markus
 * Schmies, Steffen Weissmann.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * - Neither the name of jReality nor the names of its contributors nor the
 *   names of their associated organizations may be used to endorse or promote
 *   products derived from this software without specific prior written
 *   permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */


package de.jreality.shader;

import static de.jreality.shader.CommonAttributes.TEXTURE_2D;
import static de.jreality.shader.CommonAttributes.TEXTURE_2D_1;
import static de.jreality.shader.CommonAttributes.TEXTURE_2D_2;
import static de.jreality.shader.CommonAttributes.TEXTURE_2D_3;

import java.awt.Color;
import java.io.IOException;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

import de.jreality.math.Rn;
import de.jreality.scene.Appearance;
import de.jreality.scene.Scene;
import de.jreality.scene.data.AttributeEntityUtility;
import de.jreality.util.Input;
import de.jreality.util.TargaFile;


/**
 *
 * A bunch of factory methods that create Textures and ReflectionMaps. Other shader related
 * stuff should follow.
 *
 * @author Steffen Weissman
 *
 */
public class TextureUtility {

	protected TextureUtility() {}

	private static Texture2D createTextureImpl(Appearance app, String key, ImageData img, boolean readDefaults) {
		Texture2D ret = (Texture2D) AttributeEntityUtility.createAttributeEntity(Texture2D.class, key, app, readDefaults);
		ret.setImage(img);
		return ret;
	}
	/**
	 * method to create a reflectionMap for the given image (as ImageData).
	 * 
	 * @param app the appearance where to create the Texture2D 
	 * @param shader the name of the shader, should be null, "", "polygonShader" maybe "lineShader" works also
	 * @param img the ImageData
	 * @param readDefaults true for the Texture2D to return default values, if false it returns null for unset values
	 * @return a proxy implementation of Texture2D
	 */
	public static Texture2D createTexture(Appearance app, String shader, ImageData img, boolean readDefaults) {
		return createTexture(app, shader, 0, img, readDefaults);
	}

	private static String[] texture2dNames = {TEXTURE_2D, TEXTURE_2D_1, TEXTURE_2D_2, TEXTURE_2D_3};
	public static Texture2D createTexture(Appearance app, String shader, int i, ImageData img, boolean readDefaults) {
		String key = (shader == null || shader.equals("")) ? texture2dNames[i] : shader+"."+texture2dNames[i];
		return createTextureImpl(app, key, img, readDefaults);

	}
	public static Texture2D createTexture(Appearance app, String shader, ImageData img) {
		return createTexture(app, shader, 0, img, true);
	}

	public static Texture2D createTexture(Appearance app, String shader, int i, ImageData img) {
		return createTexture(app, shader, i, img, true);
	}
/**
	 * method to create a reflectionMap for the given image (as ImageData).
	 * 
	 * @param app the appearance where to create the Texture2D 
	 * @param shader the name of the shader, should be null, "", "polygonShader" maybe "lineShader" works also
	 * @param in an Input to load the ImageData from
	 * @return a proxy implementation of Texture2D
	 * @throws IOException
	 */
	public static Texture2D createTexture(Appearance app, String shader, Input in, boolean readDefaults) throws IOException {
		return createTexture(app, shader, ImageData.load(in), readDefaults);
	}
	public static Texture2D createTexture(Appearance app, String shader, Input in) throws IOException {
		return createTexture(app, shader, in, true);
	}

	/**
	 * method to create a reflectionMap for the given image (as a String to locate as a resource)
	 * 
	 * @param app the appearance where to create the Texture2D 
	 * @param shader the name of the shader, should be null, "", "polygonShader" maybe "lineShader" works also
	 * @param resourceName
	 * @return a proxy implementation of Texture2D
	 * @throws IOException if the resource fails to load
	 * 
	 * @see Readers.getInput()
	 */
	public static Texture2D createTexture(Appearance app, String shader, String resourceName, boolean readDefaults) throws IOException {
		return createTexture(app, shader, Input.getInput(resourceName), readDefaults);
	}
	public static Texture2D createTexture(Appearance app, String shader, String resourceName) throws IOException {
		return createTexture(app, shader, resourceName, true);
	}

	/************* Reflection Map *****************/

	/**
	 * {"bk","ft","dn","up","lf","rt"}
	 */
	public static final String[] STANDARD_CUBEMAP_PARTS = {"bk","ft","dn","up","lf","rt"};

	/**
	 * method to create a reflectionMap for the 6 given images (as ImageData). The images are assumed to be in the
	 * following order:
	 * 
	 * {"bk","ft","dn","up","lf","rt"}
	 * 
	 * @param app the appearance where to create the ReflectionMap 
	 * @param shader the name of the shader, should be null, "", "polygonShader" maybe "lineShader" works also
	 * @param imgs
	 * @return a proxy implementation of ReflectionMap
	 */
	public static CubeMap createReflectionMap(Appearance app, String shader, ImageData... imgs) {
		String key = (shader == null || shader.equals("")) ? "reflectionMap" : shader+".reflectionMap";
		if (imgs != null) {  
			if (imgs.length != 6) throw new IllegalArgumentException("need 6 images for reflection map");
			return createCubeMap(app, key, imgs);
		} else {
			app.setAttribute(key, Appearance.INHERITED);
			return null;
		}
	}

	/**
	 * method to create a reflectionMap for the 6 given images (as Inputs). The images are assumed to be in the
	 * following order:
	 * 
	 * {"bk","ft","dn","up","lf","rt"}
	 * 
	 * @param app the appearance where to create the ReflectionMap 
	 * @param shader the name of the shader, should be null, "", "polygonShader" maybe "lineShader" works also
	 * @param data an array of 6 images
	 * @return a proxy implementation of ReflectionMap
	 * @throws IOException if data fails to load
	 */
	public static CubeMap createReflectionMap(Appearance app, String shader, Input[] data) throws IOException {
		ImageData[] imgs = new ImageData[data.length];
		for(int i = 0; i < imgs.length; i++)
			imgs[i] = ImageData.load(data[i]);
		return createReflectionMap(app, shader, imgs);
	}

	/**
	 * Convienience Method to load all 6 images for a ReflectionMap. The 6 images are created
	 * by looking for the resources named 
	 * <pre>resourcePrefix+sides[i=0..5]+resourceEnding</pre>
	 * 
	 * The order of the 6 images is the same as for the other ReflectionMap methods: {"bk","ft","dn","up","lf","rt"}
	 *
	 * @param app the appearance where to create the ReflectionMap 
	 * @param shader the name of the shader, should be null, "", "polygonShader" maybe "lineShader" works also
	 * @param resourcePrefix part of the resource names
	 * @param sides part of the resource names
	 * @param resourceEnding part of the resource names
	 * @return a proxy implementation of ReflectionMap
	 * @throws IOException if the resources fail to load
	 */
	public static CubeMap createReflectionMap(Appearance app, String shader, String resourcePrefix, String[] sides, String resourceEnding) throws IOException {
		Input[] data = new Input[sides.length];
		for (int i = 0; i < sides.length; i++) {
			data[i] = Input.getInput(resourcePrefix+sides[i]+resourceEnding);
		}
		return createReflectionMap(app, shader, data);
	}

	/**
	 * method to read the 6 sides of a ReflectionMap. Other than the create methods this method takes
	 * not only the shader name but the full string to the ReflectionMap Entity.
	 * @param app the ea to read from
	 * @param prefix the prefix under which the ReflectionMap is written to the appearance
	 * @return an array of 6 texture2d proxy implementations in the opengl order:  {"bk","ft","dn","up","lf","rt"}
	 */
	public static CubeMap readReflectionMap(EffectiveAppearance app, String prefix) {
		return (CubeMap) AttributeEntityUtility.createAttributeEntity(CubeMap.class, prefix, app);
	}

	public static ImageData[] getCubeMapImages(CubeMap rm) {
		ImageData[] sides = new ImageData[6];
		sides[0] = rm.getBack();
		sides[1] = rm.getFront();
		sides[2] = rm.getBottom();
		sides[3] = rm.getTop();
		sides[4] = rm.getLeft();
		sides[5] = rm.getRight();
		return sides;
	}

	/**
	 * method to create a reflectionMap for the 6 given images (as ImageData). The images are assumed to be in the
	 * following order:
	 * 
	 * {"bk","ft","dn","up","lf","rt"}
	 * 
	 * @param app the appearance where to create the ReflectionMap 
	 * @param prefix the prefix, i. e. "polygonShader.reflectionMap" or "skyBox", ...
	 * @param imgs
	 * @return a proxy implementation of ReflectionMap
	 */
	public static CubeMap createCubeMap(final Appearance app, final String prefix, final ImageData[] imgs) {
		if (imgs.length != 6) throw new IllegalArgumentException("need 6 images for reflection map");
		final CubeMap[] cm=new CubeMap[1];
		Scene.executeWriter(app, new Runnable() {
			public void run() {
				CubeMap ret = (CubeMap) AttributeEntityUtility.createAttributeEntity(CubeMap.class, prefix, app, true);
				ret.setBack(imgs[0]);
				ret.setFront(imgs[1]);
				ret.setBottom(imgs[2]);
				ret.setTop(imgs[3]);
				ret.setLeft(imgs[4]);
				ret.setRight(imgs[5]);
				cm[0]=ret;
			}
		});
		return cm[0];
	}

	/***************************** Sky box **************************/

	/**
	 * method to create a sky box for the 6 given images (as ImageData). The images are assumed to be in the
	 * following order:
	 * 
	 * {"bk","ft","dn","up","lf","rt"}
	 * 
	 * @param app the appearance where to create the sky box 
	 * @param imgs
	 * @return a proxy implementation of ReflectionMap
	 */
	public static CubeMap createSkyBox(Appearance app, ImageData[] imgs) {
		if (imgs != null) {
			if (imgs.length != 6) throw new IllegalArgumentException("need 6 images for reflection map");
			return createCubeMap(app, CommonAttributes.SKY_BOX, imgs);
		} else {
			app.setAttribute(CommonAttributes.SKY_BOX, Appearance.INHERITED);
			return null;
		}
	}

	/**
	 * method to create a sky box for the 6 given images (as Inputs). The images are assumed to be in the
	 * following order:
	 * 
	 * {"bk","ft","dn","up","lf","rt"}
	 * 
	 * @param app the appearance where to create the sky box 
	 * @param data an array of 6 images
	 * @return a proxy implementation of ReflectionMap
	 * @throws IOException if data fails to load
	 */
	public static CubeMap createSkyBox(Appearance app, Input[] data) throws IOException {
		ImageData[] imgs = new ImageData[data.length];
		for(int i = 0; i < imgs.length; i++)
			imgs[i] = ImageData.load(data[i]);
		return createSkyBox(app, imgs);
	}

	/**
	 * Convienience Method to load all 6 images for a ReflectionMap. The 6 images are created
	 * by looking for the resources named 
	 * <pre>resourcePrefix+sides[i=0..5]+resourceEnding</pre>
	 * 
	 * The order of the 6 images is the same as for the other ReflectionMap methods: {"bk","ft","dn","up","lf","rt"}
	 *
	 * @param app the appearance where to create the sky box 
	 * @param resourcePrefix part of the resource names
	 * @param sides part of the resource names
	 * @param resourceEnding part of the resource names
	 * @return a proxy implementation of ReflectionMap
	 * @throws IOException if the resources fail to load
	 */
	public static CubeMap createCubeMap(Appearance app, String prefix, String resourcePrefix, String[] sides, String resourceEnding) throws IOException {
		return createCubeMap(app, prefix, createCubeMapData(resourcePrefix, sides, resourceEnding));
	}

	/**
	 * Convienience Method to load 6 images for a CubeMap.
	 * The 6 images are created by looking for the resources named 
	 * <pre>resourcePrefix+sides[i=0..5]+resourceEnding</pre>
	 * 
	 * The order of the 6 images is the same as for the other
	 * CubeMap methods: {"bk","ft","dn","up","lf","rt"}
	 *
	 * @param resourcePrefix part of the resource names
	 * @param sides part of the resource names
	 * @param resourceEnding part of the resource names
	 * @return an array of length 6 containing all <code>ImageData</code>s
	 * @throws IOException if the resources fail to load
	 */
	public static ImageData[] createCubeMapData(String resourcePrefix, String[] sides, String resourceEnding) throws IOException {
		Input[] data = new Input[sides.length];
		ImageData[] imgs = new ImageData[data.length];
		for (int i = 0; i < sides.length; i++) {
			data[i] = Input.getInput(resourcePrefix+sides[i]+resourceEnding);
			imgs[i] = ImageData.load(data[i]);
		}
		return imgs;
	}

	public static ImageData[] createCubeMapData(Input zipFile) throws IOException {
		ZipInputStream zis = new ZipInputStream(zipFile.getInputStream());
		ImageData[] id = new ImageData[6];
		for (ZipEntry ze = zis.getNextEntry(); ze != null; ze = zis.getNextEntry()) {
			if (ze.isDirectory()) continue;
			int index = findIndex(ze.getName());
			if (index != -1) {
				if (ze.getName().toLowerCase().endsWith(".tga")) {
					id[index]=new ImageData(TargaFile.getBufferedImage(zis));
				}
				else {
					id[index]=ImageData.load(Input.getInput("zip entry", zis));
				}
			}
		}
		return id;
	}

	
	private static int[] order = new int[]{5,4,2,3,1,0};  //Quake order
//	private static int[] order = new int[]{1,0,2,3,4,5};  //ViewerVR environments
	
	private static int findIndex(String name) {
		//createCubeMap method requires order "bk","ft","up","dn","lf","rt"

		if (name.contains("posx") || name.contains("positive_x")) return 0;
		if (name.contains("negx") || name.contains("negative_x")) return 1;
		if (name.contains("posy") || name.contains("positive_y")) return 2;
		if (name.contains("negy") || name.contains("negative_y")) return 3;
		if (name.contains("posz") || name.contains("positive_z")) return 4;
		if (name.contains("negz") || name.contains("negative_z")) return 5;

		// Quake order => ft,bk,up,dn,rt,lf => working with nebula.zip and other zip-files in testData3D
		// ViewerVR environments => "rt,lf,up,dn,bk,ft" => working with jpg files (snow, desert)

		if (name.contains("lf.")) return order[0];
		if (name.contains("rt.")) return order[1];
		if (name.contains("up.")) return order[2];
		if (name.contains("dn.")) return order[3];
		if (name.contains("bk.")) return order[4];
		if (name.contains("ft.")) return order[5];
		
		return -1;
	}

	
//	TODO: colors seem to have no effect!
	public static ImageData createPointSprite(int textureSize, double[] lightDirection, Color diffuseColor, Color specularColor, double specularExponent) {
		if (lightDirection == null) lightDirection = new double[]{1,-1,2};
		double[][] sphereVertices = new double[textureSize * textureSize][3];
		double x,y,z;
		int I = 0, II = 0;
		for (int i = 0; i<textureSize; ++i) {
			y = 2*(i+.5)/textureSize - 1.0;
			for (int j = 0; j< textureSize; ++j)  {
				x = 2*(j+.5)/textureSize - 1.0;
				double dsq = x*x+y*y;
				if (dsq <= 1.0) { 
					z = Math.sqrt(1.0-dsq);
					sphereVertices[I][0] = x; sphereVertices[I][1] = y; sphereVertices[I][2] = z;
				}
				else sphereVertices[I][0] = sphereVertices[I][1] = sphereVertices[I][2] = -1;
				I++;
			}
		}
		float[] diffuseColorAsFloat = diffuseColor.getColorComponents(null);
		float[] specularColorAsFloat = specularColor.getColorComponents(null);
		double[] reflected = new double[3];
		//System.out.println("specular color is "+specularColor.toString());
		byte[] sphereTex = new byte[textureSize * textureSize * 4];
		for (int i = 0; i<textureSize; ++i) {
			for (int j = 0; j< textureSize; ++j)  {
				if (sphereVertices[I][0] != -1) { 
					double diffuse = Rn.innerProduct(lightDirection, sphereVertices[I]);
					if (diffuse < 0) diffuse = 0;
					if (diffuse > 1.0) diffuse =1.0;
					z = sphereVertices[I][2];
					reflected[0] = 2*sphereVertices[I][0]*z;
					reflected[1] = 2*sphereVertices[I][1]*z;
					reflected[2] = 2*z*z-1;
					double specular = Rn.innerProduct(lightDirection, reflected);
					if (specular < 0.0) specular = 0.0;
					if (specular > 1.0) specular = 1.0;
					specular = Math.pow(specular, specularExponent);
					for (int k = 0; k<3; ++k) {
						double f = (diffuse * diffuseColorAsFloat[k] + specular * specularColorAsFloat[k]);
						if (f < 0) f = 0;
						if (f > 1) f = 1;
						sphereTex[II+k] =  (byte) (255 * f); 
					}
					sphereTex[II+3] = sphereVertices[I][2] < .1 ? (byte) (2550*sphereVertices[I][2]) : -128;
				} else {
					sphereTex[II] =  sphereTex[II+1] = sphereTex[II+2] = sphereTex[II+3]  = 0;  
				}
				II += 4;
				I++;
			}
		}
		return new ImageData(sphereTex, textureSize, textureSize);
	}

	public static void removeReflectionMap(Appearance app, String shader) {
		String key = (shader == null || shader.equals("")) ? "reflectionMap" : shader+".reflectionMap";
		if (AttributeEntityUtility.hasAttributeEntity(CubeMap.class, key, app)) {
			CubeMap cm = (CubeMap) AttributeEntityUtility.createAttributeEntity(CubeMap.class, key, app, false);
			app.setAttribute(key, Appearance.INHERITED);
			cm.setBack(null);
			cm.setFront(null);
			cm.setBottom(null);
			cm.setTop(null);
			cm.setLeft(null);
			cm.setRight(null);
			cm.setBlendColor(null);
		}
	}

	public static void removeTexture(Appearance app, String shader) {
		String key = (shader == null || shader.equals("")) ? "texture2d" : shader+".texture2d";
		if (AttributeEntityUtility.hasAttributeEntity(Texture2D.class, key, app)) {
			Texture2D tex = (Texture2D) AttributeEntityUtility.createAttributeEntity(Texture2D.class, key, app, false);
			app.setAttribute(key, Appearance.INHERITED);
			tex.setImage(null);
			tex.setTextureMatrix(null);
		}		
	}

	public static void main(String[] args) throws IOException {
		createCubeMapData(Input.getInput("/home/weissman/Desktop/test_cubemap.zip"));
	}

	public static boolean hasReflectionMap(EffectiveAppearance ea, String shader) {
		String key = (shader == null || shader.equals("")) ? "reflectionMap" : shader+".reflectionMap";
		return AttributeEntityUtility.hasAttributeEntity(CubeMap.class, key, ea);
	}

	public static boolean hasReflectionMap(Appearance app, String shader) {
		String key = (shader == null || shader.equals("")) ? "reflectionMap" : shader+".reflectionMap";
		return AttributeEntityUtility.hasAttributeEntity(CubeMap.class, key, app);
	}

	/********** Background image ************/
	
	public static void setBackgroundTexture(Appearance rootApp, ImageData id) {
		createTextureImpl(rootApp, CommonAttributes.BACKGROUND_TEXTURE2D, id, false);
	}
	
	
	public static Texture2D getBackgroundTexture(Appearance rootApp) {
		if (AttributeEntityUtility.hasAttributeEntity(Texture2D.class, CommonAttributes.BACKGROUND_TEXTURE2D, rootApp)) {
			return (Texture2D) AttributeEntityUtility.createAttributeEntity(Texture2D.class, CommonAttributes.BACKGROUND_TEXTURE2D, rootApp, true);
		}
		return null;
	}
	
	
	
}
