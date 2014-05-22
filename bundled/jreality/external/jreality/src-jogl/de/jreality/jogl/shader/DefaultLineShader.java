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
/*
 * Created on Apr 29, 2004
 *
 */
package de.jreality.jogl.shader;

import static de.jreality.shader.CommonAttributes.USE_GLSL;

import java.awt.Color;

import javax.media.opengl.GL;
import javax.media.opengl.GL2;

import de.jreality.geometry.FrameFieldType;
import de.jreality.geometry.IndexedLineSetUtility;
import de.jreality.geometry.PolygonalTubeFactory;
import de.jreality.geometry.TubeUtility;
import de.jreality.jogl.JOGLConfiguration;
import de.jreality.jogl.JOGLRenderer;
import de.jreality.jogl.JOGLRendererHelper;
import de.jreality.jogl.JOGLRenderingState;
import de.jreality.math.Rn;
import de.jreality.scene.Appearance;
import de.jreality.scene.Geometry;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.IndexedLineSet;
import de.jreality.scene.Scene;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.DataList;
import de.jreality.scene.data.DoubleArray;
import de.jreality.scene.data.IntArray;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.EffectiveAppearance;
import de.jreality.shader.GlslProgram;
import de.jreality.shader.ShaderUtility;
import de.jreality.util.CameraUtility;
import de.jreality.util.LoggingSystem;

/**
 * @author Charles Gunn
 * 
 */
public class DefaultLineShader extends AbstractPrimitiveShader implements
		LineShader {
	protected de.jreality.shader.DefaultLineShader templateShader = null;
	FrameFieldType tubeStyle = FrameFieldType.PARALLEL;
	double tubeRadius = 0.05, lineWidth = 1.0;

	boolean lineStipple = false;
	int lineFactor = 1;
	int lineStipplePattern = 0x1c47;
	boolean tubeDraw = false, lineLighting = false, opaqueTubes = false,
			vertexColors = false, radiiWorldCoords = false;
	Color diffuseColor = java.awt.Color.BLACK;
	double[][] crossSection,
			defaultCrossSection = TubeUtility.octagonalCrossSection;

	private PolygonShader polygonShader;
	transient boolean changedTransp, changedLighting;
	transient float[] diffuseColorAsFloat;
	transient int faceCount = 0;

	StandardGLSLShader noneuc = new EuclideanGLSLShader();
	boolean useGLSL, hasNoneuc = false;
	GlslProgram glslProgram;

	public DefaultLineShader(de.jreality.shader.DefaultLineShader orig) {
		templateShader = orig;
	}

	public DefaultLineShader() {
	}

	public void setFromEffectiveAppearance(EffectiveAppearance eap, String name) {
		super.setFromEffectiveAppearance(eap, name);
		useGLSL = eap.getAttribute(ShaderUtility.nameSpace(name, USE_GLSL),
				false);
		tubeDraw = eap.getAttribute(
				ShaderUtility.nameSpace(name, CommonAttributes.TUBES_DRAW),
				CommonAttributes.TUBES_DRAW_DEFAULT);
		tubeRadius = eap.getAttribute(
				ShaderUtility.nameSpace(name, CommonAttributes.TUBE_RADIUS),
				CommonAttributes.TUBE_RADIUS_DEFAULT);
		opaqueTubes = eap.getAttribute(ShaderUtility.nameSpace(name,
				CommonAttributes.OPAQUE_TUBES_AND_SPHERES),
				CommonAttributes.OPAQUE_TUBES_AND_SPHERES_DEFAULT);
		tubeStyle = (FrameFieldType) eap.getAttribute(
				ShaderUtility.nameSpace(name, CommonAttributes.TUBE_STYLE),
				CommonAttributes.TUBE_STYLE_DEFAULT);
		// smoothLineShading =
		// eap.getAttribute(ShaderUtility.nameSpace(name,CommonAttributes.SMOOTH_LINE_SHADING),
		// CommonAttributes.SMOOTH_LINE_SHADING_DEFAULT);
		// smoothShading =
		// eap.getAttribute(ShaderUtility.nameSpace(name,CommonAttributes.SMOOTH_SHADING),
		// CommonAttributes.SMOOTH_SHADING_DEFAULT);
		lineLighting = eap.getAttribute(ShaderUtility.nameSpace(name,
				CommonAttributes.LINE_LIGHTING_ENABLED), false);
		vertexColors = eap.getAttribute(ShaderUtility.nameSpace(name,
				CommonAttributes.VERTEX_COLORS_ENABLED), false);
		radiiWorldCoords = eap.getAttribute(ShaderUtility.nameSpace(name,
				CommonAttributes.RADII_WORLD_COORDINATES), false);
		lineStipple = eap.getAttribute(
				ShaderUtility.nameSpace(name, CommonAttributes.LINE_STIPPLE),
				lineStipple);
		lineWidth = eap.getAttribute(
				ShaderUtility.nameSpace(name, CommonAttributes.LINE_WIDTH),
				CommonAttributes.LINE_WIDTH_DEFAULT);
		lineFactor = eap.getAttribute(
				ShaderUtility.nameSpace(name, CommonAttributes.LINE_FACTOR),
				lineFactor);
		lineStipplePattern = eap.getAttribute(ShaderUtility.nameSpace(name,
				CommonAttributes.LINE_STIPPLE_PATTERN), lineStipplePattern);
		diffuseColor = (Color) eap.getAttribute(
				ShaderUtility.nameSpace(name, CommonAttributes.DIFFUSE_COLOR),
				CommonAttributes.LINE_DIFFUSE_COLOR_DEFAULT);
		double transp = eap.getAttribute(
				ShaderUtility.nameSpace(name, CommonAttributes.TRANSPARENCY),
				CommonAttributes.TRANSPARENCY_DEFAULT);
		diffuseColor = ShaderUtility.combineDiffuseColorWithTransparency(
				diffuseColor, transp, JOGLRenderingState.useOldTransparency);
		diffuseColorAsFloat = diffuseColor.getRGBComponents(null);
		crossSection = (double[][]) eap.getAttribute(
				ShaderUtility.nameSpace(name, "crossSection"),
				defaultCrossSection);
		// System.err.println("xsec length = "+crossSection.length);
		if (templateShader != null) {
			polygonShader = DefaultGeometryShader.createFrom(templateShader
					.getPolygonShader());
			polygonShader.setFromEffectiveAppearance(eap, name
					+ ".polygonShader");
		} else
			// polygonShader = (PolygonShader) ShaderLookup.getShaderAttr(eap,
			// name, "polygonShader");
			throw new IllegalStateException("Not from a template!");
		if (useGLSL) {
			if (GlslProgram.hasGlslProgram(eap, name)) {
				// dummy to write glsl values like "lightingEnabled"
				Appearance app = new Appearance();
				glslProgram = new GlslProgram(app, eap, name);
				hasNoneuc = false;
			} else {
				noneuc.setFromEffectiveAppearance(eap, name);
				glslProgram = noneuc.getStandardShader();
				hasNoneuc = true;
			}
		} else
			hasNoneuc = false;
	}

	public void preRender(JOGLRenderingState jrs) {
		JOGLRenderer jr = jrs.renderer;
		GL2 gl = jrs.renderer.globalGL;
		gl.glMaterialfv(GL.GL_FRONT, GL2.GL_DIFFUSE, diffuseColorAsFloat, 0);
		gl.glColor4fv(diffuseColorAsFloat, 0);
		System.arraycopy(diffuseColorAsFloat, 0,
				jr.renderingState.diffuseColor, 0, 4);

		gl.glLineWidth((float) (lineWidth * jrs.globalAntiAliasingFactor));
		jrs.lineWidth = lineWidth * jrs.globalAntiAliasingFactor;
		if (lineStipple) {
			gl.glEnable(GL2.GL_LINE_STIPPLE);
			gl.glLineStipple(lineFactor, (short) lineStipplePattern);
		} else
			gl.glDisable(GL2.GL_LINE_STIPPLE);

		changedLighting = false;
		if (tubeDraw) {
			Geometry g = jrs.currentGeometry;
			jrs.currentGeometry = null; // hack!
			polygonShader.render(jrs);
			jrs.currentGeometry = g;
		} else {
			// System.err.println("line lighting = "+lineLighting);
			if (lineLighting != jrs.lighting) {
				if (lineLighting)
					gl.glEnable(GL2.GL_LIGHTING);
				else
					gl.glDisable(GL2.GL_LIGHTING);
				changedLighting = true;
			}
			if (lineLighting)
				polygonShader.render(jrs);
		}

		// this little bit of code forces tubes to be opaque: could add
		// transparency-enable flag to the line shader to allow this to be
		// controlled
		changedTransp = false;
		if (tubeDraw) {
			if (opaqueTubes) {
				gl.glDepthMask(true);
				gl.glDisable(GL.GL_BLEND);
				jrs.currentAlpha = 1.0;
			} else {
				gl.glEnable(GL.GL_BLEND);
				gl.glDepthMask(jrs.zbufferEnabled);
				JOGLConfiguration.glBlendFunc(gl);
			}
			changedTransp = true;
		}
		// }

		if (!tubeDraw)
			gl.glDepthRange(0.0d, jrs.depthFudgeFactor);
		if (useGLSL) {
			if (hasNoneuc) {
				noneuc.render(jr);
			}
			GlslLoader.render(glslProgram, jr);
		}
	}

	public void postRender(JOGLRenderingState jrs) {
		if (!jrs.shadeGeometry)
			return;
		JOGLRenderer jr = jrs.renderer;
		GL2 gl = jr.globalGL;
		if (useGLSL)
			GlslLoader.postRender(glslProgram, gl);
		if (!tubeDraw) {
			jr.globalGL.glDepthRange(0.0d, 1d);
		} else
			polygonShader.postRender(jrs);
		if (changedTransp) { // right now this is equivalent to "if (tubeDraw)"
			if (jrs.transparencyEnabled) {
				gl.glEnable(GL.GL_BLEND);
				gl.glDepthMask(jrs.zbufferEnabled);
				JOGLConfiguration.glBlendFunc(gl);
			} else {
				gl.glDepthMask(true);
				gl.glDisable(GL.GL_BLEND);
			}
		}
		if (changedLighting) {
			if (jrs.lighting)
				gl.glEnable(GL2.GL_LIGHTING);
			else
				gl.glDisable(GL2.GL_LIGHTING);
		}

	}

	public boolean providesProxyGeometry() {
		return tubeDraw;
	}

	public int proxyGeometryFor(JOGLRenderingState jrs) {
		final Geometry original = jrs.currentGeometry;
		final JOGLRenderer jr = jrs.renderer;
		final boolean useDisplayLists = jrs.useDisplayLists;
		double factor = 1.0;
		if (radiiWorldCoords) {
			double[] o2w = jr.renderingState.currentPath.getMatrix(null);
			factor = CameraUtility.getScalingFactor(o2w,
					jr.renderingState.currentMetric);
			factor = 1.0 / factor;
		}
		final double radiusFactor = factor;
		// if ( !(original instanceof IndexedLineSet)) return -1;
		if (tubeDraw && original instanceof IndexedLineSet) {
			final int[] dlist = new int[1];
			Scene.executeReader(original, new Runnable() {
				public void run() {
					dlist[0] = createTubesOnEdgesAsDL(
							(IndexedLineSet) original, radiusFactor
									* tubeRadius, jr, false, useDisplayLists);
				}
			});
			return dlist[0];
		}
		return -1;
	}

	int[] tubeDL = null;
	boolean testQMS = true;

	public int createTubesOnEdgesAsDL(IndexedLineSet ils, double rad,
			JOGLRenderer jr, boolean pickMode, boolean useDisplayLists) {
		GL2 gl = jr.globalGL;
		double[] p1 = new double[4], p2 = new double[4];
		p1[3] = p2[3] = 1.0;
		double[][] oneCurve = null;
		final int sig = jr.renderingState.currentMetric;

		// if (jr.renderingState.levelOfDetail == 0.0) crossSection =
		// TubeUtility.diamondCrossSection;
		DataList vertices = ils.getVertexAttributes(Attribute.COORDINATES);
		DataList radiidl = ils.getEdgeAttributes(Attribute.RELATIVE_RADII);
		DoubleArray radii = null;
		if (radiidl != null)
			radii = radiidl.toDoubleArray();
		if (ils.getNumPoints() <= 1)
			return -1;
		// JOGLConfiguration.theLog.log(Level.FINE,"Creating tubes for "+ils.getName());
		if (tubeDL == null) {
			tubeDL = new int[3];
		}
		if (tubeDL[sig + 1] == 0) {
			tubeDL[sig + 1] = gl.glGenLists(1);
			// LoggingSystem.getLogger(this).fine("LineShader: Allocating new dlist "+tubeDL[sig+1]+" for gl "+jr.globalGL);
			gl.glNewList(tubeDL[sig + 1], GL2.GL_COMPILE);
			JOGLRendererHelper.drawFaces(jr, TubeUtility.urTube[sig + 1]);
			gl.glEndList();
		}
		faceCount = 0;
		int tubeFaces = TubeUtility.urTube[sig + 1].getNumFaces();
		int nextDL = -1;
		if (useDisplayLists) {
			nextDL = gl.glGenLists(1);
			// LoggingSystem.getLogger(this).fine("LineShader: Allocating new dlist "+nextDL+" for gl "+jr.globalGL);
			gl.glNewList(nextDL, GL2.GL_COMPILE);
		}
		int k, l;
		DoubleArray da;
		double[] mat = new double[16];
		DataList edgec = ils.getEdgeAttributes(Attribute.COLORS);
		int n = ils.getNumEdges();
		for (int i = 0; i < n; ++i) {
			IntArray ia = ils.getEdgeAttributes(Attribute.INDICES).item(i)
					.toIntArray();
			int m = ia.size();
			double effectiveRadius = rad;
			if (radii != null) {
				effectiveRadius = rad * radii.getValueAt(i);
			}
			if (pickMode)
				gl.glPushName(i);
			DoubleArray edgecolor = null;
			int clength = 3;
			if (edgec != null) {
				edgecolor = edgec.item(i).toDoubleArray();
				clength = edgecolor.size();
				if (clength == 3)
					gl.glColor3d(edgecolor.getValueAt(0),
							edgecolor.getValueAt(1), edgecolor.getValueAt(2));
				else
					gl.glColor4d(edgecolor.getValueAt(0),
							edgecolor.getValueAt(1), edgecolor.getValueAt(2),
							edgecolor.getValueAt(3));
			}
			// System.err.println(m+" edges");
			if ((m == 2 && !vertexColors) || pickMode) { // probably an
															// IndexedFaceSet
				faceCount += (m - 1) * tubeFaces;

				for (int j = 0; j < m - 1; ++j) {
					k = ia.getValueAt(j);
					da = vertices.item(k).toDoubleArray();
					l = da.size();
					for (int xx = 0; xx < l; ++xx)
						p1[xx] = da.getValueAt(xx);
					k = ia.getValueAt(j + 1);
					da = vertices.item(k).toDoubleArray();
					l = da.size();
					for (int xx = 0; xx < l; ++xx)
						p2[xx] = da.getValueAt(xx);
					SceneGraphComponent cc = TubeUtility.tubeOneEdge(p1, p2,
							effectiveRadius, crossSection, sig);
					if (cc.getGeometry() != null) {
						if (pickMode)
							gl.glPushName(j);
						gl.glPushMatrix();
						gl.glMultTransposeMatrixd(cc.getTransformation()
								.getMatrix(mat), 0);
						gl.glCallList(tubeDL[sig + 1]);
						gl.glPopMatrix();
						if (pickMode)
							gl.glPopName();
					}

				}
			} else { // the assumption is that this is a genuine IndexedLineSet
						// (not subclass with faces)
				oneCurve = IndexedLineSetUtility.extractCurve(oneCurve, ils, i);
				double[][] clrs = null;
				if (vertexColors)
					clrs = IndexedLineSetUtility.extractCurveColors(clrs, ils,
							i);
				DataList dl = ils.getVertexAttributes(Attribute.RELATIVE_RADII);
				PolygonalTubeFactory ptf = new PolygonalTubeFactory(oneCurve);
				ptf.setClosed(false);
				if (clrs != null) {
					ptf.setVertexColors(clrs);
					ptf.setVertexColorsEnabled(true);
				}
				ptf.setCrossSection(crossSection);
				ptf.setFrameFieldType(tubeStyle);
				ptf.setMetric(sig);
				ptf.setRadius(effectiveRadius);
				if (dl != null) {
					double[] relrad = IndexedLineSetUtility.extractRadii(null,
							ils, i);
					double[] relrad2 = Rn.times(null, rad, relrad);
					ptf.setRadii(relrad2);
				}
				ptf.update();
				IndexedFaceSet tube = ptf.getTube();
				if (tube != null) {
					JOGLRendererHelper.drawFaces(jr, tube,
							jr.renderingState.smoothShading,
							jr.renderingState.diffuseColor[3]);
					faceCount += tube.getNumFaces();
				}
			}
			if (pickMode)
				gl.glPopName();
		}

		if (useDisplayLists)
			gl.glEndList();
		// problems with display list validity when switching to full-screen
		// mode: kill them
		return nextDL;
	}

	public void render(JOGLRenderingState jrs) {
		Geometry g = jrs.currentGeometry;
		JOGLRenderer jr = jrs.renderer;
		boolean useDisplayLists = jrs.useDisplayLists;
		if (!(g instanceof IndexedLineSet)) {
			throw new IllegalArgumentException("Must be IndexedLineSet");
		}
		if (jrs.shadeGeometry)
			preRender(jrs);
		if (g != null) {
			if (providesProxyGeometry()) {
				// System.err.println("count is: "+jr.getRenderingState().polygonCount);
				if (!useDisplayLists || dListProxy == -1) {
					dListProxy = proxyGeometryFor(jrs);
					displayListsDirty = false;
				}
				jr.globalGL.glCallList(dListProxy);
				jr.renderingState.polygonCount += faceCount;
			} else {
				if (!useDisplayLists) {
					// System.err.println("rendering lines w/o dlist");
					JOGLRendererHelper.drawLines(jr, (IndexedLineSet) g,
							vertexColors, jr.renderingState.diffuseColor[3]);
				} else {
					if (useDisplayLists && dList == -1) {
						dList = jr.globalGL.glGenLists(1);
						// LoggingSystem.getLogger(this).fine("LineShader: Allocating new dlist "+dList+" for gl "+jr.globalGL);
						jr.globalGL.glNewList(dList, GL2.GL_COMPILE); // _AND_EXECUTE);
						JOGLRendererHelper
								.drawLines(jr, (IndexedLineSet) g,
										vertexColors,
										jr.renderingState.diffuseColor[3]);
						// System.err.println("rendering lines w/ dlist");
						jr.globalGL.glEndList();
						displayListsDirty = false;
					}
					jr.globalGL.glCallList(dList);
				}
			}
		}
	}

	public void flushCachedState(JOGLRenderer jr) {
		LoggingSystem.getLogger(this).fine(
				"LineShader: Flushing display lists " + dList + " : "
						+ dListProxy);
		if (dList != -1)
			jr.globalGL.glDeleteLists(dList, 1);
		if (dListProxy != -1)
			jr.globalGL.glDeleteLists(dListProxy, 1);
		dList = dListProxy = -1;
		displayListsDirty = true;
		if (tubeDL != null) {
			// LoggingSystem.getLogger(this).fine("LineShader: Flushing display lists "+tubeDL[0]+" : "+tubeDL[1]+" : "+tubeDL[2]);
			for (int i = 0; i < 3; ++i)
				if (tubeDL[i] != 0) {
					jr.globalGL.glDeleteLists(tubeDL[i], 1);
					tubeDL[i] = 0;
				}
			tubeDL = null;
		}
	}

}
