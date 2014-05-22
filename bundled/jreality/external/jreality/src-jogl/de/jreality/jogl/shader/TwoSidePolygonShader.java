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

import javax.media.opengl.GL;

import de.jreality.jogl.JOGLRenderer;
import de.jreality.jogl.JOGLRenderingState;
import de.jreality.scene.Geometry;
import de.jreality.shader.EffectiveAppearance;
import de.jreality.util.LoggingSystem;

/**
 * @author gunn
 * 
 */
public class TwoSidePolygonShader extends AbstractPrimitiveShader implements
		PolygonShader {
	private PolygonShader front;
	private PolygonShader back;

	public TwoSidePolygonShader() {
		super();
		front = new DefaultPolygonShader();
		back = new DefaultPolygonShader();
	}

	public TwoSidePolygonShader(de.jreality.shader.TwoSidePolygonShader tsps) {
		front = DefaultGeometryShader.createFrom(tsps.getFront());
		// front.setFrontBack(DefaultPolygonShader.FRONT);
		back = DefaultGeometryShader.createFrom(tsps.getFront());
		// back.setFrontBack(DefaultPolygonShader.BACK);
	}

	public void render(JOGLRenderingState jrs) {
		Geometry g = jrs.currentGeometry;
		jrs.currentGeometry = null;
		if (back != null) {
			jrs.frontBack = GL.GL_BACK;
			back.render(jrs);
		}
		jrs.currentGeometry = g;
		if (front != null) {
			jrs.frontBack = GL.GL_FRONT;
			front.render(jrs);
		}
		jrs.frontBack = GL.GL_FRONT_AND_BACK;
	}

	public void postRender(JOGLRenderingState jrs) {
		if (front != null)
			front.postRender(jrs);
		if (back != null)
			back.postRender(jrs);
		displayListsDirty = front.displayListsDirty()
				|| back.displayListsDirty();
	}

	public void setFromEffectiveAppearance(EffectiveAppearance eap,
			String shaderName) {
		// front = (PolygonShader) ShaderLookup.getShaderAttr(eap, shaderName,
		// CommonAttributes.POLYGON_SHADER, "front");
		// LoggingSystem.getLogger(this).log(Level.FINER,"Front shader is "+front.getClass().toString());
		// front.setFrontBack(DefaultPolygonShader.FRONT);
		// back = (PolygonShader) ShaderLookup.getShaderAttr(eap,
		// shaderName,CommonAttributes.POLYGON_SHADER, "back");
		// back.setFrontBack(DefaultPolygonShader.BACK);
		front.setFromEffectiveAppearance(eap, shaderName + ".front");
		back.setFromEffectiveAppearance(eap, shaderName + ".back");
	}

	public boolean providesProxyGeometry() {
		LoggingSystem.getLogger(this).log(Level.FINER,
				"Front has proxy: " + front.providesProxyGeometry());
		if (front != null)
			return front.providesProxyGeometry();
		return false;
	}

	public int proxyGeometryFor(JOGLRenderingState jrs) {
		int dp = 0;
		if (front != null) {
			dp = front.proxyGeometryFor(jrs);
			LoggingSystem.getLogger(this)
					.log(Level.FINER, "Providing dl " + dp);
			return dp;
		}
		return -1;
	}

	public void flushCachedState(JOGLRenderer jr) {
		if (front != null)
			front.flushCachedState(jr);
		if (back != null)
			back.flushCachedState(jr);
	}

}
