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

import java.util.logging.Level;

import javax.media.opengl.GL2;

import de.jreality.geometry.IndexedFaceSetUtility;
import de.jreality.jogl.JOGLConfiguration;
import de.jreality.jogl.JOGLRenderer;
import de.jreality.jogl.JOGLRendererHelper;
import de.jreality.jogl.JOGLRenderingState;
import de.jreality.scene.Geometry;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.shader.EffectiveAppearance;
import de.jreality.shader.ShaderUtility;
import de.jreality.util.LoggingSystem;

/**
 * @author gunn
 * 
 */
public class ImplodePolygonShader extends DefaultPolygonShader {
	double implodeFactor;

	public ImplodePolygonShader(de.jreality.shader.DefaultPolygonShader ps) {
		super(ps);
	}

	public void setFromEffectiveAppearance(EffectiveAppearance eap, String name) {
		super.setFromEffectiveAppearance(eap, name);
		implodeFactor = eap.getAttribute(
				ShaderUtility.nameSpace(name, "implodeFactor"), implodeFactor);
		System.err.println("implode factor = " + implodeFactor);
	}

	public double getImplodeFactor() {
		return implodeFactor;
	}

	public boolean providesProxyGeometry() {
		// if (implodeFactor == 0.0) return false;
		return true;
	}

	public int proxyGeometryFor(JOGLRenderingState jrs) {
		final Geometry original = jrs.currentGeometry;
		final JOGLRenderer jr = jrs.renderer;
		final int sig = jrs.currentMetric;
		final boolean useDisplayLists = jrs.useDisplayLists;
		if (!(original instanceof IndexedFaceSet))
			return -1;
		if (dListProxy != -1)
			return dListProxy;
		GL2 gl = jr.globalGL;
		JOGLConfiguration.theLog.log(Level.FINE, this
				+ "Providing proxy geometry " + implodeFactor);
		IndexedFaceSet ifs = IndexedFaceSetUtility.implode(
				(IndexedFaceSet) original, implodeFactor);
		double alpha = vertexShader == null ? 1.0 : jrs.diffuseColor[3];
		if (useDisplayLists) {
			dListProxy = gl.glGenLists(1);
			gl.glNewList(dListProxy, GL2.GL_COMPILE);
		}
		// if (jr.isPickMode()) gl.glPushName(JOGLPickAction.GEOMETRY_BASE);
		JOGLRendererHelper.drawFaces(jr, ifs, jrs.smoothShading, alpha);
		// if (jr.isPickMode()) gl.glPopName();
		if (useDisplayLists)
			gl.glEndList();
		return dListProxy;
	}

	public void flushCachedState(JOGLRenderer jr) {
		super.flushCachedState(jr);
		LoggingSystem.getLogger(this).fine(
				"ImplodePolygonShader: Flushing display lists " + dListProxy
						+ " : " + dListProxy);
		if (dListProxy != -1) {
			jr.globalGL.glDeleteLists(dListProxy, 1);
			dListProxy = -1;
		}
		displayListsDirty = true;
	}
}
