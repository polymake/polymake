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

import static de.jreality.shader.CommonAttributes.REFLECTION_MAP;
import static de.jreality.shader.CommonAttributes.SMOOTH_SHADING;
import static de.jreality.shader.CommonAttributes.SMOOTH_SHADING_DEFAULT;
import static de.jreality.shader.CommonAttributes.TEXTURE_2D;
import static de.jreality.shader.CommonAttributes.TEXTURE_2D_1;
import static de.jreality.shader.CommonAttributes.TEXTURE_2D_2;
import static de.jreality.shader.CommonAttributes.TEXTURE_2D_3;
import static de.jreality.shader.CommonAttributes.USE_GLSL;

import java.awt.Color;

import javax.media.opengl.GL;
import javax.media.opengl.GL2;

import de.jreality.jogl.JOGLRenderer;
import de.jreality.jogl.JOGLRendererHelper;
import de.jreality.jogl.JOGLRenderingState;
import de.jreality.math.Pn;
import de.jreality.scene.Appearance;
import de.jreality.scene.Cylinder;
import de.jreality.scene.Geometry;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.Scene;
import de.jreality.scene.Sphere;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.AttributeEntityUtility;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.CubeMap;
import de.jreality.shader.EffectiveAppearance;
import de.jreality.shader.GlslProgram;
import de.jreality.shader.ShaderUtility;
import de.jreality.shader.Texture2D;
import de.jreality.shader.TextureUtility;
import de.jreality.util.LoggingSystem;

/**
 * @author Charles Gunn
 * 
 */
public class DefaultPolygonShader extends AbstractPrimitiveShader implements
		PolygonShader {

	public static final int FRONT_AND_BACK = GL.GL_FRONT_AND_BACK;
	public static final int FRONT = GL.GL_FRONT;
	public static final int BACK = GL.GL_BACK;

	boolean smoothShading = true;
	Texture2D texture2D, texture2D_1, texture2D_2, texture2D_3;
	JOGLTexture2D joglTexture2D, joglTexture2D_1, joglTexture2D_2,
			joglTexture2D_3;
	CubeMap reflectionMap;
	JOGLCubeMap joglCubeMap;
	public DefaultVertexShader vertexShader = new DefaultVertexShader();
	boolean useGLSL = false, oneGLSL = false; // , oneTexturePerImage = false;
	int texUnit = 0, refMapUnit = 0;
	GlslProgram glslProgram;
	static GlslProgram oneGlslProgram;
	transient boolean geometryHasTextureCoordinates = false,
			hasTextures = false, firstTime = true,
			noneuclideanInitialized = false;

	transient de.jreality.shader.DefaultPolygonShader templateShader;
	StandardGLSLShader standard;
	static StandardGLSLShader oneStandard;
	boolean hasStandardGLSL = false;

	public DefaultPolygonShader() {

	}

	public DefaultPolygonShader(de.jreality.shader.DefaultPolygonShader ps) {
		templateShader = ps;
	}

	static int count = 0;

	public void setFromEffectiveAppearance(EffectiveAppearance eap, String name) {
		super.setFromEffectiveAppearance(eap, name);
		smoothShading = eap.getAttribute(
				ShaderUtility.nameSpace(name, SMOOTH_SHADING),
				SMOOTH_SHADING_DEFAULT);
		useGLSL = eap.getAttribute(ShaderUtility.nameSpace(name, USE_GLSL),
				false);
		oneGLSL = eap.getAttribute(ShaderUtility.nameSpace(name, "oneGLSL"),
				false);
		// oneTexturePerImage =
		// eap.getAttribute(ShaderUtility.nameSpace(name,ONE_TEXTURE2D_PER_IMAGE),
		// true);
		joglTexture2D = joglTexture2D_1 = joglTexture2D_2 = joglTexture2D_3 = null;
		joglCubeMap = null;
		hasTextures = false;
		if (AttributeEntityUtility.hasAttributeEntity(Texture2D.class,
				ShaderUtility.nameSpace(name, TEXTURE_2D), eap)) {
			texture2D = (Texture2D) AttributeEntityUtility
					.createAttributeEntity(Texture2D.class, ShaderUtility
							.nameSpace(name, CommonAttributes.TEXTURE_2D), eap);
			joglTexture2D = new JOGLTexture2D(texture2D);
			hasTextures = true;
		}
		if (AttributeEntityUtility.hasAttributeEntity(CubeMap.class,
				ShaderUtility.nameSpace(name, REFLECTION_MAP), eap)) {
			reflectionMap = TextureUtility.readReflectionMap(eap,
					ShaderUtility.nameSpace(name, REFLECTION_MAP));
			joglCubeMap = new JOGLCubeMap(reflectionMap);
			hasTextures = true;
		}
		if (AttributeEntityUtility.hasAttributeEntity(Texture2D.class,
				ShaderUtility.nameSpace(name, TEXTURE_2D_1), eap)) {
			texture2D_1 = (Texture2D) AttributeEntityUtility
					.createAttributeEntity(Texture2D.class,
							ShaderUtility.nameSpace(name, TEXTURE_2D_1), eap);
			joglTexture2D_1 = new JOGLTexture2D(texture2D_1);
			hasTextures = true;
		}

		if (AttributeEntityUtility.hasAttributeEntity(Texture2D.class,
				ShaderUtility.nameSpace(name, TEXTURE_2D_2), eap)) {
			texture2D_2 = (Texture2D) AttributeEntityUtility
					.createAttributeEntity(Texture2D.class,
							ShaderUtility.nameSpace(name, TEXTURE_2D_2), eap);
			joglTexture2D_2 = new JOGLTexture2D(texture2D_2);
			hasTextures = true;
		}

		if (AttributeEntityUtility.hasAttributeEntity(Texture2D.class,
				ShaderUtility.nameSpace(name, TEXTURE_2D_3), eap)) {
			texture2D_3 = (Texture2D) AttributeEntityUtility
					.createAttributeEntity(Texture2D.class,
							ShaderUtility.nameSpace(name, TEXTURE_2D_3), eap);
			joglTexture2D_3 = new JOGLTexture2D(texture2D_3);
			hasTextures = true;
		}

		hasStandardGLSL = false;
		if (useGLSL) {
			int metric = eap.getAttribute(
					ShaderUtility.nameSpace(name, CommonAttributes.METRIC),
					Pn.EUCLIDEAN);
			if (GlslProgram.hasGlslProgram(eap, name)) {
				Appearance app = new Appearance();
				glslProgram = new GlslProgram(app, eap, name);
			} else {
				hasStandardGLSL = true;
				if (metric == Pn.EUCLIDEAN) {
					standard = new EuclideanGLSLShader();
				} else {
					standard = new NoneuclideanGLSLShader();
				}
				if (!oneGLSL || oneGlslProgram == null) {
					oneStandard = standard;
					standard.setFromEffectiveAppearance(eap, name);
					glslProgram = standard.getStandardShader();
					if (oneGLSL)
						oneGlslProgram = glslProgram;
				}
				if (oneGLSL) {
					standard = oneStandard;
					glslProgram = oneGlslProgram;
				}
				// System.err.println("using non euc shader");
			}
		}
		vertexShader.setFromEffectiveAppearance(eap, name);
		geometryHasTextureCoordinates = false;
		firstTime = true;
	}

	public void preRender(JOGLRenderingState jrs) {
		JOGLRenderer jr = jrs.renderer;
		GL2 gl = jr.globalGL;
		if (smoothShading)
			gl.glShadeModel(GL2.GL_SMOOTH);
		else
			gl.glShadeModel(GL2.GL_FLAT);
		jrs.smoothShading = smoothShading;
		int texunitcoords = 0;
		// hasTextures = false;
		if (hasTextures) {
			gl.glPushAttrib(GL2.GL_TEXTURE_BIT);
			texUnit = GL.GL_TEXTURE0;
			Geometry curgeom = jr.renderingState.currentGeometry;
			if (firstTime) // assume geometry stays constant between calls to
							// setFromEffectiveAppearance() ...
				if (curgeom != null
						&& (curgeom instanceof IndexedFaceSet)
						&& ((IndexedFaceSet) curgeom)
								.getVertexAttributes(Attribute.TEXTURE_COORDINATES) != null) {
					geometryHasTextureCoordinates = true;
				}
			if (geometryHasTextureCoordinates) {

				if (joglTexture2D != null) {
					gl.glActiveTexture(GL.GL_TEXTURE0);
					gl.glEnable(GL.GL_TEXTURE_2D);
					Texture2DLoaderJOGL.render(gl, joglTexture2D,
							jrs.oneTexture2DPerImage);
					texUnit++;
					texunitcoords++;
				}
				if (joglTexture2D_1 != null) {
					gl.glActiveTexture(GL.GL_TEXTURE0 + 1);
					gl.glEnable(GL.GL_TEXTURE_2D);
					Texture2DLoaderJOGL.render(gl, joglTexture2D_1,
							jrs.oneTexture2DPerImage);
					texUnit++;
					texunitcoords++;
				}
				if (joglTexture2D_2 != null) {
					gl.glActiveTexture(GL.GL_TEXTURE0 + 2);
					gl.glEnable(GL.GL_TEXTURE_2D);
					Texture2DLoaderJOGL.render(gl, joglTexture2D_2,
							jrs.oneTexture2DPerImage);
					texUnit++;
					texunitcoords++;
				}
				if (joglTexture2D_3 != null) {
					gl.glActiveTexture(GL.GL_TEXTURE0 + 3);
					gl.glEnable(GL.GL_TEXTURE_2D);
					Texture2DLoaderJOGL.render(gl, joglTexture2D_3,
							jrs.oneTexture2DPerImage);
					texUnit++;
					texunitcoords++;
				}
			}
		}

		if (joglCubeMap != null) {
			gl.glActiveTexture(texUnit);
			gl.glEnable(GL.GL_TEXTURE_CUBE_MAP);
			refMapUnit = texUnit;
			Texture2DLoaderJOGL.render(jr, joglCubeMap);
			texUnit++;
		}

		jr.renderingState.texUnitCount = texunitcoords;
		vertexShader.render(jrs);
		if (useGLSL) {
			if (hasStandardGLSL) {
				standard.render(jr);
			} else
				GlslLoader.render(glslProgram, jr);
		}
		firstTime = false;
	}

	public void postRender(JOGLRenderingState jrs) {
		if (!jrs.shadeGeometry)
			return;
		JOGLRenderer jr = jrs.renderer;
		GL2 gl = jrs.renderer.globalGL;
		if (useGLSL) {
			GlslLoader.postRender(glslProgram, gl);
		}
		// for (int i = GL.GL_TEXTURE0; i < GL.GL_TEXTURE0+3; ++i) {
		if (hasTextures) {
			if (joglTexture2D != null) {
				gl.glActiveTexture(GL.GL_TEXTURE0);
				gl.glDisable(GL.GL_TEXTURE_2D);
			}
			if (joglTexture2D_1 != null) {
				gl.glActiveTexture(GL.GL_TEXTURE0 + 1);
				gl.glDisable(GL.GL_TEXTURE_2D);
			}
			if (joglTexture2D_2 != null) {
				gl.glActiveTexture(GL.GL_TEXTURE0 + 2);
				gl.glDisable(GL.GL_TEXTURE_2D);
			}
			if (joglTexture2D_3 != null) {
				gl.glActiveTexture(GL.GL_TEXTURE0 + 3);
				gl.glDisable(GL.GL_TEXTURE_2D);
			}
		}
		// }
		if (joglCubeMap != null) {
			gl.glActiveTexture(refMapUnit);
			gl.glDisable(GL.GL_TEXTURE_CUBE_MAP);
			gl.glDisable(GL2.GL_TEXTURE_GEN_S);
			gl.glDisable(GL2.GL_TEXTURE_GEN_T);
			gl.glDisable(GL2.GL_TEXTURE_GEN_R);
		}
		jr.renderingState.texUnitCount = 0;
		// TODO fix this to return to previous state -- maybe textures NOT
		// active
		if (hasTextures)
			gl.glPopAttrib();
	}

	public boolean providesProxyGeometry() {
		return false;
	}

	static Color[] cdbg = { Color.BLUE, Color.GREEN, Color.YELLOW, Color.RED,
			Color.GRAY, Color.WHITE };

	public void render(final JOGLRenderingState jrs) {
		final Geometry g = jrs.currentGeometry;
		final JOGLRenderer jr = jrs.renderer;
		final boolean useDisplayLists = jrs.useDisplayLists;
		if (jrs.shadeGeometry)
			preRender(jrs);

		// I had to do locking here, seems that jogl backend only locks on the
		// corresponding component...
		// maybe this needs to be done at other places too...?
		if (g != null)
			Scene.executeReader(g, new Runnable() {

				public void run() {
					if (g instanceof Sphere || g instanceof Cylinder) {
						int i = 3;
						int dlist;
						if (g instanceof Sphere) {
							jr.renderingState.polygonCount += 24 * (i * (i + 1) + 3);
							dlist = jr.renderingState.getSphereDisplayLists(i);
						} else {
							jr.renderingState.polygonCount += 4 * Math
									.pow(2, i);
							dlist = jr.renderingState
									.getCylinderDisplayLists(i);
						}
						jr.globalGL.glCallList(dlist);
						displayListsDirty = false;
					} else if (g instanceof IndexedFaceSet) {
						jr.renderingState.polygonCount += ((IndexedFaceSet) g)
								.getNumFaces();
						if (providesProxyGeometry()) {
							if (!useDisplayLists || dListProxy == -1) {
								dListProxy = proxyGeometryFor(jrs);
								displayListsDirty = false;
							}
							jr.globalGL.glCallList(dListProxy);
						} else {
							if (useDisplayLists) {
								if (dList == -1) {
									dList = jr.globalGL.glGenLists(1);
									// LoggingSystem.getLogger(this).fine(" PolygonShader: is "+this+" Allocating new dlist "+dList+" for gl "+jr.globalGL);
									jr.globalGL
											.glNewList(dList, GL2.GL_COMPILE); // _AND_EXECUTE);
									JOGLRendererHelper.drawFaces(jr,
											(IndexedFaceSet) g);
									jr.globalGL.glEndList();
									displayListsDirty = false;
								}
								jr.globalGL.glCallList(dList);
							} else
								JOGLRendererHelper.drawFaces(jr,
										(IndexedFaceSet) g);
						}
					}
				}
			});
	}

	public static void defaultPolygonRender(JOGLRenderingState jrs) {
		Geometry g = jrs.currentGeometry;
		JOGLRenderer jr = jrs.renderer;

		if (g instanceof Sphere || g instanceof Cylinder) {
			int i = 3;
			int dlist;
			if (g instanceof Sphere)
				dlist = jr.renderingState.getSphereDisplayLists(i);
			else
				dlist = jr.renderingState.getCylinderDisplayLists(i);
			jr.globalGL.glCallList(dlist);
		} else if (g instanceof IndexedFaceSet) {
			JOGLRendererHelper.drawFaces(jr, (IndexedFaceSet) g);
		}

	}

	public void flushCachedState(JOGLRenderer jr) {
		LoggingSystem.getLogger(this).fine(
				"PolygonShader: Flushing display lists " + dList + " : "
						+ dListProxy);
		if (dList != -1)
			jr.globalGL.glDeleteLists(dList, 1);
		if (dListProxy != -1)
			jr.globalGL.glDeleteLists(dListProxy, 1);
		dList = dListProxy = -1;
		displayListsDirty = true;
	}
}
