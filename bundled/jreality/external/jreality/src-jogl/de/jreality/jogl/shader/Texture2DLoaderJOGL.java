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

package de.jreality.jogl.shader;

import java.awt.Dimension;
import java.awt.image.BufferedImage;
import java.awt.image.DataBuffer;
import java.awt.image.DataBufferByte;
import java.awt.image.DataBufferDouble;
import java.awt.image.DataBufferFloat;
import java.awt.image.DataBufferInt;
import java.awt.image.DataBufferShort;
import java.awt.image.DataBufferUShort;
import java.lang.ref.ReferenceQueue;
import java.lang.ref.WeakReference;
import java.nio.Buffer;
import java.nio.ByteBuffer;
import java.nio.FloatBuffer;
import java.nio.IntBuffer;
import java.nio.ShortBuffer;
import java.util.IdentityHashMap;
import java.util.List;
import java.util.Vector;
import java.util.WeakHashMap;

import javax.media.opengl.GL;
import javax.media.opengl.GL2;
import javax.media.opengl.glu.gl2.GLUgl2;

import de.jreality.jogl.JOGLRenderer;
import de.jreality.math.Rn;
import de.jreality.shader.CubeMap;
import de.jreality.shader.ImageData;
import de.jreality.shader.Texture2D;
import de.jreality.util.LoggingSystem;

/**
 * Manages mapping Texture2D and CubeMap to GL textures and loading. Needs the
 * following improvements: - mapping from WeakReference<ImageData> to multiple
 * GL objects - garbage collection for cube map textures.
 * 
 * @author Charles Gunn, Steffen Weissmann
 */
public class Texture2DLoaderJOGL {
	private static WeakHashMap<GL, WeakHashMap<ImageData, Integer>> lookupTextures = new WeakHashMap<GL, WeakHashMap<ImageData, Integer>>();
	private static WeakHashMap<GL, WeakHashMap<ImageData, Integer>> lookupCubemaps = new WeakHashMap<GL, WeakHashMap<ImageData, Integer>>();
	private static WeakHashMap<GL, List<ImageData>> animatedTextures = new WeakHashMap<GL, List<ImageData>>();

	private static ReferenceQueue<ImageData> refQueue = new ReferenceQueue<ImageData>();
	private static IdentityHashMap<WeakReference<ImageData>, Integer> refToID = new IdentityHashMap<WeakReference<ImageData>, Integer>();
	private static IdentityHashMap<WeakReference<ImageData>, GL> refToGL = new IdentityHashMap<WeakReference<ImageData>, GL>();
	private static IdentityHashMap<WeakReference<ImageData>, Dimension> refToDim = new IdentityHashMap<WeakReference<ImageData>, Dimension>();

	private static final boolean REPLACE_TEXTURES = true;
	private static WeakHashMap<GL, List<Integer>> fboTextures = new WeakHashMap<GL, List<Integer>>();

	private Texture2DLoaderJOGL() {
	}

	private static int createTextureID(GL gl) {
		int[] tmp = new int[1];
		gl.glGenTextures(1, tmp, 0);
		return tmp[0];
	}

	private static List<Integer> getFBOTexturesForGL(GL gl) {
		List<Integer> list = fboTextures.get(gl);
		if (list == null) {
			list = new Vector<Integer>();
			fboTextures.put(gl, list);
		}
		return list;
	}

	private static WeakHashMap<ImageData, Integer> getTextureTableForGL(GL gl) {
		WeakHashMap<ImageData, Integer> ht = lookupTextures.get(gl);
		if (ht == null) {
			ht = new WeakHashMap<ImageData, Integer>();
			lookupTextures.put(gl, ht);
		}
		return ht;
	}

	private static WeakHashMap<ImageData, Integer> getCubeMapTableForGL(GL gl) {
		WeakHashMap<ImageData, Integer> ht = lookupCubemaps.get(gl);
		if (ht == null) {
			ht = new WeakHashMap<ImageData, Integer>();
			lookupCubemaps.put(gl, ht);
		}
		return ht;
	}

	public static void clearAnimatedTextureTable(GL gl) {
		if (animatedTextures.get(gl) == null) {
			animatedTextures.put(gl, new Vector<ImageData>());
			return;
		}
		animatedTextures.get(gl).clear();
		lastRendered = null;
	}

	/******************* new Textures *******************/
	public static void render(GL2 gl, Texture2D tex) {
		JOGLTexture2D jogltex = new JOGLTexture2D(tex);
		render(gl, jogltex, false);
	}

	public static void render(GL2 gl, JOGLTexture2D tex) {
		render(gl, tex, false);
	}

	static Texture2D lastRendered = null;
	static boolean haveAutoMipmapGeneration, haveCheckedAutoMipmapGeneration;

	public static void render(GL2 gl, JOGLTexture2D tex,
			boolean oneTexturePerImage) {
		// System.err.println("rendering texture length "+tex.getImage().getByteArray().length);

		// can't do this statically at start-up since we need a GL context to
		// inquire
		checkForTextureExtensions(gl);
		boolean first = true;
		boolean fbo = false;
		boolean replace = false;

		WeakHashMap<ImageData, Integer> ht = getTextureTableForGL(gl);

		ImageData image = tex.getImage();
		Integer texid = null;
		int width = 0, height = 0;
		// hack for fbo generated texture id's
		if (tex.getTexID() != -1) {
			texid = tex.getTexID();
			fbo = true;
			// System.err.println("Got texid "+texid);
		} else {
			if (image == null)
				return;
			width = image.getWidth();
			height = image.getHeight();
			texid = ht.get(image);
		}
		// System.err.println("texid = "+texid);
		if (texid != null) {
			first = false;
		} else {
			// List<Integer> fbotextures = getFBOTexturesForGL(gl);

			Dimension dim = new Dimension(width, height);
			{ // delete garbage collected textures or reuse if possible
				for (Object ref = refQueue.poll(); ref != null; ref = refQueue
						.poll()) {
					Integer id = (Integer) refToID.remove(ref);
					if (id == null)
						throw new Error();
					GL g = (GL) refToGL.remove(ref);
					Dimension d = (Dimension) refToDim.remove(ref);
					if (REPLACE_TEXTURES && g == gl && dim.equals(d)
							&& !replace) {
						// replace texture
						LoggingSystem.getLogger(Texture2DLoaderJOGL.class)
								.fine("replacing texture...");
						texid = id;
						replace = true;
						first = false;
					} else {
						LoggingSystem.getLogger(Texture2DLoaderJOGL.class)
								.fine("deleted texture...");
						g.glDeleteTextures(1, new int[] { id.intValue() }, 0);
					}
				}
				LoggingSystem.getLogger(Texture2DLoaderJOGL.class).fine(
						"creating texture... ");
			}
			// create the texture ID for this texture
			if (texid == null) {
				texid = createTextureID(gl);
				ht.put(image, texid);
			}
			// register reference for refQueue
			WeakReference<ImageData> ref = new WeakReference<ImageData>(image,
					refQueue);
			refToID.put(ref, texid);
			refToGL.put(ref, gl);
			refToDim.put(ref,
					new Dimension(image.getWidth(), image.getHeight()));
		}

		gl.glBindTexture(GL.GL_TEXTURE_2D, texid);
		gl.glMatrixMode(GL.GL_TEXTURE);
		gl.glLoadTransposeMatrixd(tex.getTextureMatrix().getArray(), 0);
		gl.glMatrixMode(GL2.GL_MODELVIEW);
		int srcPixelFormat = tex.getPixelFormat();
		boolean animated = tex.getAnimated();
		if (animated) {
			List<ImageData> list = animatedTextures.get(gl);
			if (list == null) {
				list = new Vector<ImageData>();
				animatedTextures.put(gl, list);
			}
			boolean done = animatedTextures.get(gl).contains(image);
			if (!done) {
				// System.err.println("updating animated texture id:" + texid);
				Runnable r = tex.getRunnable();
				if (r != null) {
					r.run();
				}
				gl.glPixelStorei(GL2.GL_UNPACK_ROW_LENGTH, image.getWidth());
				gl.glPixelStorei(GL2.GL_UNPACK_SKIP_ROWS, 0);
				gl.glPixelStorei(GL2.GL_UNPACK_SKIP_PIXELS, 0);

				// System.err.println("image size: "+image.getWidth()+":"+image.getHeight());
				DataBuffer dataBuffer = ((BufferedImage) image.getImage())
						.getRaster().getDataBuffer();
				Buffer buffer = null;
				if (tex.getTexID() == -1) {
					if (dataBuffer instanceof DataBufferByte) {
						buffer = ByteBuffer.wrap(((DataBufferByte) dataBuffer)
								.getData());
					} else if (dataBuffer instanceof DataBufferDouble) {
						throw new RuntimeException(
								"DataBufferDouble rasters not supported by OpenGL");
					} else if (dataBuffer instanceof DataBufferFloat) {
						buffer = FloatBuffer
								.wrap(((DataBufferFloat) dataBuffer).getData());
					} else if (dataBuffer instanceof DataBufferInt) {
						buffer = IntBuffer.wrap(((DataBufferInt) dataBuffer)
								.getData());
					} else if (dataBuffer instanceof DataBufferShort) {
						buffer = ShortBuffer
								.wrap(((DataBufferShort) dataBuffer).getData());
					} else if (dataBuffer instanceof DataBufferUShort) {
						buffer = ShortBuffer
								.wrap(((DataBufferUShort) dataBuffer).getData());
					} else {
						throw new RuntimeException(
								"Unexpected DataBuffer type?");
					}
					gl.glTexSubImage2D(GL.GL_TEXTURE_2D, 0, 0, 0, width,
							height, srcPixelFormat, GL.GL_UNSIGNED_BYTE, buffer);
					// gl.glTexImage2D(GL.GL_TEXTURE_2D, 0, GL.GL_RGBA,
					// width, height, 0, srcPixelFormat,
					// GL.GL_UNSIGNED_BYTE, buffer);
				}

				animatedTextures.get(gl).add(image);
			}
		}

		if ((first || !oneTexturePerImage || lastRendered == null || image != lastRendered
				.getImage())) {
			// System.err.println("rerendering texture id:" + texid);
			// calls to glTexParameter get saved and restored by "bind()" so
			// should be handled separately
			lastRendered = tex;
			if (canFilterAnisotropic) {
				gl.glTexParameterf(GL.GL_TEXTURE_2D,
						GL.GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy.get(0));
			}

			// if (fbo) return;
			gl.glTexParameteri(GL.GL_TEXTURE_2D, GL.GL_TEXTURE_WRAP_S,
					tex.getRepeatS());
			gl.glTexParameteri(GL.GL_TEXTURE_2D, GL.GL_TEXTURE_WRAP_T,
					tex.getRepeatT());
			gl.glTexParameteri(GL.GL_TEXTURE_2D, GL.GL_TEXTURE_MIN_FILTER,
					tex.getMinFilter());
			gl.glTexParameteri(GL.GL_TEXTURE_2D, GL.GL_TEXTURE_MAG_FILTER,
					tex.getMagFilter());
			if (fbo)
				return;
			float[] texcolor = tex.getBlendColor().getRGBComponents(null);
			gl.glTexEnvfv(GL2.GL_TEXTURE_ENV, GL2.GL_TEXTURE_ENV_COLOR,
					texcolor, 0);
			gl.glTexEnvf(GL2.GL_TEXTURE_ENV, GL2.GL_TEXTURE_ENV_MODE,
					tex.getApplyMode());
			if (tex.getApplyMode() == Texture2D.GL_COMBINE) {
				// System.err.println("Combining with alpha "+texcolor[3]);
				gl.glTexEnvf(GL2.GL_TEXTURE_ENV, GL2.GL_COMBINE_RGB,
						tex.getCombineMode());
				gl.glTexEnvf(GL2.GL_TEXTURE_ENV, GL2.GL_SOURCE0_RGB,
						tex.getSource0Color()); // GL.GL_TEXTURE);
				gl.glTexEnvf(GL2.GL_TEXTURE_ENV, GL2.GL_OPERAND0_RGB,
						tex.getOperand0Color()); // GL.GL_SRC_COLOR);
				gl.glTexEnvf(GL2.GL_TEXTURE_ENV, GL2.GL_SOURCE1_RGB,
						tex.getSource1Color()); // GL.GL_PREVIOUS);
				gl.glTexEnvf(GL2.GL_TEXTURE_ENV, GL2.GL_OPERAND1_RGB,
						tex.getOperand1Color()); // GL.GL_SRC_COLOR);
				gl.glTexEnvf(GL2.GL_TEXTURE_ENV, GL2.GL_SOURCE2_RGB,
						tex.getSource2Color()); // GL.GL_CONSTANT);
				gl.glTexEnvf(GL2.GL_TEXTURE_ENV, GL2.GL_OPERAND2_RGB,
						tex.getOperand2Color()); // GL.GL_SRC_ALPHA);
				gl.glTexEnvf(GL2.GL_TEXTURE_ENV, GL2.GL_COMBINE_ALPHA,
						tex.getCombineModeAlpha());
				gl.glTexEnvf(GL2.GL_TEXTURE_ENV, GL2.GL_SOURCE0_ALPHA,
						tex.getSource0Alpha()); // GL.GL_TEXTURE);
				gl.glTexEnvf(GL2.GL_TEXTURE_ENV, GL2.GL_OPERAND0_ALPHA,
						tex.getOperand0Alpha()); // GL.GL_SRC_COLOR);
				gl.glTexEnvf(GL2.GL_TEXTURE_ENV, GL2.GL_SOURCE1_ALPHA,
						tex.getSource1Alpha()); // GL.GL_PREVIOUS);
				gl.glTexEnvf(GL2.GL_TEXTURE_ENV, GL2.GL_OPERAND1_ALPHA,
						tex.getOperand1Alpha()); // GL.GL_SRC_COLOR);
				gl.glTexEnvf(GL2.GL_TEXTURE_ENV, GL2.GL_SOURCE2_ALPHA,
						tex.getSource2Alpha()); // GL.GL_CONSTANT);
				gl.glTexEnvf(GL2.GL_TEXTURE_ENV, GL2.GL_OPERAND2_ALPHA,
						tex.getOperand2Alpha()); // GL.GL_SRC_ALPHA);
			}
			// create either a series of mipmaps of a single texture image based
			// on
			// what's loaded
			boolean mipmapped = tex.getMipmapMode();
			if ((first || replace)) {
				byte[] data = image.getByteArray();
				if (mipmapped) {
					if (haveAutoMipmapGeneration) {
						gl.glPixelStorei(GL2.GL_UNPACK_ROW_LENGTH,
								image.getWidth());
						gl.glPixelStorei(GL2.GL_UNPACK_SKIP_ROWS, 0);
						gl.glPixelStorei(GL2.GL_UNPACK_SKIP_PIXELS, 0);
						gl.glTexParameteri(GL.GL_TEXTURE_2D,
								GL2.GL_GENERATE_MIPMAP, GL.GL_TRUE);
						gl.glTexImage2D(GL.GL_TEXTURE_2D, 0, GL.GL_RGBA, width,
								height, 0, srcPixelFormat, GL.GL_UNSIGNED_BYTE,
								ByteBuffer.wrap(data));
					} else {
						System.err.println("Building mipmaps");
						GLUgl2 glu = new GLUgl2();
						try {
							glu.gluBuild2DMipmaps(GL.GL_TEXTURE_2D,
									GL2.GL_COMPRESSED_RGBA, width, height,
									srcPixelFormat, GL.GL_UNSIGNED_BYTE,
									ByteBuffer.wrap(data));
						} catch (Exception e) {
							e.printStackTrace();
						}
					}
				} else {
					gl.glPixelStorei(GL2.GL_UNPACK_ROW_LENGTH, image.getWidth());
					gl.glPixelStorei(GL2.GL_UNPACK_SKIP_ROWS, 0);
					gl.glPixelStorei(GL2.GL_UNPACK_SKIP_PIXELS, 0);
					gl.glTexParameteri(GL.GL_TEXTURE_2D,
							GL2.GL_GENERATE_MIPMAP, GL.GL_FALSE);
					gl.glTexImage2D(GL.GL_TEXTURE_2D, 0, GL.GL_RGBA,
							image.getWidth(), image.getHeight(), 0,
							srcPixelFormat, GL.GL_UNSIGNED_BYTE,
							ByteBuffer.wrap(data));
				}
			}
		}
	}

	static GLUgl2 glu = null;

	public static void render(JOGLRenderer jr, CubeMap ref) {
		// public static void render(GL gl, CubeMap ref, double[] c2w) {
		boolean first = true;
		boolean mipmapped = true;
		lastRendered = null;
		GL2 gl = jr.globalGL;
		WeakHashMap<ImageData, Integer> ht = getCubeMapTableForGL(gl);
		checkForTextureExtensions(gl);
		// hash one side of the cube map and do only render sides when hashed
		// image data changed
		Integer texid = (Integer) ht.get(ref.getLeft());
		int textureID;
		if (texid != null) {
			first = false;
			textureID = texid.intValue();
		} else {
			// create the texture ID for this texture
			textureID = createTextureID(gl);
			ht.put(ref.getLeft(), new Integer(textureID));
		}
		// System.err.println("Binding cubemap texture for "+textureID);
		gl.glBindTexture(GL.GL_TEXTURE_CUBE_MAP, textureID);

		double[] c2w = Rn.copy(null, jr.renderingState.cameraToWorld);
		c2w[3] = c2w[7] = c2w[11] = 0.0;
		int srcPixelFormat = GL.GL_RGBA;

		gl.glTexParameteri(GL.GL_TEXTURE_2D, GL.GL_TEXTURE_WRAP_S,
				Texture2D.GL_CLAMP_TO_EDGE);
		gl.glTexParameteri(GL.GL_TEXTURE_2D, GL.GL_TEXTURE_WRAP_T,
				Texture2D.GL_CLAMP_TO_EDGE);
		gl.glTexParameteri(GL.GL_TEXTURE_2D, GL.GL_TEXTURE_MIN_FILTER,
				Texture2D.GL_LINEAR_MIPMAP_LINEAR);
		gl.glTexParameteri(GL.GL_TEXTURE_2D, GL.GL_TEXTURE_MAG_FILTER,
				Texture2D.GL_LINEAR);

		float[] texcolor = ref.getBlendColor().getRGBComponents(null);
		gl.glTexEnvfv(GL2.GL_TEXTURE_ENV, GL2.GL_TEXTURE_ENV_COLOR, texcolor, 0);
		gl.glTexEnvf(GL2.GL_TEXTURE_ENV, GL2.GL_TEXTURE_ENV_MODE,
				Texture2D.GL_COMBINE);

		gl.glTexEnvf(GL2.GL_TEXTURE_ENV, GL2.GL_TEXTURE_ENV_MODE,
				GL2.GL_COMBINE);
		gl.glTexEnvf(GL2.GL_TEXTURE_ENV, GL2.GL_COMBINE_RGB,
				Texture2D.COMBINE_MODE_DEFAULT);
		gl.glTexEnvf(GL2.GL_TEXTURE_ENV, GL2.GL_SOURCE0_RGB, GL.GL_TEXTURE);
		gl.glTexEnvf(GL2.GL_TEXTURE_ENV, GL2.GL_OPERAND0_RGB, GL.GL_SRC_COLOR);
		gl.glTexEnvf(GL2.GL_TEXTURE_ENV, GL2.GL_SOURCE1_RGB, GL2.GL_PREVIOUS);
		gl.glTexEnvf(GL2.GL_TEXTURE_ENV, GL2.GL_OPERAND1_RGB, GL.GL_SRC_COLOR);
		gl.glTexEnvf(GL2.GL_TEXTURE_ENV, GL2.GL_SOURCE2_RGB, GL2.GL_CONSTANT);
		gl.glTexEnvf(GL2.GL_TEXTURE_ENV, GL2.GL_OPERAND2_RGB, GL.GL_SRC_ALPHA);

		gl.glMatrixMode(GL.GL_TEXTURE);
		gl.glLoadTransposeMatrixd(c2w, 0);
		gl.glMatrixMode(GL2.GL_MODELVIEW);

		gl.glTexParameteri(GL.GL_TEXTURE_CUBE_MAP, GL2.GL_TEXTURE_WRAP_R,
				Texture2D.GL_CLAMP_TO_EDGE);
		gl.glTexParameteri(GL.GL_TEXTURE_CUBE_MAP, GL.GL_TEXTURE_WRAP_S,
				Texture2D.GL_CLAMP_TO_EDGE);
		gl.glTexParameteri(GL.GL_TEXTURE_CUBE_MAP, GL.GL_TEXTURE_WRAP_T,
				Texture2D.GL_CLAMP_TO_EDGE);

		gl.glTexGeni(GL2.GL_S, GL2.GL_TEXTURE_GEN_MODE, GL2.GL_REFLECTION_MAP);
		gl.glTexGeni(GL2.GL_T, GL2.GL_TEXTURE_GEN_MODE, GL2.GL_REFLECTION_MAP);
		gl.glTexGeni(GL2.GL_R, GL2.GL_TEXTURE_GEN_MODE, GL2.GL_REFLECTION_MAP);
		gl.glEnable(GL2.GL_TEXTURE_GEN_S);
		gl.glEnable(GL2.GL_TEXTURE_GEN_T);
		gl.glEnable(GL2.GL_TEXTURE_GEN_R);
		gl.glEnable(GL.GL_TEXTURE_CUBE_MAP);

		// create either a series of mipmaps of a single texture image based on
		// what's loaded
		if (first) {
			ImageData[] faces = new ImageData[6];
			faces[0] = ref.getBack();
			faces[1] = ref.getFront();
			faces[2] = ref.getBottom();
			faces[3] = ref.getTop();
			faces[4] = ref.getLeft();
			faces[5] = ref.getRight();
			for (int i = 0; i < 6; ++i) {
				byte[] data = faces[i].getByteArray();
				int width = faces[i].getWidth();
				int height = faces[i].getHeight();
				gl.glPixelStorei(GL2.GL_UNPACK_ROW_LENGTH, width);
				gl.glPixelStorei(GL2.GL_UNPACK_SKIP_ROWS, 0);
				gl.glPixelStorei(GL2.GL_UNPACK_SKIP_PIXELS, 0);
				if (glu == null)
					glu = new GLUgl2();
				if (mipmapped)
					// this doesn't work on my ATI card -cgg March, 2012
					if (false && haveAutoMipmapGeneration) {
						gl.glTexParameteri(GL.GL_TEXTURE_CUBE_MAP_POSITIVE_X
								+ i, GL2.GL_GENERATE_MIPMAP, GL.GL_TRUE);
						gl.glTexImage2D(GL.GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
								0, GL2.GL_COMPRESSED_RGBA, width, height, 0,
								srcPixelFormat, GL.GL_UNSIGNED_BYTE,
								ByteBuffer.wrap(data));
					} else {
						glu.gluBuild2DMipmaps(GL.GL_TEXTURE_CUBE_MAP_POSITIVE_X
								+ i, GL.GL_RGBA, width, height, srcPixelFormat,
								GL.GL_UNSIGNED_BYTE, ByteBuffer.wrap(data));
					}
				else
					gl.glTexImage2D(GL.GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0,
							GL2.GL_COMPRESSED_RGBA, width, height, 0,
							srcPixelFormat, GL.GL_UNSIGNED_BYTE,
							ByteBuffer.wrap(data));

			}
		}
	}

	private static FloatBuffer maxAnisotropy = FloatBuffer.allocate(1);
	private static boolean canFilterAnisotropic = false,
			haveCheckedForAnisotropy = false;

	private static void checkForTextureExtensions(GL gl) {
		if (!haveCheckedForAnisotropy) {
			if (gl.glGetString(GL.GL_EXTENSIONS).contains(
					"GL_EXT_texture_filter_anisotropic")) {
				gl.glGetFloatv(GL.GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT,
						maxAnisotropy);
				canFilterAnisotropic = true;
			} else
				canFilterAnisotropic = false;
			haveCheckedForAnisotropy = true;
		}
		if (!haveCheckedAutoMipmapGeneration) {
			String vendor = gl.glGetString(GL.GL_VENDOR);
			System.err.println("Vendor = " + vendor);
			haveAutoMipmapGeneration = ((gl
					.isExtensionAvailable("GL_VERSION_1_4") || gl
					.isExtensionAvailable("GL_SGIS_generate_mipmap")));
			haveCheckedAutoMipmapGeneration = true;
			System.err.println("Have automipmap generation = "
					+ haveAutoMipmapGeneration);
		}
	}

	public static void deleteAllTextures(GL gl) {
		WeakHashMap<ImageData, Integer> ht = lookupTextures.get(gl);
		if (ht == null)
			return;
		for (int idx : ht.values()) {
			int[] list = new int[] { idx };
			gl.glDeleteTextures(1, list, 0);
		}
		ht.clear();
	}

}
