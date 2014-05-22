/**
 *
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

import static de.jreality.shader.CommonAttributes.ATTENUATE_POINT_SIZE;
import static de.jreality.shader.CommonAttributes.ATTENUATE_POINT_SIZE_DEFAULT;
import static de.jreality.shader.CommonAttributes.DIFFUSE_COLOR;
import static de.jreality.shader.CommonAttributes.LIGHTING_ENABLED;
import static de.jreality.shader.CommonAttributes.LIGHT_DIRECTION;
import static de.jreality.shader.CommonAttributes.OPAQUE_TUBES_AND_SPHERES;
import static de.jreality.shader.CommonAttributes.OPAQUE_TUBES_AND_SPHERES_DEFAULT;
import static de.jreality.shader.CommonAttributes.POINT_DIFFUSE_COLOR_DEFAULT;
import static de.jreality.shader.CommonAttributes.POINT_RADIUS;
import static de.jreality.shader.CommonAttributes.POINT_RADIUS_DEFAULT;
import static de.jreality.shader.CommonAttributes.POINT_SIZE;
import static de.jreality.shader.CommonAttributes.POINT_SIZE_DEFAULT;
import static de.jreality.shader.CommonAttributes.POINT_SPRITE;
import static de.jreality.shader.CommonAttributes.POLYGON_SHADER;
import static de.jreality.shader.CommonAttributes.RADII_WORLD_COORDINATES;
import static de.jreality.shader.CommonAttributes.SPECULAR_COLOR;
import static de.jreality.shader.CommonAttributes.SPECULAR_COLOR_DEFAULT;
import static de.jreality.shader.CommonAttributes.SPECULAR_EXPONENT;
import static de.jreality.shader.CommonAttributes.SPECULAR_EXPONENT_DEFAULT;
import static de.jreality.shader.CommonAttributes.SPHERES_DRAW;
import static de.jreality.shader.CommonAttributes.SPHERES_DRAW_DEFAULT;
import static de.jreality.shader.CommonAttributes.TRANSPARENCY;
import static de.jreality.shader.CommonAttributes.TRANSPARENCY_DEFAULT;

import java.awt.Color;

import javax.media.opengl.GL;
import javax.media.opengl.GL2;

import de.jreality.geometry.GeometryUtility;
import de.jreality.jogl.JOGLConfiguration;
import de.jreality.jogl.JOGLRenderer;
import de.jreality.jogl.JOGLRendererHelper;
import de.jreality.jogl.JOGLRenderingState;
import de.jreality.jogl.JOGLSphereHelper;
import de.jreality.math.P3;
import de.jreality.math.Pn;
import de.jreality.math.Rn;
import de.jreality.scene.Appearance;
import de.jreality.scene.Geometry;
import de.jreality.scene.PointSet;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.AttributeEntityUtility;
import de.jreality.scene.data.DataList;
import de.jreality.scene.data.DoubleArray;
import de.jreality.scene.data.IntArray;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.EffectiveAppearance;
import de.jreality.shader.ShaderUtility;
import de.jreality.shader.Texture2D;
import de.jreality.util.CameraUtility;
import de.jreality.util.LoggingSystem;

public class DefaultPointShader extends AbstractPrimitiveShader implements
		PointShader {
	double pointSize = 1.0;
	// on my mac, the only value for the following array that seems to "work" is
	// {1,0,0}. WHY?
	float[] pointAttenuation = { 0.0f, 0f, 1.00000f }, noPointAttentuation = {
			1f, 0f, 0f };
	double pointRadius = .1;
	Color diffuseColor = java.awt.Color.RED;
	float[] diffuseColorAsFloat;
	float[] specularColorAsFloat = { 0f, 1f, 1f, 1f }; // for texturing point
														// sprite to simulate
														// sphere
	boolean sphereDraw = false, lighting = true, opaqueSpheres = true,
			radiiWorldCoords = false, attenuatePointSize = true,
			doSprites = true;
	PolygonShader polygonShader = null;
	Appearance a = new Appearance();
	Texture2D spriteTexture = (Texture2D) AttributeEntityUtility
			.createAttributeEntity(Texture2D.class, "", a, true);
	Texture2D currentTex;
	double specularExponent = 60.0;
	int polygonCount = 0;
	boolean changedTransp, changedLighting, spriteNeedsUpdated;
	double[] lightDirection = { 1, -1, 2 };
	private Color specularColor;
	static int textureSize = 128;
	de.jreality.shader.DefaultPointShader templateShader;

	public DefaultPointShader(de.jreality.shader.DefaultPointShader orig) {
		templateShader = orig;
	}

	public DefaultPointShader() {

	}

	public void setFromEffectiveAppearance(EffectiveAppearance eap, String name) {
		super.setFromEffectiveAppearance(eap, name);
		sphereDraw = eap.getAttribute(
				ShaderUtility.nameSpace(name, SPHERES_DRAW),
				SPHERES_DRAW_DEFAULT);
		opaqueSpheres = eap.getAttribute(
				ShaderUtility.nameSpace(name, OPAQUE_TUBES_AND_SPHERES),
				OPAQUE_TUBES_AND_SPHERES_DEFAULT);
		doSprites = eap.getAttribute(
				ShaderUtility.nameSpace(name, CommonAttributes.SPRITES_DRAW),
				true);
		lightDirection = (double[]) eap.getAttribute(
				ShaderUtility.nameSpace(name, LIGHT_DIRECTION), lightDirection);
		lighting = eap.getAttribute(
				ShaderUtility.nameSpace(name, LIGHTING_ENABLED), true);
		pointSize = eap.getAttribute(ShaderUtility.nameSpace(name, POINT_SIZE),
				POINT_SIZE_DEFAULT);
		attenuatePointSize = eap.getAttribute(
				ShaderUtility.nameSpace(name, ATTENUATE_POINT_SIZE),
				ATTENUATE_POINT_SIZE_DEFAULT);
		pointRadius = eap.getAttribute(
				ShaderUtility.nameSpace(name, POINT_RADIUS),
				POINT_RADIUS_DEFAULT);
		radiiWorldCoords = eap.getAttribute(
				ShaderUtility.nameSpace(name, RADII_WORLD_COORDINATES), false);
		diffuseColor = (Color) eap.getAttribute(
				ShaderUtility.nameSpace(name, DIFFUSE_COLOR),
				POINT_DIFFUSE_COLOR_DEFAULT);
		double t = eap.getAttribute(
				ShaderUtility.nameSpace(name, TRANSPARENCY),
				TRANSPARENCY_DEFAULT);
		diffuseColor = ShaderUtility.combineDiffuseColorWithTransparency(
				diffuseColor, t, JOGLRenderingState.useOldTransparency);
		diffuseColorAsFloat = diffuseColor.getRGBComponents(null);
		if (templateShader != null) {
			polygonShader = DefaultGeometryShader.createFrom(templateShader
					.getPolygonShader());
			polygonShader.setFromEffectiveAppearance(eap, name
					+ ".polygonShader");
		} else
			polygonShader = (PolygonShader) ShaderLookup.getShaderAttr(eap,
					name, "polygonShader");
		// System.err.println("Attenuate point size is "+attenuatePointSize);
		if (!sphereDraw) {
			if (AttributeEntityUtility.hasAttributeEntity(Texture2D.class,
					ShaderUtility.nameSpace(name, POINT_SPRITE + ".texture2d"),
					eap))
				currentTex = (Texture2D) AttributeEntityUtility
						.createAttributeEntity(
								Texture2D.class,
								ShaderUtility.nameSpace(name, POINT_SPRITE
										+ ".texture2d"), eap);
			else {
				Rn.normalize(lightDirection, lightDirection);
				specularColor = (Color) eap.getAttribute(
						ShaderUtility.nameSpace(name, POLYGON_SHADER + "."
								+ SPECULAR_COLOR), SPECULAR_COLOR_DEFAULT);
				specularColorAsFloat = specularColor.getRGBComponents(null);
				specularExponent = eap
						.getAttribute(
								ShaderUtility.nameSpace(name, POLYGON_SHADER
										+ "." + SPECULAR_EXPONENT),
								SPECULAR_EXPONENT_DEFAULT);
				spriteNeedsUpdated = true;
				currentTex = spriteTexture;
			}
		}
	}

	public Color getDiffuseColor() {
		return diffuseColor;
	}

	float[] oldDiffuseColorAsFloat = new float[4];

	private void setupTexture() {

		float sum = 0;
		for (int i = 0; i < 4; ++i) {
			float diff = diffuseColorAsFloat[i] - oldDiffuseColorAsFloat[i];
			oldDiffuseColorAsFloat[i] = diffuseColorAsFloat[i];
			sum += Math.abs(diff);
		}
		// if (sum < 10E-4) return;
		spriteTexture.setImage(ShadedSphereImage.shadedSphereImage(
				lightDirection, diffuseColor, specularColor, specularExponent,
				textureSize, lighting, null));
		spriteTexture.setApplyMode(Texture2D.GL_MODULATE);
		// use nearest filter to avoid corrupting the alpha = 0 transparency
		// trick
		spriteTexture.setMinFilter(Texture2D.GL_NEAREST);
	}

	private void preRender(JOGLRenderingState jrs) {
		JOGLRenderer jr = jrs.renderer;
		GL2 gl = jrs.renderer.globalGL;
		gl.glMaterialfv(GL.GL_FRONT, GL2.GL_DIFFUSE, diffuseColorAsFloat, 0);
		gl.glColor4fv(diffuseColorAsFloat, 0);
		System.arraycopy(diffuseColorAsFloat, 0,
				jr.renderingState.diffuseColor, 0, 4);

		if (!sphereDraw) {
			// LoggingSystem.getLogger(JOGLRendererHelper.class).fine("Rendering sprites");
			PointSet ps = (PointSet) jrs.currentGeometry;
			if (spriteNeedsUpdated) {
				lighting = jrs.lighting;
				if (ps.getVertexAttributes(Attribute.COLORS) != null)
					diffuseColor = Color.white;
				setupTexture();
				spriteNeedsUpdated = false;
			}
			lighting = false;
			gl.glPointSize((float) pointSize);
			jrs.pointSize = pointSize;
			// this doesn't work on my powerbook with ati radeon
			// (no exception, but the points don't show up no matter what the
			// arguments given
			try {
				gl.glPointParameterfv(GL2.GL_POINT_DISTANCE_ATTENUATION,
						attenuatePointSize ? pointAttenuation
								: noPointAttentuation, 0);
			} catch (Exception e) {
				// TODO: i dont know - got error on ati radeon 9800
			}
			gl.glEnable(GL2.GL_POINT_SMOOTH);
			if (doSprites) {
				gl.glEnable(GL2.GL_POINT_SPRITE);
				gl.glTexEnvi(GL2.GL_POINT_SPRITE, GL2.GL_COORD_REPLACE,
						GL.GL_TRUE);
				if (currentTex == spriteTexture
						&& ps.getVertexAttributes(Attribute.COLORS) != null)
					spriteTexture.setApplyMode(Texture2D.GL_MODULATE);
				else
					// this way we get real specular highlights
					spriteTexture.setApplyMode(Texture2D.GL_REPLACE);
				gl.glActiveTexture(GL.GL_TEXTURE0);
				gl.glEnable(GL.GL_TEXTURE_2D);
				Texture2DLoaderJOGL.render(gl, currentTex);
			}
		} else {
			// really need to call the preRender() method on the polygonShader,
			// but it doesn't exist.
			Geometry g = jrs.currentGeometry;
			// TODO fix this hack
			jrs.currentGeometry = null;
			polygonShader.render(jrs);
			jrs.currentGeometry = g;
		}

		// jr.renderingState.lighting = lighting;
		changedLighting = false;
		if (lighting != jrs.lighting) {
			if (lighting)
				gl.glEnable(GL2.GL_LIGHTING);
			else
				gl.glDisable(GL2.GL_LIGHTING);
			changedLighting = true;
		}
		// TODO build in support for OPAQUE_TUBES_AND_SPHERES
		changedTransp = false;
		// if (sphereDraw) {
		if (opaqueSpheres == jrs.transparencyEnabled) { // change of state!
			if (opaqueSpheres) {
				gl.glDepthMask(true);
				gl.glDisable(GL.GL_BLEND);
			} else {
				gl.glEnable(GL.GL_BLEND);
				gl.glDepthMask(jrs.zbufferEnabled);
				JOGLConfiguration.glBlendFunc(gl);
			}
			changedTransp = true;
		}
		// }

	}

	public void postRender(JOGLRenderingState jrs) {
		if (!jrs.shadeGeometry)
			return;
		JOGLRenderer jr = jrs.renderer;
		GL2 gl = jr.globalGL;
		if (!sphereDraw) {
			gl.glDisable(GL2.GL_POINT_SPRITE);
			gl.glActiveTexture(GL.GL_TEXTURE0);
			gl.glTexEnvi(GL2.GL_POINT_SPRITE, GL2.GL_COORD_REPLACE, GL.GL_FALSE);
			gl.glDisable(GL.GL_TEXTURE_2D);
		} else {
			polygonShader.postRender(jrs);
		}
		if (changedTransp) {
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
		return sphereDraw;
	}

	public int proxyGeometryFor(JOGLRenderingState jrs) {
		Geometry original = jrs.currentGeometry;
		JOGLRenderer jr = jrs.renderer;
		int sig = jrs.currentMetric;
		boolean useDisplayLists = jrs.useDisplayLists;
		GL2 gl = jr.globalGL;
		PointSet ps = (PointSet) original;
		DataList vertices = ps.getVertexAttributes(Attribute.COORDINATES);
		if (vertices == null)
			return -1; // throw new
						// IllegalStateException("No vertex coordinates for "+ps.getName());
		DataList piDL = ps.getVertexAttributes(Attribute.INDICES);
		IntArray vind = null;
		if (piDL != null)
			vind = piDL.toIntArray();
		DataList vertexColors = ps.getVertexAttributes(Attribute.COLORS);
		DataList radii = ps.getVertexAttributes(Attribute.RELATIVE_RADII);
		DoubleArray da = null, ra = null;
		if (radii != null)
			ra = radii.toDoubleArray();
		double radiiFactor = 1.0;
		if (radiiWorldCoords) {
			double[] o2w = jr.renderingState.currentPath.getMatrix(null);
			radiiFactor = CameraUtility.getScalingFactor(o2w,
					jr.renderingState.currentMetric);
			radiiFactor = 1.0 / radiiFactor;
		}

		// JOGLConfiguration.theLog.log(Level.INFO,"VC is "+vertexColors);
		int colorLength = 0;
		if (vertexColors != null)
			colorLength = GeometryUtility.getVectorLength(vertexColors);
		int n = ps.getNumPoints();
		int resolution = 1;
		// if (jr.renderingState.levelOfDetail == 0.0) resolution = 0;
		// I happen to know that 15 is the highest LOD for the sphere display
		// lists used here
		resolution = (int) (15 * jr.renderingState.levelOfDetail);
		if (resolution < 1)
			resolution = 1;
		int dlist = JOGLSphereHelper.getSphereDLists(resolution, jr);
		polygonCount = n * 24 * resolution * (resolution + 1) + 6;
		int nextDL = -1;
		if (useDisplayLists) {
			nextDL = gl.glGenLists(1);
			gl.glNewList(nextDL, GL2.GL_COMPILE);
		}
		double[] mat = Rn.identityMatrix(4);
		double[] scale = Rn.identityMatrix(4);
		scale[0] = scale[5] = scale[10] = radiiFactor * pointRadius;
		int length = n; // vind == null ? n : vind.getLength();
		for (int i = 0; i < length; ++i) {
			if (vind != null && vind.getValueAt(i) == 0)
				continue;
			int index = i;
			double[] transVec = vertices.item(index).toDoubleArray(null);
			if (!Pn.isValidCoordinate(transVec, 3, sig))
				continue;
			if (!P3.isValidTranslationVector(transVec, sig))
				continue;
			if (ra != null) {
				double radius = ra.getValueAt(i);
				scale[0] = scale[5] = scale[10] = pointRadius * radius;
			}
			gl.glPushMatrix();
			P3.makeTranslationMatrix(mat, transVec, sig);
			Rn.times(mat, mat, scale);
			gl.glMultTransposeMatrixd(mat, 0);
			if (vertexColors != null) {
				da = vertexColors.item(index).toDoubleArray();
				if (colorLength == 3) {
					gl.glColor3d(da.getValueAt(0), da.getValueAt(1),
							da.getValueAt(2));
				} else if (colorLength == 4) {
					gl.glColor4d(da.getValueAt(0), da.getValueAt(1),
							da.getValueAt(2), da.getValueAt(3));
				}
			}
			gl.glCallList(dlist);
			gl.glPopMatrix();
		}
		if (useDisplayLists)
			gl.glEndList();
		return nextDL;
	}

	public Shader getPolygonShader() {
		return polygonShader;
	}

	public void render(JOGLRenderingState jrs) {
		Geometry g = jrs.currentGeometry;
		JOGLRenderer jr = jrs.renderer;
		boolean useDisplayLists = jrs.useDisplayLists;
		if (!(g instanceof PointSet)) {
			throw new IllegalArgumentException("Must be PointSet");
		}
		if (jrs.shadeGeometry)
			preRender(jrs);
		if (g != null) {
			if (providesProxyGeometry()) {
				if (!useDisplayLists || dListProxy == -1) {
					dListProxy = proxyGeometryFor(jrs);
					displayListsDirty = false;
				}
				jr.globalGL.glCallList(dListProxy);
				jr.renderingState.polygonCount += polygonCount;
			} else {
				if (!useDisplayLists) {
					JOGLRendererHelper.drawVertices(jr, (PointSet) g,
							jr.renderingState.diffuseColor[3]);
				} else {
					if (useDisplayLists && dList == -1) {
						dList = jr.globalGL.glGenLists(1);
						jr.globalGL.glNewList(dList, GL2.GL_COMPILE); // _AND_EXECUTE);
						JOGLRendererHelper.drawVertices(jr, (PointSet) g,
								jr.renderingState.diffuseColor[3]);
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
				"PointShader: Flushing display lists " + dList + " : "
						+ dListProxy);
		if (dList != -1)
			jr.globalGL.glDeleteLists(dList, 1);
		// TODO !!!
		// if (dListProxy != -1) jr.getGL().glDeleteLists(dListProxy,1);
		dList = dListProxy = -1;
		displayListsDirty = true;
		// dList = -1;
	}

}
