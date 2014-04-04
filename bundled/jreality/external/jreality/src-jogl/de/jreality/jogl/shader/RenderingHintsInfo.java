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
import de.jreality.scene.Appearance;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.DefaultPolygonShader;
import de.jreality.shader.RenderingHintsShader;

/**
 * @author Charles Gunn
 * 
 */
public class RenderingHintsInfo {
	// a number between 0= min and 1=max level and detail.
	double levelOfDetail = RenderingHintsShader.LEVEL_OF_DETAIL_DEFAULT,
			oldLevelOfDetail;
	double depthFudgeFactor = RenderingHintsShader.DEPTH_FUDGE_FACTOR_DEFAULT,
			oldDepthFudgeFactor;
	final static int SINGLE_COLOR = 0x81F9;
	final static int SEPARATE_SPECULAR_COLOR = 0x81FA;
	final static int ACTIVE = 0;
	final static int VALUE = 1;
	final static int OLD = 2;
	final static int TE = 0;
	final static int ZB = 1;
	final static int LE = 2;
	final static int BF = 3;
	final static int DL = 4;
	final static int LL = 5;
	final static int SS = 6;
	final static int IA = 7;
	final static int FN = 8;
	final static int SM = 9;
	final static int FF = 10;
	final static int LD = 11;
	final static int numBooleans = SM + 1;
	boolean[][] values = new boolean[LD + 1][3]; // 0: active, 1: current value,
													// 2: old value
	public boolean hasSomeActiveField = false, merged = false;
	static boolean[] defaultValues = {
			RenderingHintsShader.TRANSPARENCY_ENABLED_DEFAULT,
			RenderingHintsShader.Z_BUFFER_ENABLED_DEFAULT,
			RenderingHintsShader.LIGHTING_ENABLED_DEFAULT,
			RenderingHintsShader.BACK_FACE_CULLING_DEFAULT,
			RenderingHintsShader.ANY_DISPLAY_LISTS_DEFAULT,
			RenderingHintsShader.LOCAL_LIGHT_MODEL_DEFAULT,
			RenderingHintsShader.SEPARATE_SPECULAR_DEFAULT,
			RenderingHintsShader.IGNORE_ALPHA0_DEFAULT,
			RenderingHintsShader.FLIP_NORMALS_DEFAULT,
			DefaultPolygonShader.SMOOTH_SHADING_DEFAULT };
	String[] attributes = {
			CommonAttributes.TRANSPARENCY_ENABLED,
			CommonAttributes.Z_BUFFER_ENABLED,
			CommonAttributes.LIGHTING_ENABLED,
			CommonAttributes.BACK_FACE_CULLING_ENABLED,
			CommonAttributes.ANY_DISPLAY_LISTS,
			CommonAttributes.LOCAL_LIGHT_MODEL,
			CommonAttributes.SEPARATE_SPECULAR_COLOR,
			CommonAttributes.IGNORE_ALPHA0,
			CommonAttributes.FLIP_NORMALS_ENABLED, // Experiment w/ handling
													// "polygonShader.smoothShading"
			CommonAttributes.SMOOTH_SHADING, // or just "smoothShading"
			CommonAttributes.DEPTH_FUDGE_FACTOR,
			CommonAttributes.LEVEL_OF_DETAIL };
	public static RenderingHintsInfo defaultRHInfo = new RenderingHintsInfo();
	static {
		for (int i = 0; i < numBooleans; ++i) {
			defaultRHInfo.values[i][ACTIVE] = true;
			defaultRHInfo.values[i][VALUE] = defaultValues[i];
			defaultRHInfo.values[i][OLD] = defaultValues[i];
		}
		defaultRHInfo.values[LD][ACTIVE] = true;
		defaultRHInfo.values[FF][ACTIVE] = true;
		defaultRHInfo.hasSomeActiveField = true;
		defaultRHInfo.merged = true;
	}

	// TODO make sure that levelOfDetail is handled consistently
	public void setFromAppearance(Appearance ap) {
		// System.err.println("in set from appearance");
		hasSomeActiveField = false;
		for (int i = 0; i < numBooleans; ++i) {
			Object foo = ap.getAttribute(attributes[i], Boolean.class);
			if (foo != Appearance.INHERITED) {
				values[i][VALUE] = (Boolean) foo;
				values[i][ACTIVE] = true;
				hasSomeActiveField = true;
				// System.err.println("Got field "+attributes[i]+" = "+values[i][VALUE]);
			} else {
				values[i][ACTIVE] = false;
			}
		}
		Object foo = ap.getAttribute(attributes[LD], Double.class);
		if (foo != Appearance.INHERITED) {
			levelOfDetail = (Double) foo;
			values[LD][ACTIVE] = true;
			hasSomeActiveField = true;
		} else {
			values[LD][ACTIVE] = false;
		}
		foo = ap.getAttribute(attributes[FF], Double.class);
		if (foo != Appearance.INHERITED) {
			depthFudgeFactor = (Double) foo;
			values[FF][ACTIVE] = true;
			hasSomeActiveField = true;
		} else {
			values[FF][ACTIVE] = false;
		}
		merged = false;
	}

	public void render(JOGLRenderingState jrs, RenderingHintsInfo previous) {
		if (!merged && previous != null) {
			for (int i = 0; i < numBooleans; ++i) {
				if (!values[i][ACTIVE])
					values[i][VALUE] = previous.values[i][VALUE];
			}
			merged = true;
		}
		_render(jrs, VALUE);
	}

	public void postRender(JOGLRenderingState jrs, RenderingHintsInfo previous) {
		for (int i = 0; i < numBooleans; ++i) {
			if (values[i][ACTIVE])
				values[i][OLD] = previous.values[i][VALUE];
		}
		_render(jrs, OLD);
	}

	public void _render(JOGLRenderingState jrs, int which) {
		if (!hasSomeActiveField)
			return;
		JOGLRenderer jr = jrs.renderer;
		GL2 gl = jr.globalGL;
		// if (this == defaultRHInfo)
		// System.err.println("rendering default rhinfo");
		// TODO handle zbuffer enabled correctly; it's a bit tricky
		// since it depends on transparency enabled value
		if (values[TE][ACTIVE]) {
			if (values[TE][which]) {
				gl.glEnable(GL.GL_BLEND);
				if (values[ZB][ACTIVE])
					gl.glDepthMask(values[ZB][which]);
				else
					gl.glDepthMask(false);
				JOGLConfiguration.glBlendFunc(gl);
				jrs.transparencyEnabled = true;
				if (values[ZB][ACTIVE])
					jrs.zbufferEnabled = values[ZB][which];
				else
					jrs.zbufferEnabled = false;
			} else {
				gl.glDepthMask(true);
				gl.glDisable(GL.GL_BLEND);
				jrs.transparencyEnabled = false;
				jrs.zbufferEnabled = true;
			}
			// System.err.println("Setting transp to "+values[TE][which]);
		}
		if (values[LE][ACTIVE]) {
			if (values[LE][which])
				gl.glEnable(GL2.GL_LIGHTING);
			else
				gl.glDisable(GL2.GL_LIGHTING);
			jrs.lighting = values[LE][which];
		}
		if (values[FN][ACTIVE]) {
			if (values[FN][which] != jr.renderingState.flipped) {
				jr.renderingState.flipped = values[FN][which];
				jr.globalGL.glFrontFace(jr.renderingState.flipped ? GL.GL_CW
						: GL.GL_CCW);
				// System.err.println("flipped "+jr.renderingState.flipped);
			}
		}
		if (values[BF][ACTIVE]) {
			if (values[BF][which]) {
				gl.glEnable(GL.GL_CULL_FACE);
				gl.glCullFace(GL.GL_BACK); // jr.renderingState.flipped ?
											// GL.GL_FRONT : GL.GL_BACK);//
				// System.err.println("Culling "+(jr.renderingState.flipped ?
				// "front" : "back"));
			} else
				gl.glDisable(GL.GL_CULL_FACE);
		}
		if (values[IA][ACTIVE]) {
			gl.glAlphaFunc(values[IA][which] ? GL.GL_GREATER : GL.GL_ALWAYS, 0f); // alpha
																					// =
																					// 0
																					// gets
																					// ignored
																					// in
																					// fragment
																					// shader:
																					// cheap
																					// transparency
		}
		if (values[LL][ACTIVE]) {
			gl.glLightModeli(GL2.GL_LIGHT_MODEL_LOCAL_VIEWER,
					values[LL][which] ? GL.GL_TRUE : GL.GL_FALSE);
		}
		if (values[SS][ACTIVE]) {
			gl.glLightModeli(GL2.GL_LIGHT_MODEL_COLOR_CONTROL,
					values[SS][which] ? GL2.GL_SEPARATE_SPECULAR_COLOR
							: GL2.GL_SINGLE_COLOR);
		}
		// if (values[SM][ACTIVE]) {
		// if (values[SM][which]) gl.glShadeModel(GL.GL_SMOOTH);
		// else gl.glShadeModel(GL.GL_FLAT);
		// jr.renderingState.smoothShading = values[SM][which];
		// // System.err.println("SM: Setting ss to "+values[SM][which]);
		// }
		if (values[DL][ACTIVE])
			jr.renderingState.useDisplayLists = values[DL][which];
		if (values[LD][ACTIVE])
			jr.renderingState.levelOfDetail = which == VALUE ? levelOfDetail
					: oldLevelOfDetail;
		if (values[FF][ACTIVE])
			jr.renderingState.depthFudgeFactor = which == VALUE ? depthFudgeFactor
					: oldDepthFudgeFactor;
	}

	public boolean hasSomeActiveField() {
		return hasSomeActiveField;
	}

}
