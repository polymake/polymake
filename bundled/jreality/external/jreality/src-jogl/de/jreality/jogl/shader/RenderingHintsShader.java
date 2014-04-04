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

import javax.media.opengl.GL;
import javax.media.opengl.GL2;

import de.jreality.jogl.JOGLConfiguration;
import de.jreality.jogl.JOGLRenderer;
import de.jreality.jogl.JOGLRenderingState;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.EffectiveAppearance;
import de.jreality.shader.ShaderUtility;

/**
 * @author Charles Gunn
 * @deprecated Use {@link RenderingHintsInfo} instead.
 */
public class RenderingHintsShader {
	double levelOfDetail = 0.0; // a number between 0= min and 1=max level and
								// detail.
	final static int SINGLE_COLOR = 0x81F9;
	final static int SEPARATE_SPECULAR_COLOR = 0x81FA;
	boolean transparencyEnabled = false,
			zBufferEnabled = false, // this only matters when
									// transparencyEnabled == true
			lightingEnabled = true,
			antiAliasingEnabled = false, // do we need this anymore?
			backFaceCullingEnabled = false, flipNormalsEnabled = false,
			useDisplayLists = true, clearColorBuffer = true,
			localLightModel = false, separateSpecularColor = false,
			ignoreAlpha0 = true, componentDisplayLists = false;

	public RenderingHintsShader() {
		super();
	}

	public static RenderingHintsShader createFromEffectiveAppearance(
			EffectiveAppearance eap, String name) {
		RenderingHintsShader drh = new RenderingHintsShader();
		drh.setFromEffectiveAppearance(eap, name);
		return drh;
	}

	public void setFromEffectiveAppearance(EffectiveAppearance eap, String name) {
		lightingEnabled = eap.getAttribute(ShaderUtility.nameSpace(name,
				CommonAttributes.LIGHTING_ENABLED), true);
		transparencyEnabled = eap.getAttribute(ShaderUtility.nameSpace(name,
				CommonAttributes.TRANSPARENCY_ENABLED), false);
		zBufferEnabled = eap.getAttribute(ShaderUtility.nameSpace(name,
				CommonAttributes.Z_BUFFER_ENABLED), false);
		ignoreAlpha0 = eap.getAttribute(
				ShaderUtility.nameSpace(name, CommonAttributes.IGNORE_ALPHA0),
				true);
		antiAliasingEnabled = eap.getAttribute(ShaderUtility.nameSpace(name,
				CommonAttributes.ANTIALIASING_ENABLED), false);
		backFaceCullingEnabled = eap.getAttribute(ShaderUtility.nameSpace(name,
				CommonAttributes.BACK_FACE_CULLING_ENABLED), false);
		flipNormalsEnabled = eap.getAttribute(ShaderUtility.nameSpace(name,
				CommonAttributes.FLIP_NORMALS_ENABLED), false);
		useDisplayLists = eap.getAttribute(ShaderUtility.nameSpace(name,
				CommonAttributes.ANY_DISPLAY_LISTS), true);
		levelOfDetail = eap
				.getAttribute(ShaderUtility.nameSpace(name,
						CommonAttributes.LEVEL_OF_DETAIL),
						CommonAttributes.LEVEL_OF_DETAIL_DEFAULT);
		clearColorBuffer = eap.getAttribute(ShaderUtility.nameSpace(name,
				CommonAttributes.CLEAR_COLOR_BUFFER), true);
		localLightModel = eap.getAttribute(ShaderUtility.nameSpace(name,
				CommonAttributes.LOCAL_LIGHT_MODEL), false);
		separateSpecularColor = eap.getAttribute(ShaderUtility.nameSpace(name,
				CommonAttributes.SEPARATE_SPECULAR_COLOR), false);
	}

	public boolean isLightingEnabled() {
		return lightingEnabled;
	}

	public boolean isTransparencyEnabled() {
		return transparencyEnabled;
	}

	private boolean oldFlipped = flipNormalsEnabled;

	public void render(JOGLRenderingState jrs) {
		JOGLRenderer jr = jrs.renderer;
		GL2 gl = jr.globalGL;
		if (transparencyEnabled) {
			gl.glEnable(GL.GL_BLEND);
			gl.glDepthMask(zBufferEnabled);
			JOGLConfiguration.glBlendFunc(gl);
		} else {
			gl.glDepthMask(true);
			gl.glDisable(GL.GL_BLEND);
		}
		jr.renderingState.transparencyEnabled = transparencyEnabled;
		if (lightingEnabled)
			gl.glEnable(GL2.GL_LIGHTING);
		else
			gl.glDisable(GL2.GL_LIGHTING);

		if (backFaceCullingEnabled) {
			gl.glEnable(GL.GL_CULL_FACE);
			gl.glCullFace(GL.GL_BACK);
		} else
			gl.glDisable(GL.GL_CULL_FACE);

		oldFlipped = jr.renderingState.flipped;
		boolean newf = flipNormalsEnabled ^ oldFlipped;
		// System.err.println("flip = "+flipNormalsEnabled);
		if (oldFlipped != newf) {
			jr.renderingState.flipped = newf;
			jr.globalGL.glFrontFace(jr.renderingState.flipped ? GL.GL_CW
					: GL.GL_CCW);
			System.err.println("Flipping normals");
		}

		jr.renderingState.levelOfDetail = levelOfDetail;
		// if (ignoreAlpha0 != jr.getRenderingState().ignoreAlpha0) {
		gl.glAlphaFunc(ignoreAlpha0 ? GL.GL_GREATER : GL.GL_ALWAYS, 0f); // alpha
																			// =
																			// 0
																			// gets
																			// ignored
																			// in
																			// fragment
																			// shader:
																			// cheap
																			// transparency
		jr.renderingState.ignoreAlpha0 = ignoreAlpha0;
		// }
		if (localLightModel != jr.renderingState.localLightModel) {
			gl.glLightModeli(GL2.GL_LIGHT_MODEL_LOCAL_VIEWER,
					localLightModel ? GL.GL_TRUE : GL.GL_FALSE);
			jr.renderingState.localLightModel = localLightModel;
		}
		if (separateSpecularColor != jr.renderingState.separateSpecularColor) {
			gl.glLightModeli(GL2.GL_LIGHT_MODEL_COLOR_CONTROL,
					separateSpecularColor ? GL2.GL_SEPARATE_SPECULAR_COLOR
							: GL2.GL_SINGLE_COLOR);
			jr.renderingState.separateSpecularColor = separateSpecularColor;
		}
		jr.renderingState.useDisplayLists = useDisplayLists; // ();
																// //useDisplayLists(activeDL,
																// jpc);
	}

	public void postRender(JOGLRenderingState jrs) {
		JOGLRenderer jr = jrs.renderer;
		GL gl = jr.globalGL;
		if (oldFlipped != jr.renderingState.flipped) {
			jr.globalGL.glFrontFace(oldFlipped ? GL.GL_CW : GL.GL_CCW);
			jr.renderingState.flipped = oldFlipped;
		}

		// if (transparencyEnabled) {
		// gl.glDepthMask(true);
		// gl.glDisable(GL.GL_BLEND);
		// }
	}
}
