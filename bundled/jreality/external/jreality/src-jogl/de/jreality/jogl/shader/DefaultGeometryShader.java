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

import de.jreality.jogl.JOGLPeerComponent;
import de.jreality.jogl.JOGLRenderingState;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.EffectiveAppearance;
import de.jreality.shader.ShaderUtility;
import de.jreality.util.LoggingSystem;

/**
 * @author Charles Gunn
 * 
 */
public class DefaultGeometryShader implements Shader {

	boolean faceDraw = true, vertexDraw = false, edgeDraw = true;
	// these should be more general shaders, but since we only have one type of
	// each ...
	public PolygonShader polygonShader;
	public LineShader lineShader;
	public PointShader pointShader;

	// public DefaultPolygonShader polygonShaderNew;
	/**
		 * 
		 */
	public DefaultGeometryShader() {
		super();
		// polygonShader = new DefaultPolygonShader();
		// lineShader = new DefaultLineShader();
		// pointShader = new DefaultPointShader();
	}

	public static DefaultGeometryShader createFromEffectiveAppearance(
			EffectiveAppearance eap, String name) {
		DefaultGeometryShader dgs = new DefaultGeometryShader();
		dgs.setFromEffectiveAppearance(eap, name);
		return dgs;
	}

	public void setFromEffectiveAppearance(EffectiveAppearance eap, String name) {
		String geomShaderName = ""; // at a later date this may be a field;
		vertexDraw = eap.getAttribute(ShaderUtility.nameSpace(geomShaderName,
				CommonAttributes.VERTEX_DRAW),
				CommonAttributes.VERTEX_DRAW_DEFAULT);
		edgeDraw = eap
				.getAttribute(ShaderUtility.nameSpace(geomShaderName,
						CommonAttributes.EDGE_DRAW),
						CommonAttributes.EDGE_DRAW_DEFAULT);
		faceDraw = eap
				.getAttribute(ShaderUtility.nameSpace(geomShaderName,
						CommonAttributes.FACE_DRAW),
						CommonAttributes.FACE_DRAW_DEFAULT);
		de.jreality.shader.DefaultGeometryShader dgs = ShaderUtility
				.createDefaultGeometryShader(eap);
		if (faceDraw) {
			if (polygonShader == null) {
				LoggingSystem.getLogger(this).finer("null polygonshader");
				polygonShader = createFrom(dgs.getPolygonShader());
			}
			polygonShader.setFromEffectiveAppearance(eap, ShaderUtility
					.nameSpace(name, CommonAttributes.POLYGON_SHADER));
		} else {
			polygonShader = null;
		}
		if (edgeDraw) {
			if (lineShader == null) {
				LoggingSystem.getLogger(this).finer("null lineshader");
				lineShader = createFrom(dgs.getLineShader());
			}

			lineShader
					.setFromEffectiveAppearance(eap, ShaderUtility.nameSpace(
							name, CommonAttributes.LINE_SHADER));
		} else {
			lineShader = null;
		}

		if (vertexDraw) {
			if (pointShader == null) {
				Class psClass = (Class) eap.getAttribute("pointShaderClass",
						Object.class);
				if (psClass != Object.class) {
					try {
						pointShader = (PointShader) psClass.newInstance();
					} catch (InstantiationException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					} catch (IllegalAccessException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
				}
				// fallback...
				if (pointShader == null)
					pointShader = createFrom(dgs.getPointShader());
			}
			pointShader.setFromEffectiveAppearance(eap, ShaderUtility
					.nameSpace(name, CommonAttributes.POINT_SHADER));
		} else {
			pointShader = null;
		}
	}

	public boolean isEdgeDraw() {
		return edgeDraw;
	}

	public boolean isFaceDraw() {
		return faceDraw;
	}

	public boolean isVertexDraw() {
		return vertexDraw;
	}

	public Shader getLineShader() {
		return lineShader;
	}

	public Shader getPointShader() {
		return pointShader;
	}

	public Shader getPolygonShader() {
		return polygonShader;
	}

	public void preRender(JOGLRenderingState jrs, JOGLPeerComponent jpc) {
	}

	public void postRender(JOGLRenderingState jrs) {
	}

	public void render(JOGLRenderingState jrs) {
	}

	public static PolygonShader createFrom(de.jreality.shader.PolygonShader ps) {
		PolygonShader ret = null;
		if (ps instanceof de.jreality.shader.TwoSidePolygonShader)
			ret = new TwoSidePolygonShader(
					(de.jreality.shader.TwoSidePolygonShader) ps);
		else if (ps instanceof de.jreality.shader.ImplodePolygonShader)
			ret = new ImplodePolygonShader(
					(de.jreality.shader.ImplodePolygonShader) ps);
		else if (ps instanceof de.jreality.shader.GlslPolygonShader)
			ret = new GlslPolygonShader();
		else if (ps instanceof de.jreality.shader.DefaultPolygonShader)
			ret = new DefaultPolygonShader(
					(de.jreality.shader.DefaultPolygonShader) ps);
		else
			ret = new DefaultPolygonShader();
		// System.err.println("ret = "+ret.getClass().getName());
		return ret;
	}

	public static LineShader createFrom(de.jreality.shader.LineShader ps) {
		LineShader ret = null;
		if (ps instanceof de.jreality.shader.DefaultLineShader)
			ret = new DefaultLineShader(
					(de.jreality.shader.DefaultLineShader) ps);
		else
			ret = new DefaultLineShader();
		// System.err.println("ret = "+ret.getClass().getName());
		return ret;
	}

	public static PointShader createFrom(de.jreality.shader.PointShader ps) {
		PointShader ret = null;
		if (ps instanceof de.jreality.shader.DefaultPointShader)
			ret = new DefaultPointShader(
					(de.jreality.shader.DefaultPointShader) ps);
		else
			ret = new DefaultPointShader();
		// System.err.println("ret = "+ret.getClass().getName());
		return ret;
	}
}
