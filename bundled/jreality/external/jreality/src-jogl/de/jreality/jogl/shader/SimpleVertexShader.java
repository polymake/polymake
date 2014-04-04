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

import java.awt.Color;
import java.util.logging.Level;

import javax.media.opengl.GL2;

import de.jreality.jogl.JOGLConfiguration;
import de.jreality.jogl.JOGLRenderer;
import de.jreality.jogl.JOGLRenderingState;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.EffectiveAppearance;
import de.jreality.shader.ShaderUtility;

/**
 * @author gunn
 * 
 */
public class SimpleVertexShader implements VertexShader {
	Color diffuseColor;
	double transparency, diffuseCoefficient;
	float[] diffuseColorAsFloat;
	int frontBack = DefaultPolygonShader.FRONT_AND_BACK;

	public Color getDiffuseColor() {
		return diffuseColor;
	}

	public float[] getDiffuseColorAsFloat() {
		return diffuseColorAsFloat;
	}

	public void setFrontBack(int f) {
		frontBack = f;
	}

	public void postRender(JOGLRenderingState jrs) {
	}

	public void render(JOGLRenderingState jrs) {
		JOGLRenderer jr = jrs.renderer;
		GL2 gl = jr.globalGL;
		// JOGLConfiguration.theLog.log(Level.FINER,"Rendering simple vertex shader");

		if (jr.renderingState.frontBack != frontBack) {
			gl.glColorMaterial(frontBack, GL2.GL_DIFFUSE);
			jr.renderingState.frontBack = frontBack;
		}
		// if (!(OpenGLState.equals(diffuseColorAsFloat,
		// jr.openGLState.diffuseColor, (float) 10E-5))) {
		if (jr.renderingState.transparencyEnabled
				|| diffuseColorAsFloat[3] == 0.0)
			gl.glColor4fv(diffuseColorAsFloat, 0);
		else
			gl.glColor3fv(diffuseColorAsFloat, 0);
		// gl.glColor4fv( diffuseColorAsFloat,0);
		// System.err.println("Setting diffuse color to "+diffuseColor);
		// System.arraycopy(diffuseColorAsFloat, 0, jr.openGLState.diffuseColor,
		// 0, 4);
		// }
	}

	public void setFromEffectiveAppearance(EffectiveAppearance eap, String name) {
		JOGLConfiguration.theLog.log(Level.FINER,
				"Setting simple vertex shader");
		diffuseCoefficient = eap.getAttribute(ShaderUtility.nameSpace(name,
				CommonAttributes.DIFFUSE_COEFFICIENT),
				CommonAttributes.DIFFUSE_COEFFICIENT_DEFAULT);
		diffuseColor = (Color) eap.getAttribute(
				ShaderUtility.nameSpace(name, CommonAttributes.DIFFUSE_COLOR),
				CommonAttributes.DIFFUSE_COLOR_DEFAULT);
		transparency = eap.getAttribute(
				ShaderUtility.nameSpace(name, CommonAttributes.TRANSPARENCY),
				CommonAttributes.TRANSPARENCY_DEFAULT);
		diffuseColor = ShaderUtility.combineDiffuseColorWithTransparency(
				diffuseColor, transparency,
				JOGLRenderingState.useOldTransparency);
		diffuseColorAsFloat = diffuseColor.getRGBComponents(null);
	}
}
