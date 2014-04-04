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

package de.jreality.jogl;

import java.awt.Color;
import java.awt.Dimension;
import java.awt.Font;
import java.awt.geom.Rectangle2D;
import java.util.List;
import java.util.logging.Level;

import javax.media.opengl.GL;
import javax.media.opengl.GL2;

import de.jreality.backends.label.LabelUtility;
import de.jreality.geometry.GeometryUtility;
import de.jreality.geometry.HeightFieldFactory;
import de.jreality.geometry.Primitives;
import de.jreality.jogl.shader.DefaultPolygonShader;
import de.jreality.jogl.shader.Texture2DLoaderJOGL;
import de.jreality.math.Rn;
import de.jreality.scene.Appearance;
import de.jreality.scene.ClippingPlane;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.IndexedLineSet;
import de.jreality.scene.PointSet;
import de.jreality.scene.SceneGraphNode;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.AttributeEntityUtility;
import de.jreality.scene.data.DataList;
import de.jreality.scene.data.DoubleArray;
import de.jreality.scene.data.DoubleArrayArray;
import de.jreality.scene.data.IntArray;
import de.jreality.scene.data.IntArrayArray;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.DefaultTextShader;
import de.jreality.shader.ImageData;
import de.jreality.shader.Texture2D;
import de.jreality.shader.TextureUtility;

public class JOGLRendererHelper {

	public final static int PER_PART = 1;
	public final static int PER_FACE = 2;
	public final static int PER_VERTEX = 4;
	public final static int PER_EDGE = 8;
	static float val = 1f, zval = 1f;
	static float[][] unitsquare = { { val, val, zval }, { -val, val, zval },
			{ -val, -val, zval }, { val, -val, zval } };

	private JOGLRendererHelper() {
	}

	static Appearance pseudoAp = new Appearance();

	static void handleBackground(JOGLRenderer jr, int width, int height,
			Appearance topAp) {
		GL2 gl = jr.globalGL;
		JOGLRenderingState openGLState = jr.renderingState;
		Object bgo = null;
		float[] backgroundColor = new float[4];
		if (topAp == null)
			topAp = pseudoAp;
		// return;
		for (int i = 0; i < 6; ++i) {
			gl.glDisable(i + GL2.GL_CLIP_PLANE0);
		}
		if (topAp != null)
			bgo = topAp.getAttribute(CommonAttributes.BACKGROUND_COLOR);
		if (bgo != null && bgo instanceof java.awt.Color)
			((java.awt.Color) bgo).getRGBComponents(backgroundColor);
		else
			backgroundColor = CommonAttributes.BACKGROUND_COLOR_DEFAULT
					.getRGBComponents(null);
		gl.glClearColor(backgroundColor[0], backgroundColor[1],
				backgroundColor[2], backgroundColor[3]);
		// Here is where we clear the screen and set the color mask
		// It's a bit complicated by the various color masking required by
		// color-channel stereo (see JOGLRenderer#display() ).
		// System.err.println("clearbufferbits = "+jr.openGLState.clearBufferBits);
		// System.err.println("colormask = "+jr.openGLState.colorMask);
		// first set the color mask for the clear
		// LoggingSystem.getLogger(JOGLRendererHelper.class).finest("JOGLRRH cbb = "+
		// openGLState.clearBufferBits);
		// set color mask for the clear
		if ((openGLState.clearBufferBits & GL.GL_COLOR_BUFFER_BIT) != 0) {
			gl.glColorMask(true, true, true, true);
		}
		gl.glClear(openGLState.clearBufferBits);
		// now set the color mask for pixel writing
		int cm = openGLState.colorMask;
		gl.glColorMask((cm & 1) != 0, (cm & 2) != 0, (cm & 4) != 0,
				(cm & 8) != 0);

		Object obj = topAp.getAttribute(CommonAttributes.SKY_BOX);
		// only draw background colors or texture if the skybox isn't there
		if (obj == Appearance.INHERITED) {
			boolean hasTexture = false, hasColors = false;
			double textureAR = 1.0;
			obj = TextureUtility.getBackgroundTexture(topAp);
			Texture2D tex = null;
			if (obj != null) {
				tex = (Texture2D) obj;
				textureAR = tex.getImage().getWidth()
						/ ((double) tex.getImage().getHeight());
				hasTexture = true;
			}
//			 bgo = topAp.getAttribute(CommonAttributes.BACKGROUND_TEXTURE2D);
//			 if (bgo != null && bgo instanceof List) {
//			 tex = (Texture2D) ((List)bgo).get(0);
//			 }
			double ar = width / ((double) height) / textureAR;
			double xl = 0, xr = 1, yb = 0, yt = 1;
			if (ar > 1.0) {
				xl = 0.0;
				xr = 1.0;
				yb = .5 * (1 - 1 / ar);
				yt = 1.0 - yb;
			} else {
				yb = 0.0;
				yt = 1.0;
				xl = .5 * (1 - ar);
				xr = 1.0 - xl;
			}
//			if (jr.offscreenMode) {
//				int numTiles = jr.offscreenRenderer.getNumTiles();
//				double xmin = ((double) jr.whichTile[0]) / numTiles;
//				double xmax = ((double) jr.whichTile[0] + 1) / numTiles;
//				double ymin = ((double) jr.whichTile[1]) / numTiles;
//				double ymax = ((double) jr.whichTile[1] + 1) / numTiles;
//				double nxl, nxr, nyb, nyt;
//				nxr = xr + xmin * (xl - xr);
//				nxl = xr + xmax * (xl - xr);
//				nyt = yt + ymin * (yb - yt);
//				nyb = yt + ymax * (yb - yt);
//				xl = nxl;
//				xr = nxr;
//				yb = nyb;
//				yt = nyt;
//			}
			double[][] texcoords = { { xl, yb }, { xr, yb }, { xr, yt },
					{ xl, yt } };
			float[][] cornersf = new float[4][];
			if (!hasTexture) {
				bgo = topAp.getAttribute(CommonAttributes.BACKGROUND_COLORS);
				if (bgo != null && bgo instanceof Color[]) {
					Color[] backgroundCorners = (Color[]) bgo;
					for (int i = 0; i < 4; ++i) {
						cornersf[i] = backgroundCorners[i]
								.getRGBComponents(null);
					}
				} else {
					for (int i = 0; i < 4; ++i)
						cornersf[i] = backgroundColor;
				}
				hasColors = true;
			}
			if (hasTexture || hasColors) {
				// bgo = (Object) corners;
				if (hasTexture) {
					gl.glPushAttrib(GL2.GL_TEXTURE_BIT);
					gl.glActiveTexture(GL.GL_TEXTURE0);
					gl.glEnable(GL.GL_TEXTURE_2D);
					Texture2DLoaderJOGL.render(gl, tex);
				}
				// gl.glPushAttrib(GL.GL_ENABLE_BIT);
				// gl.glDisable(GL.GL_DEPTH_TEST);
				gl.glDisable(GL2.GL_LIGHTING);
				gl.glShadeModel(GL2.GL_SMOOTH);
				gl.glBegin(GL2.GL_POLYGON);
				// gl.glScalef(.5f, .5f, 1.0f);
				for (int q = 0; q < 4; ++q) {
					if (hasTexture) {
						gl.glColor3f(1f, 1f, 1f);
						gl.glTexCoord2dv(texcoords[q], 0);
					} else {
						gl.glColor4fv(cornersf[q], 0);
					}
					gl.glVertex3fv(unitsquare[q], 0);
				}
				gl.glEnd();
				// TODO push/pop this correctly (now may overwrite previous
				// values)
				// gl.glPopAttrib();
				gl.glEnable(GL.GL_DEPTH_TEST);
				gl.glEnable(GL2.GL_LIGHTING);
				if (hasTexture) {
					gl.glDisable(GL.GL_TEXTURE_2D);
					gl.glMatrixMode(GL2.GL_PROJECTION);
					gl.glPopAttrib();
				}
			}
		}
		bgo = topAp.getAttribute(CommonAttributes.FOG_ENABLED);
		boolean doFog = CommonAttributes.FOG_ENABLED_DEFAULT;
		if (bgo instanceof Boolean)
			doFog = ((Boolean) bgo).booleanValue();
		jr.renderingState.fogEnabled = doFog;
		if (doFog) {
			gl.glEnable(GL2.GL_FOG);
			bgo = topAp.getAttribute(CommonAttributes.FOG_COLOR);
			float[] fogColor = backgroundColor;
			if (bgo != null && bgo instanceof Color) {
				fogColor = ((Color) bgo).getRGBComponents(null);
			}
			gl.glFogi(GL2.GL_FOG_MODE, GL2.GL_EXP);
			gl.glFogfv(GL2.GL_FOG_COLOR, fogColor, 0);
			bgo = topAp.getAttribute(CommonAttributes.FOG_DENSITY);
			float density = (float) CommonAttributes.FOG_DENSITY_DEFAULT;
			if (bgo != null && bgo instanceof Double) {
				density = (float) ((Double) bgo).doubleValue();
			}
			gl.glFogf(GL2.GL_FOG_DENSITY, density);
		} else {
			gl.glDisable(GL2.GL_FOG);
			gl.glFogf(GL2.GL_FOG_DENSITY, 0f);
		}
	}

	// private static ByteBuffer vBuffer, vcBuffer, vnBuffer, fcBuffer,
	// fnBuffer, tcBuffer;

	// private static DataList vLast = null, vcLast = null, vnLast = null;

	@Deprecated
	public static void drawVertices(JOGLRenderer jr, PointSet sg, double alpha) {
		jr.renderingState.currentAlpha = alpha;
		drawVertices(jr, sg);
	}

	public static void drawVertices(JOGLRenderer jr, PointSet sg) {
		double alpha = jr.renderingState.currentAlpha;
		GL2 gl = jr.globalGL;
		JOGLRenderingState openGLState = jr.renderingState;
		// if (sg.getNumPoints() == 0)
		// return;
		// gl.glPointSize((float)
		// currentGeometryShader.pointShader.getPointSize());
		DataList vertices = sg.getVertexAttributes(Attribute.COORDINATES);
		if (vertices == null || vertices.size() == 0)
			return;
		DataList piDL = sg.getVertexAttributes(Attribute.INDICES);
		IntArray vind = null;
		if (piDL != null)
			vind = piDL.toIntArray();
		DataList vertexColors = sg.getVertexAttributes(Attribute.COLORS);
		DataList pointSize = sg.getVertexAttributes(Attribute.RELATIVE_RADII);
		int vertexLength = GeometryUtility.getVectorLength(vertices);
		int colorLength = 0;
		if (vertexColors != null) {
			colorLength = GeometryUtility.getVectorLength(vertexColors);
			if (openGLState.frontBack != DefaultPolygonShader.FRONT_AND_BACK) {
				gl.glColorMaterial(DefaultPolygonShader.FRONT_AND_BACK,
						GL2.GL_DIFFUSE);
				openGLState.frontBack = DefaultPolygonShader.FRONT_AND_BACK;
			}
		}

		DoubleArray da, ra = null;
		if (pointSize != null)
			ra = pointSize.toDoubleArray();
		// vertices in picking mode");
		if (pointSize == null)
			gl.glBegin(GL.GL_POINTS);
		for (int i = 0; i < sg.getNumPoints(); ++i) {
			// double vv;
			if (vind != null && vind.getValueAt(i) == 0)
				continue;
			if (pointSize != null) {
				float ps = (float) (jr.renderingState.pointSize * ra
						.getValueAt(i));
				gl.glPointSize(ps);
			}
			if (pointSize != null)
				gl.glBegin(GL.GL_POINTS);
			// if (pointSize != null) gl.glBegin(GL.GL_POINTS);
			if (vertexColors != null) {
				da = vertexColors.item(i).toDoubleArray();
				if (colorLength == 3) {
					gl.glColor4d(da.getValueAt(0), da.getValueAt(1),
							da.getValueAt(2), alpha);
				} else if (colorLength == 4) {
					gl.glColor4d(da.getValueAt(0), da.getValueAt(1),
							da.getValueAt(2), alpha * da.getValueAt(3));
				}
			}
			da = vertices.item(i).toDoubleArray();
			if (vertexLength == 3)
				gl.glVertex3d(da.getValueAt(0), da.getValueAt(1),
						da.getValueAt(2));
			else if (vertexLength == 4)
				gl.glVertex4d(da.getValueAt(0), da.getValueAt(1),
						da.getValueAt(2), da.getValueAt(3));
			if (pointSize != null)
				gl.glEnd();
		}
		gl.glEnd();
	}

	@Deprecated
	public static void drawLines(JOGLRenderer jr, IndexedLineSet sg,
			boolean interpolateVertexColors, double alpha) {
		jr.renderingState.currentAlpha = alpha;
		jr.renderingState.useVertexColors = interpolateVertexColors;
		drawLines(jr, sg);
	}

	public static void drawLines(JOGLRenderer jr, IndexedLineSet sg) {
		double alpha = jr.renderingState.currentAlpha;
		boolean interpolateVertexColors = jr.renderingState.useVertexColors;

		if (sg.getNumEdges() == 0)
			return;

		GL2 gl = jr.globalGL;

		DataList vertices = sg.getVertexAttributes(Attribute.COORDINATES);
		int vertexLength = GeometryUtility.getVectorLength(vertices);
		DataList edgeColors = sg.getEdgeAttributes(Attribute.COLORS);
		DataList vertexColors = sg.getVertexAttributes(Attribute.COLORS);
		DataList vertexNormals = sg.getVertexAttributes(Attribute.NORMALS);
		DataList lineWidth = sg.getVertexAttributes(Attribute.RELATIVE_RADII);
		DoubleArray ra = null;
		if (lineWidth != null)
			ra = lineWidth.toDoubleArray();
		boolean hasNormals = vertexNormals != null;
		DoubleArray da;
		if (sg.getEdgeAttributes(Attribute.INDICES) == null)
			return;
		int colorBind = 0, colorLength = 0;
		if (interpolateVertexColors && vertexColors != null) {
			colorBind = PER_VERTEX;
			colorLength = GeometryUtility.getVectorLength(vertexColors);
		} else if (edgeColors != null) {
			colorBind = PER_EDGE;
			colorLength = GeometryUtility.getVectorLength(edgeColors);
		} else
			colorBind = PER_PART;
		if (colorBind != PER_PART) {
			if (jr.renderingState.frontBack != DefaultPolygonShader.FRONT_AND_BACK) {
				gl.glColorMaterial(DefaultPolygonShader.FRONT_AND_BACK,
						GL2.GL_DIFFUSE);
				jr.renderingState.frontBack = DefaultPolygonShader.FRONT_AND_BACK;
			}
		}
		int numEdges = sg.getNumEdges();
		// if (pickMode) JOGLConfiguration.theLog.log(Level.INFO,"Rendering
		// edges in picking mode");
		// System.err.println("rendering line set with edge count = "+numEdges);
		for (int i = 0; i < numEdges; ++i) {
			gl.glBegin(GL.GL_LINE_STRIP);
			int[] ed = sg.getEdgeAttributes(Attribute.INDICES).item(i)
					.toIntArray(null);
			int m = ed.length;
			if (colorBind == PER_EDGE) {
				da = edgeColors.item(i).toDoubleArray();
				if (colorLength == 3) {
					gl.glColor4d(da.getValueAt(0), da.getValueAt(1),
							da.getValueAt(2), alpha);
				} else if (colorLength == 4) {
					gl.glColor4d(da.getValueAt(0), da.getValueAt(1),
							da.getValueAt(2), alpha * da.getValueAt(3));
				}
			}

			for (int j = 0; j < m; ++j) {
				int k = ed[j];
				if (colorBind == PER_VERTEX) {
					da = vertexColors.item(k).toDoubleArray();
					if (colorLength == 3) {
						gl.glColor4d(da.getValueAt(0), da.getValueAt(1),
								da.getValueAt(2), alpha);
					} else if (colorLength == 4) {
						gl.glColor4d(da.getValueAt(0), da.getValueAt(1),
								da.getValueAt(2), alpha * da.getValueAt(3));
					}
				}
				if (hasNormals) {
					da = vertexNormals.item(k).toDoubleArray();
					gl.glNormal3d(da.getValueAt(0), da.getValueAt(1),
							da.getValueAt(2));
				}
				da = vertices.item(k).toDoubleArray();
				if (vertexLength == 3)
					gl.glVertex3d(da.getValueAt(0), da.getValueAt(1),
							da.getValueAt(2));
				else if (vertexLength == 4)
					gl.glVertex4d(da.getValueAt(0), da.getValueAt(1),
							da.getValueAt(2), da.getValueAt(3));
			}
			gl.glEnd();
		}
	}

	@Deprecated
	public static void drawFaces(JOGLRenderer jr, IndexedFaceSet sg,
			boolean smooth, double alpha) {
		jr.renderingState.smoothShading = smooth;
		jr.renderingState.currentAlpha = alpha;
		drawFaces(jr, sg);
	}

	public static void drawFaces(JOGLRenderer jr, IndexedFaceSet sg) {
		boolean smooth = jr.renderingState.smoothShading;
		double alpha = jr.renderingState.currentAlpha;
		if (sg.getNumFaces() == 0)
			return;
		GL2 gl = jr.globalGL;
		int colorBind = -1, normalBind, colorLength = 3;
		DataList vertices = sg.getVertexAttributes(Attribute.COORDINATES);
		DataList vertexNormals = sg.getVertexAttributes(Attribute.NORMALS);
		// vertexNormals = GlslPolygonShader.correctNormals(vertexNormals);
		DataList faceNormals = sg.getFaceAttributes(Attribute.NORMALS);
		// faceNormals = GlslPolygonShader.correctNormals(faceNormals);
		DataList vertexColors = sg.getVertexAttributes(Attribute.COLORS);
		DataList faceColors = sg.getFaceAttributes(Attribute.COLORS);
		DataList texCoords = sg
			.getVertexAttributes(Attribute.TEXTURE_COORDINATES);
		DataList texCoords1 = sg
		.getVertexAttributes(Attribute.TEXTURE_COORDINATES1);
		DataList texCoords2 = sg
		.getVertexAttributes(Attribute.TEXTURE_COORDINATES2);
		DataList lightMapCoords = sg.getVertexAttributes(Attribute
				.attributeForName("lightmap coordinates"));
		int textureCount = jr.renderingState.texUnitCount;
		 if (texCoords2 != null)
		 System.err.println("got texture coordinates 2, # tex units = "+textureCount+" name "+sg.getName());

		// JOGLConfiguration.theLog.log(Level.INFO,"Vertex normals are:
		// "+((vertexNormals != null) ? vertexNormals.size() : 0));
		// JOGLConfiguration.theLog.log(Level.INFO,"alpha value is "+alpha);

		// vertex color has priority over face color
		vertices = sg.getVertexAttributes(Attribute.COORDINATES);
		int vertexLength = GeometryUtility.getVectorLength(vertices);
		if (vertexColors != null && (faceColors == null || smooth)) {
			colorBind = PER_VERTEX;
			colorLength = GeometryUtility.getVectorLength(vertexColors);
		} else if (faceColors != null && colorBind != PER_VERTEX) {
			colorBind = PER_FACE;
			colorLength = GeometryUtility.getVectorLength(faceColors);
		} else
			colorBind = PER_PART;
		// JOGLConfiguration.theLog.log(Level.INFO,"Color binding is
		// "+colorBind);
		if (colorBind != PER_PART) {
			if (jr.renderingState.frontBack != DefaultPolygonShader.FRONT_AND_BACK) {
				gl.glEnable(GL2.GL_COLOR_MATERIAL);
				gl.glColorMaterial(DefaultPolygonShader.FRONT_AND_BACK,
						GL2.GL_DIFFUSE);
				jr.renderingState.frontBack = DefaultPolygonShader.FRONT_AND_BACK;
			}
		}
		int nFiber = 3;
		if (vertexNormals != null && smooth) {
			normalBind = PER_VERTEX;
			nFiber = GeometryUtility.getVectorLength(vertexNormals);
		} else if (faceNormals != null) {
			normalBind = PER_FACE;
			nFiber = GeometryUtility.getVectorLength(faceNormals);
		} else
			normalBind = PER_PART;
		// System.err.println("Geom = "+sg.getName()+" normal length = "+nFiber);
		jr.renderingState.normals4d = (nFiber == 4);
		// HACK!!! make sure the vertex shader knows whether the normals are 4d
		// or 3d
		gl.glFogf(GL2.GL_FOG_START, nFiber == 4 ? 0.01f : 0f);
		// if (nFiber == 4)
		// System.err.println("Rendering 4d normals for "+sg.getName());
		DoubleArray da = null;
		boolean isQuadMesh = false;
		boolean isRegularDomainQuadMesh = false;
		Rectangle2D theDomain = null;
		int maxU = 0, maxV = 0, maxFU = 0, maxFV = 0, numV = 0, numF;
		Object qmatt = sg
				.getGeometryAttributes(GeometryUtility.QUAD_MESH_SHAPE);
		if (qmatt != null && qmatt instanceof Dimension) {
			Dimension dm = (Dimension) qmatt;
			isQuadMesh = true;
			maxU = dm.width;
			maxV = dm.height;
			numV = maxU * maxV;
			maxFU = maxU - 1;
			maxFV = maxV - 1;
			// Done with GeometryAttributes?
			// qmatt =
			// sg.getGeometryAttributes(GeometryUtility.HEIGHT_FIELD_SHAPE);
			// if (qmatt != null && qmatt instanceof Rectangle2D) {
			// theDomain = (Rectangle2D) qmatt;
			// isRegularDomainQuadMesh = true;
			// }
		}

		// if (textureUnitsDL!= null)
		// for (int i = GL.GL_TEXTURE0; i < jr.renderingState.texUnitCount; ++i)
		// {
		// gl.glActiveTexture(i);
		// gl.glDisable(GL.GL_TEXTURE_2D);
		// }
		//
		numF = sg.getNumFaces();
		if (isQuadMesh) {
			double[] pt = new double[3];
			// this loops through the "rows" of the mesh (v is constant on each
			// row)
			for (int i = 0; i < maxFV; ++i) {
				gl.glBegin(GL2.GL_QUAD_STRIP);
				// each iteration of this loop draws one quad strip consisting
				// of 2 * maxU vertices
				for (int j = 0; j <= maxFU; ++j) {
					int u = j % maxU;
					// draw two points: one on "this" row, the other directly
					// below on the next "row"
					for (int incr = 0; incr < 2; ++incr) {
						int vnn = (i * maxU + j % maxU + incr * maxU) % numV;
						int fnn = (i * maxFU + j % maxFU + incr * maxFU) % numF;
						int v = (i + incr) % maxV;
						if (normalBind == PER_FACE) {
							if (incr == 0 && j != maxFU) { // ) { //
								da = faceNormals.item(fnn).toDoubleArray();
								if (nFiber == 3)
									gl.glNormal3d(da.getValueAt(0),
											da.getValueAt(1), da.getValueAt(2));
								else
									gl.glMultiTexCoord4d(GL.GL_TEXTURE0 + 3,
											da.getValueAt(0), da.getValueAt(1),
											da.getValueAt(2), da.getValueAt(3));
							}
						} else if (normalBind == PER_VERTEX) {
							da = vertexNormals.item(vnn).toDoubleArray();
							if (nFiber == 3)
								gl.glNormal3d(da.getValueAt(0),
										da.getValueAt(1), da.getValueAt(2));
							else
								gl.glMultiTexCoord4d(GL.GL_TEXTURE0 + 3,
										da.getValueAt(0), da.getValueAt(1),
										da.getValueAt(2), da.getValueAt(3));
						}
						if (colorBind == PER_FACE) {
							if (incr == 0) {
								da = faceColors.item(fnn).toDoubleArray();
								if (colorLength == 3) {
									gl.glColor4d(da.getValueAt(0),
											da.getValueAt(1), da.getValueAt(2),
											alpha);
								} else if (colorLength == 4) {
									gl.glColor4d(da.getValueAt(0),
											da.getValueAt(1), da.getValueAt(2),
											alpha * da.getValueAt(3));
								}
							}
						} else if (colorBind == PER_VERTEX) {
							da = vertexColors.item(vnn).toDoubleArray();
							if (colorLength == 3) {
								gl.glColor4d(da.getValueAt(0),
										da.getValueAt(1), da.getValueAt(2),
										alpha);
							} else if (colorLength == 4) {
								gl.glColor4d(da.getValueAt(0),
										da.getValueAt(1), da.getValueAt(2),
										alpha * da.getValueAt(3));
							}
						}
						for (int nn = 0; nn < textureCount; ++nn) {
							int texunit = GL.GL_TEXTURE0 + nn;
							if (nn == textureCount - 1
									&& lightMapCoords != null) {
								da = lightMapCoords.item(vnn).toDoubleArray();
							} else if (texCoords1 != null && nn == 1) {
								da = texCoords1.item(vnn).toDoubleArray();
							} else if (texCoords2 != null && nn == 2) {
								da = texCoords2.item(vnn).toDoubleArray();
							} else if (texCoords != null) {
								da = texCoords.item(vnn).toDoubleArray();
							}
							if (da.size() == 2) {
								gl.glMultiTexCoord2d(texunit, da.getValueAt(0),
										da.getValueAt(1));
							} else if (da.size() == 3) {
								gl.glMultiTexCoord3d(texunit, da.getValueAt(0),
										da.getValueAt(1), da.getValueAt(2));
							} else if (da.size() > 3) {
								gl.glMultiTexCoord4d(texunit, da.getValueAt(0),
										da.getValueAt(1), da.getValueAt(2),
										da.getValueAt(3));
							}
						}
						da = vertices.item(vnn).toDoubleArray();
						if (vertexLength == 1 && isRegularDomainQuadMesh) {

							double z = da.getValueAt(0);
							HeightFieldFactory.getCoordinatesForUV(pt,
									theDomain, u, v, maxU, maxV);
							gl.glVertex3d(pt[0], pt[1], z);
						} else if (vertexLength == 3)
							gl.glVertex3d(da.getValueAt(0), da.getValueAt(1),
									da.getValueAt(2));
						else if (vertexLength == 4)
							gl.glVertex4d(da.getValueAt(0), da.getValueAt(1),
									da.getValueAt(2), da.getValueAt(3));
					}
				}
				gl.glEnd();
			}
		} else {
			for (int i = 0; i < sg.getNumFaces(); ++i) {
				if (colorBind == PER_FACE) {
					da = faceColors.item(i).toDoubleArray();
					if (colorLength == 3) {
						gl.glColor4d(da.getValueAt(0), da.getValueAt(1),
								da.getValueAt(2), alpha);
					} else if (colorLength == 4) {
						gl.glColor4d(da.getValueAt(0), da.getValueAt(1),
								da.getValueAt(2), alpha * da.getValueAt(3));
					}
				}
				if (normalBind == PER_FACE) {
					da = faceNormals.item(i).toDoubleArray();
					if (nFiber == 3)
						gl.glNormal3d(da.getValueAt(0), da.getValueAt(1),
								da.getValueAt(2));
					else
						gl.glMultiTexCoord4d(GL.GL_TEXTURE0 + 3,
								da.getValueAt(0), da.getValueAt(1),
								da.getValueAt(2), da.getValueAt(3));
					// gl.glNormal3d(da.getValueAt(0), da.getValueAt(1), da
					// .getValueAt(2));
				}
				IntArray tf = sg.getFaceAttributes(Attribute.INDICES).item(i)
						.toIntArray();
				final int nf = tf.getLength();
				// hack to allow texture per face!
				gl.glBegin(GL2.GL_POLYGON);
				for (int j = 0; j < nf; ++j) {
					int k = tf.getValueAt(j);
					if (normalBind == PER_VERTEX) {
						da = vertexNormals.item(k).toDoubleArray();
						if (nFiber == 3)
							gl.glNormal3d(da.getValueAt(0), da.getValueAt(1),
									da.getValueAt(2));
						else
							gl.glMultiTexCoord4d(GL.GL_TEXTURE0 + 3,
									da.getValueAt(0), da.getValueAt(1),
									da.getValueAt(2), da.getValueAt(3));
						// gl.glNormal3d(da.getValueAt(0), da.getValueAt(1), da
						// .getValueAt(2));
					}
					if (colorBind == PER_VERTEX) {
						da = vertexColors.item(k).toDoubleArray();
						if (colorLength == 3) {
							gl.glColor4d(da.getValueAt(0), da.getValueAt(1),
									da.getValueAt(2), alpha);
						} else if (colorLength == 4) {
							gl.glColor4d(da.getValueAt(0), da.getValueAt(1),
									da.getValueAt(2), alpha * da.getValueAt(3));
						}
					}
					for (int nn = 0; nn < textureCount; ++nn) {
						// if (nn != textureUnits[i]) continue;
						int texunit = GL.GL_TEXTURE0 + nn;
						if (nn == textureCount - 1 && lightMapCoords != null) {
							// if (nn == 0 && lightMapCoords != null) {
							da = lightMapCoords.item(k).toDoubleArray();
						} else if (texCoords1 != null && nn == 1) {
							da = texCoords1.item(k).toDoubleArray();
						} else if (texCoords2 != null && nn == 2) {
							da = texCoords2.item(k).toDoubleArray();
						} else if (texCoords != null) {
							da = texCoords.item(k).toDoubleArray();
						}

						if (da.size() == 2) {
							gl.glMultiTexCoord2d(texunit, da.getValueAt(0),
									da.getValueAt(1));
						} else if (da.size() == 3) {
							gl.glMultiTexCoord3d(texunit, da.getValueAt(0),
									da.getValueAt(1), da.getValueAt(2));
						} else if (da.size() > 3) {
							gl.glMultiTexCoord4d(texunit, da.getValueAt(0),
									da.getValueAt(1), da.getValueAt(2),
									da.getValueAt(3));
						}
					}
					da = vertices.item(k).toDoubleArray();
					if (vertexLength == 3)
						gl.glVertex3d(da.getValueAt(0), da.getValueAt(1),
								da.getValueAt(2));
					else if (vertexLength == 4)
						gl.glVertex4d(da.getValueAt(0), da.getValueAt(1),
								da.getValueAt(2), da.getValueAt(3));
				}
				gl.glEnd();
			}
		}
	}

	private static IndexedFaceSet bb = Primitives
			.texturedQuadrilateral(new double[] { 0, 1, 0, 1, 1, 0, 1, 0, 0, 0,
					0, 0 });

	private static  Texture2D tex2d = null;

	public static void drawPointLabels(JOGLRenderer jr, PointSet ps,
			DefaultTextShader ts) {
		if (!ts.getShowLabels().booleanValue())
			return;

		Font font = ts.getFont();
		Color c = ts.getDiffuseColor();
		double scale = ts.getScale().doubleValue();
		double[] offset = ts.getOffset();
		int alignment = ts.getAlignment();
		ImageData[] img = LabelUtility.createPointImages(ps, font, c);

		renderLabels(jr, img, ps.getVertexAttributes(Attribute.COORDINATES)
				.toDoubleArrayArray(), null, offset, alignment, scale);

	}

	public static void drawEdgeLabels(JOGLRenderer jr, IndexedLineSet ils,
			DefaultTextShader ts) {
		if (!ts.getShowLabels().booleanValue())
			return;

		Font font = ts.getFont();
		Color c = ts.getDiffuseColor();
		double scale = ts.getScale().doubleValue();
		double[] offset = ts.getOffset();
		int alignment = ts.getAlignment();
		ImageData[] img = LabelUtility.createEdgeImages(ils, font, c);

		renderLabels(jr, img, ils.getVertexAttributes(Attribute.COORDINATES)
				.toDoubleArrayArray(), ils.getEdgeAttributes(Attribute.INDICES)
				.toIntArrayArray(), offset, alignment, scale);

	}

	public static void drawFaceLabels(JOGLRenderer jr, IndexedFaceSet ifs,
			DefaultTextShader ts) {
		if (!ts.getShowLabels().booleanValue())
			return;
		Font font = ts.getFont();
		Color c = ts.getDiffuseColor();
		double scale = ts.getScale().doubleValue();
		double[] offset = ts.getOffset();
		int alignment = ts.getAlignment();
		ImageData[] img = LabelUtility.createFaceImages(ifs, font, c);

		renderLabels(jr, img, ifs.getVertexAttributes(Attribute.COORDINATES)
				.toDoubleArrayArray(), ifs.getFaceAttributes(Attribute.INDICES)
				.toIntArrayArray(), offset, alignment, scale);

	}

	private static void renderLabels(JOGLRenderer jr, ImageData[] labels,
			DoubleArrayArray vertices, IntArrayArray indices, double[] offset,
			int alignment, double scale) {
		if (labels == null)
			return;
		if (tex2d == null) {
			tex2d = (Texture2D) AttributeEntityUtility.createAttributeEntity(Texture2D.class, "", new Appearance(), true);
			tex2d.setRepeatS(Texture2D.GL_CLAMP);
			tex2d.setRepeatT(Texture2D.GL_CLAMP);
		}
		GL2 gl = jr.globalGL;
		gl.glPushAttrib(GL2.GL_ENABLE_BIT);
		gl.glEnable(GL.GL_BLEND);
		gl.glDisable(GL2.GL_LIGHTING);
		gl.glDepthMask(true);
		JOGLConfiguration.glBlendFunc(gl);
		gl.glColor3d(1, 1, 1);
		double[] c2o = jr.getContext().getCameraToObject();
		gl.glActiveTexture(GL.GL_TEXTURE0);
		gl.glEnable(GL.GL_TEXTURE_2D);
		int oldTC = jr.renderingState.texUnitCount;
		jr.renderingState.texUnitCount = 1;
		double[] bbm = new double[16];
		jr.renderingState.smoothShading = true;
		jr.renderingState.currentAlpha = 1.0;
		for (int i = 0, n = labels.length; i < n; i++) {
			ImageData img = labels[i];
			tex2d.setImage(img);
			LabelUtility.calculateBillboardMatrix(bbm, img.getWidth() * scale,
					img.getHeight() * scale, offset, alignment, c2o,
					LabelUtility.positionFor(i, vertices, indices),
					jr.renderingState.currentMetric); // )Pn.EUCLIDEAN);
			Texture2DLoaderJOGL.render(gl, tex2d);
			gl.glPushMatrix();
			gl.glMultTransposeMatrixd(bbm, 0);
			drawFaces(jr, bb);
			gl.glPopMatrix();
		}
		gl.glPopAttrib();
		jr.renderingState.texUnitCount = oldTC;
	}

	private static double[] correctionNDC = null;
	static {
		correctionNDC = Rn.identityMatrix(4);
		correctionNDC[10] = correctionNDC[11] = .5;
	}

	private static double[] clipPlane = { 0d, 0d, -1d, 0d };

	/**
	 * 
	 */
	final static int clipBase = GL2.GL_CLIP_PLANE0;

	public static void processClippingPlanes(JOGLRenderer jr,
			List<SceneGraphPath> clipPlanes) {
		int n = clipPlanes.size();
		jr.renderingState.currentClippingPlane = clipBase;
		// globalGL.glDisable(GL.GL_CLIP_PLANE0);
		for (int i = 0; i < n; ++i) {
			SceneGraphPath lp = (SceneGraphPath) clipPlanes.get(i);
			// JOGLConfiguration.theLog.log(Level.INFO,"Light"+i+":
			// "+lp.toString());
			SceneGraphNode cp = lp.getLastElement();
			if (!(cp instanceof ClippingPlane))
				JOGLConfiguration.theLog.log(Level.WARNING,
						"Invalid clipplane class " + cp.getClass().toString());
			else {
				if (((ClippingPlane) cp).isLocal())
					continue;
				double[] mat = lp.getMatrix(null);
				jr.globalGL.glPushMatrix();
				jr.globalGL.glMultTransposeMatrixd(mat, 0);
				pushClippingPlane(jr, ((ClippingPlane) cp).getPlane());
				jr.globalGL.glPopMatrix();
			}
		}
	}

	public static void pushClippingPlane(JOGLRenderer jr, double[] plane) {
		GL2 gl = jr.globalGL;
		gl.glClipPlane(jr.renderingState.currentClippingPlane,
				plane == null ? clipPlane : plane, 0);
		gl.glEnable(jr.renderingState.currentClippingPlane);
		jr.renderingState.currentClippingPlane++;
	}

	// calls to clipping plane have to be properly nested
	public static void popClippingPlane(JOGLRenderer jr) {
		jr.renderingState.currentClippingPlane--;
		jr.globalGL.glDisable(jr.renderingState.currentClippingPlane);
	}

}
