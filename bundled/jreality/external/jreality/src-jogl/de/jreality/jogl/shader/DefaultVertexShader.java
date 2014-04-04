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

import javax.media.opengl.GL2;

import de.jreality.jogl.JOGLRenderer;
import de.jreality.jogl.JOGLRenderingState;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.EffectiveAppearance;
import de.jreality.shader.ShaderUtility;

/**
 * @author gunn
 * 
 */
public class DefaultVertexShader {
	public Color ambientColor, diffuseColor, specularColor;
	public double specularExponent, ambientCoefficient, diffuseCoefficient,
			specularCoefficient, transparency;
	public float[] specularColorAsFloat, ambientColorAsFloat,
			diffuseColorAsFloat;

	public DefaultVertexShader() {
		super();
	}

	public void setFromEffectiveAppearance(EffectiveAppearance eap, String name) {
		specularExponent = eap.getAttribute(ShaderUtility.nameSpace(name,
				CommonAttributes.SPECULAR_EXPONENT),
				CommonAttributes.SPECULAR_EXPONENT_DEFAULT);
		ambientCoefficient = eap.getAttribute(ShaderUtility.nameSpace(name,
				CommonAttributes.AMBIENT_COEFFICIENT),
				CommonAttributes.AMBIENT_COEFFICIENT_DEFAULT);
		diffuseCoefficient = eap.getAttribute(ShaderUtility.nameSpace(name,
				CommonAttributes.DIFFUSE_COEFFICIENT),
				CommonAttributes.DIFFUSE_COEFFICIENT_DEFAULT);
		specularCoefficient = eap.getAttribute(ShaderUtility.nameSpace(name,
				CommonAttributes.SPECULAR_COEFFICIENT),
				CommonAttributes.SPECULAR_COEFFICIENT_DEFAULT);
		ambientColor = (Color) eap.getAttribute(
				ShaderUtility.nameSpace(name, CommonAttributes.AMBIENT_COLOR),
				CommonAttributes.AMBIENT_COLOR_DEFAULT);
		ambientColorAsFloat = ambientColor.getRGBComponents(null);
		diffuseColor = (Color) eap.getAttribute(
				ShaderUtility.nameSpace(name, CommonAttributes.DIFFUSE_COLOR),
				CommonAttributes.DIFFUSE_COLOR_DEFAULT);
		// System.err.println("dc = "+diffuseColor);
		transparency = eap.getAttribute(
				ShaderUtility.nameSpace(name, CommonAttributes.TRANSPARENCY),
				CommonAttributes.TRANSPARENCY_DEFAULT);
		// JOGLConfiguration.theLog.log(Level.INFO,"Name is "+name+" transparency is "+transparency);
		diffuseColor = ShaderUtility.combineDiffuseColorWithTransparency(
				diffuseColor, transparency,
				JOGLRenderingState.useOldTransparency);
		diffuseColorAsFloat = diffuseColor.getRGBComponents(null);
		specularColor = (Color) eap.getAttribute(
				ShaderUtility.nameSpace(name, CommonAttributes.SPECULAR_COLOR),
				CommonAttributes.SPECULAR_COLOR_DEFAULT);
		specularColorAsFloat = specularColor.getRGBComponents(null);
		for (int i = 0; i < 3; ++i) {
			ambientColorAsFloat[i] *= (float) ambientCoefficient;
			diffuseColorAsFloat[i] *= (float) diffuseCoefficient;
			specularColorAsFloat[i] *= (float) specularCoefficient;
		}
	}

	public void render(JOGLRenderingState jrs) {
		JOGLRenderer jr = jrs.renderer;
		GL2 gl = jr.globalGL;
		int frontBack = jrs.frontBack;
		// if (jr.openGLState.frontBack != frontBack) {
		gl.glColorMaterial(frontBack, GL2.GL_DIFFUSE);
		// jr.openGLState.frontBack = frontBack;
		// }
		// if (!(OpenGLState.equals(diffuseColorAsFloat,
		// jr.openGLState.diffuseColor, (float) 10E-5))) {
		if (jr.renderingState.transparencyEnabled
				|| diffuseColorAsFloat[3] == 0.0)
			gl.glColor4fv(diffuseColorAsFloat, 0);
		else
			gl.glColor3fv(diffuseColorAsFloat, 0);
		System.arraycopy(diffuseColorAsFloat, 0,
				jr.renderingState.diffuseColor, 0, 4);
		jrs.currentAlpha = jrs.diffuseColor[3];
		// }
		// gl.glMaterialfv(frontBack, GL.GL_DIFFUSE, diffuseColorAsFloat);
		gl.glMaterialfv(frontBack, GL2.GL_AMBIENT, ambientColorAsFloat, 0);
		gl.glMaterialfv(frontBack, GL2.GL_SPECULAR, specularColorAsFloat, 0);
		gl.glMaterialf(frontBack, GL2.GL_SHININESS, (float) specularExponent);
		// LoggingSystem.getLogger(this).finest("VertexShader: Setting diffuse color to: "+Rn.toString(getDiffuseColorAsFloat()));
	}

	public void postRender(JOGLRenderingState jrs) {
	}

}
