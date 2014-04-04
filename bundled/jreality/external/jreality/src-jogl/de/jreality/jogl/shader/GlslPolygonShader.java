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

import static de.jreality.shader.CommonAttributes.REFLECTION_MAP;
import static de.jreality.shader.CommonAttributes.TEXTURE_2D;
import static de.jreality.shader.CommonAttributes.TEXTURE_2D_1;
import static de.jreality.shader.CommonAttributes.TEXTURE_2D_2;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.DoubleBuffer;
import java.nio.IntBuffer;
import java.util.WeakHashMap;

import javax.media.opengl.DebugGL2;
import javax.media.opengl.GL;
import javax.media.opengl.GL2;

import de.jreality.geometry.GeometryUtility;
import de.jreality.jogl.JOGLRenderer;
import de.jreality.jogl.JOGLRendererHelper;
import de.jreality.jogl.JOGLRenderingState;
import de.jreality.math.Pn;
import de.jreality.math.Rn;
import de.jreality.scene.Appearance;
import de.jreality.scene.Cylinder;
import de.jreality.scene.Geometry;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.Sphere;
import de.jreality.scene.data.Attribute;
import de.jreality.scene.data.AttributeEntityUtility;
import de.jreality.scene.data.DataList;
import de.jreality.scene.data.DoubleArray;
import de.jreality.scene.data.DoubleArrayArray;
import de.jreality.scene.data.IntArray;
import de.jreality.scene.data.IntArrayArray;
import de.jreality.scene.data.StorageModel;
import de.jreality.scene.event.GeometryEvent;
import de.jreality.scene.event.GeometryListener;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.CubeMap;
import de.jreality.shader.EffectiveAppearance;
import de.jreality.shader.GlslProgram;
import de.jreality.shader.ShaderUtility;
import de.jreality.shader.Texture2D;

/**
 * it is assumed that the shader source code stayes FIXED!
 * 
 * @author Steffen Weissmann
 * 
 */
public class GlslPolygonShader extends AbstractPrimitiveShader implements
		PolygonShader {

	private static final int PER_VERTEX = 0;
	private static final int PER_FACE = 1;
	private static final int PER_PART = 2;
	GlslProgram program;

	Texture2D texture1, texture0, texture2;

	CubeMap environmentMap;
	private DefaultVertexShader vertexShader = new DefaultVertexShader();
	private boolean smoothShading;
	// private int frontBack = DefaultPolygonShader.FRONT_AND_BACK;
	private boolean useVertexArrays = true, doNormals4 = false;

	public void setFromEffectiveAppearance(EffectiveAppearance eap, String name) {
		super.setFromEffectiveAppearance(eap, name);
		eap.getAttribute(
				ShaderUtility.nameSpace(name, CommonAttributes.METRIC),
				Pn.EUCLIDEAN);
		smoothShading = eap.getAttribute(
				ShaderUtility.nameSpace(name, CommonAttributes.SMOOTH_SHADING),
				CommonAttributes.SMOOTH_SHADING_DEFAULT);
		useVertexArrays = eap.getAttribute(
				ShaderUtility.nameSpace(name, "useVertexArrays"), true);
		if (GlslProgram.hasGlslProgram(eap, name)) {
			Appearance app = new Appearance();
			EffectiveAppearance eap2 = eap.create(app);
			program = new GlslProgram(app, eap2, name);
		} else
			program = null;
		// TODO remove duplicate names after steffen has refactored the texture
		// and reflection map stuff
		if (AttributeEntityUtility.hasAttributeEntity(Texture2D.class,
				ShaderUtility.nameSpace(name, TEXTURE_2D), eap)) {
			texture0 = (Texture2D) AttributeEntityUtility
					.createAttributeEntity(Texture2D.class,
							ShaderUtility.nameSpace(name, TEXTURE_2D), eap);
		} else
			texture0 = null;
		if (AttributeEntityUtility.hasAttributeEntity(Texture2D.class,
				ShaderUtility.nameSpace(name, TEXTURE_2D_1), eap)) {
			texture1 = (Texture2D) AttributeEntityUtility
					.createAttributeEntity(Texture2D.class,
							ShaderUtility.nameSpace(name, TEXTURE_2D_1), eap);
		} else
			texture1 = null;
		if (AttributeEntityUtility.hasAttributeEntity(Texture2D.class,
				ShaderUtility.nameSpace(name, TEXTURE_2D_2), eap)) {
			texture2 = (Texture2D) AttributeEntityUtility
					.createAttributeEntity(Texture2D.class,
							ShaderUtility.nameSpace(name, TEXTURE_2D_2), eap);
		} else
			texture2 = null;
		if (AttributeEntityUtility.hasAttributeEntity(CubeMap.class,
				ShaderUtility.nameSpace(name, REFLECTION_MAP), eap)) {
			environmentMap = (CubeMap) AttributeEntityUtility
					.createAttributeEntity(CubeMap.class,
							ShaderUtility.nameSpace(name, REFLECTION_MAP), eap);
		} else
			environmentMap = null;
		// vertexShader = (VertexShader) ShaderLookup.getShaderAttr(eap, name,
		// CommonAttributes.VERTEX_SHADER);
		vertexShader.setFromEffectiveAppearance(eap, name);
		// System.err.println("glslpolygonshader: set from eap "+program);
		needsChecked = true;
		geometryHasTextureCoordinates = false;
	}

	boolean needsChecked = true, geometryHasTextureCoordinates,
			doTexture = false;

	public void render(JOGLRenderingState jrs) {
		JOGLRenderer jr = jrs.renderer;
		GL2 gl = jr.globalGL;
		if (smoothShading)
			gl.glShadeModel(GL2.GL_SMOOTH);
		else
			gl.glShadeModel(GL2.GL_FLAT);
		jrs.smoothShading = smoothShading;

		// vertexShader.setFrontBack(frontBack);
		vertexShader.render(jrs);

		doTexture = false;
		if (texture0 != null) {
			Geometry curgeom = jr.renderingState.currentGeometry;
			if (needsChecked) // assume geometry stays constant between calls to
								// setFromEffectiveAppearance() ...
				if (curgeom != null
						&& (curgeom instanceof IndexedFaceSet)
						&& ((IndexedFaceSet) curgeom)
								.getVertexAttributes(Attribute.TEXTURE_COORDINATES) != null) {
					geometryHasTextureCoordinates = true;
					needsChecked = false;
				}
			if (geometryHasTextureCoordinates) {
				gl.glActiveTexture(GL.GL_TEXTURE0);
				Texture2DLoaderJOGL.render(jr.globalGL, texture0);
				gl.glEnable(GL.GL_TEXTURE_2D);
				// if (program != null &&
				// program.getSource().getUniformParameter("texture") != null) {
				// doTexture = true;
				// program.setUniform("texture",GL.GL_TEXTURE0);
				// System.err.println("Setting texture to "+GL.GL_TEXTURE0);
				// }
			}
		}
		if (program != null
				&& program.getSource().getUniformParameter("doTexture") != null) {
			program.setUniform("doTexture", doTexture);
			System.err.println("Setting do texture = " + doTexture);
		}

		if (texture0 != null) {
			gl.glActiveTexture(GL.GL_TEXTURE0);
			Texture2DLoaderJOGL.render(jr.globalGL, texture0);
			gl.glEnable(GL.GL_TEXTURE_2D);
		}

		if (texture1 != null) {
			gl.glActiveTexture(GL.GL_TEXTURE1);
			Texture2DLoaderJOGL.render(jr.globalGL, texture1);
			gl.glEnable(GL.GL_TEXTURE_2D);
		}
		if (texture2 != null) {
			gl.glActiveTexture(GL.GL_TEXTURE2);
			Texture2DLoaderJOGL.render(jr.globalGL, texture2);
			gl.glEnable(GL.GL_TEXTURE_2D);
		}
		if (environmentMap != null) {
			gl.glActiveTexture(GL.GL_TEXTURE3);
			Texture2DLoaderJOGL.render(jr, environmentMap);
			gl.glEnable(GL.GL_TEXTURE_CUBE_MAP);
		}
		if (program != null) {
			if (program.getSource().getUniformParameter("lightingEnabled") != null) {
				program.setUniform("lightingEnabled", jrs.lighting);
				System.err.println("setting lighting to " + jrs.lighting);
			}
			if (program.getSource().getUniformParameter("transparency") != null) {
				program.setUniform("transparency",
						jrs.transparencyEnabled ? jrs.diffuseColor[3] : 0f);
			}
			if (program.getSource().getAttribute("normals4") != null) {
				doNormals4 = true;
			} else
				doNormals4 = false;
			// System.err.println("normals4 = "+doNormals4);
			GlslLoader.render(program, jr);
		}
		Geometry g = jrs.currentGeometry;
		if (g != null) {
			if (g instanceof Sphere || g instanceof Cylinder) {
				int i = 3;
				int dlist;
				if (g instanceof Sphere)
					dlist = jr.renderingState.getSphereDisplayLists(i);
				else
					dlist = jr.renderingState.getCylinderDisplayLists(i);
				jr.globalGL.glCallList(dlist);
			} else if (g instanceof IndexedFaceSet) {
				if (useVertexArrays)
					drawFaces(jr, (IndexedFaceSet) g, smoothShading,
							jrs.diffuseColor[3], doNormals4);
				else { // use display lists to render
					if (!upToDate((IndexedFaceSet) g, smoothShading)
							|| dList == -1) {
						if (dList != -1)
							jr.globalGL.glDeleteLists(dList, 1);
						dList = jr.globalGL.glGenLists(1);
						jr.globalGL.glNewList(dList, GL2.GL_COMPILE);
						JOGLRendererHelper.drawFaces(jr, (IndexedFaceSet) g);
						jr.globalGL.glEndList();
					}
					jr.globalGL.glCallList(dList);
				}
			}
		}
	}

	public void postRender(JOGLRenderingState jrs) {
		JOGLRenderer jr = jrs.renderer;
		GL gl = jr.globalGL;
		if (program != null)
			GlslLoader.postRender(program, jr);
		if (texture0 != null) {
			gl.glActiveTexture(GL.GL_TEXTURE0);
			gl.glDisable(GL.GL_TEXTURE_2D);
		}
		if (texture1 != null) {
			gl.glActiveTexture(GL.GL_TEXTURE1);
			gl.glDisable(GL.GL_TEXTURE_2D);
		}
		if (texture2 != null) {
			gl.glActiveTexture(GL.GL_TEXTURE2);
			gl.glDisable(GL.GL_TEXTURE_2D);
		}
		if (environmentMap != null) {
			gl.glActiveTexture(GL.GL_TEXTURE2);
			gl.glDisable(GL.GL_TEXTURE_CUBE_MAP);
			gl.glDisable(GL2.GL_TEXTURE_GEN_S);
			gl.glDisable(GL2.GL_TEXTURE_GEN_T);
			gl.glDisable(GL2.GL_TEXTURE_GEN_R);
		}
	}

	/**
	 * Method does nothing! FrontBack is private and nowhere used inside
	 * GlslPolygonShader.
	 * 
	 * @param f
	 */
	public void setFrontBack(int f) {
		// frontBack = f;
	}

	public void setProgram(GlslProgram program) {
		this.program = program;
	}

	public static void drawFaces(JOGLRenderer jr, IndexedFaceSet sg,
			boolean smooth, double alpha, boolean doNormals4) {
		if (sg.getNumFaces() == 0)
			return;
		GL2 gl = jr.globalGL;

		int colorBind = -1, normalBind, colorLength = 3;
		DataList vertices = sg.getVertexAttributes(Attribute.COORDINATES);
		DataList vertexNormals = sg.getVertexAttributes(Attribute.NORMALS);
		DataList faceNormals = sg.getFaceAttributes(Attribute.NORMALS);
		DataList vertexColors = sg.getVertexAttributes(Attribute.COLORS);
		DataList faceColors = sg.getFaceAttributes(Attribute.COLORS);
		DataList texCoords0 = sg
				.getVertexAttributes(Attribute.TEXTURE_COORDINATES);
		DataList texCoords1 = sg
				.getVertexAttributes(Attribute.TEXTURE_COORDINATES1);
		DataList texCoords2 = sg
				.getVertexAttributes(Attribute.TEXTURE_COORDINATES2);
		DataList lightMapCoords = sg.getVertexAttributes(Attribute
				.attributeForName("lightmap coordinates"));
		// JOGLConfiguration.theLog.log(Level.INFO,"Vertex normals are:
		// "+((vertexNormals != null) ? vertexNormals.size() : 0));
		// JOGLConfiguration.theLog.log(Level.INFO,"alpha value is "+alpha);

		// vertex color has priority over face color
		vertices = sg.getVertexAttributes(Attribute.COORDINATES);
		int vertexLength = GeometryUtility.getVectorLength(vertices);
		if (vertexColors != null && smooth) {
			colorBind = PER_VERTEX;
			colorLength = GeometryUtility.getVectorLength(vertexColors);
		} else if (faceColors != null) {
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
		if (vertexNormals != null && smooth) {
			normalBind = PER_VERTEX;
		} else if (faceNormals != null) {
			normalBind = PER_FACE;
		} else
			normalBind = PER_PART;
		renderFaces(sg, alpha, gl, false, colorBind, normalBind, colorLength,
				vertices, vertexNormals, faceNormals, vertexColors, faceColors,
				texCoords0, texCoords1, texCoords2, lightMapCoords,
				vertexLength, smooth, doNormals4);
	}

	public static DataList correctNormals(DataList n) {
		if (n != null && n.toDoubleArrayArray().item(0).size() == 4) {
			double[][] norms = n.toDoubleArrayArray(null);
			double[][] norms3 = new double[norms.length][3];
			for (int i = 0; i < norms.length; ++i) {
				Pn.dehomogenize(norms3[i], norms[i]);
				if (norms[i][3] < 0)
					Rn.times(norms3[i], -1, norms3[i]);
				if (norms[i][3] == 0.0)
					Rn.times(norms3[i], 10000000, norms3[i]);
			}
			return StorageModel.DOUBLE_ARRAY.array(3).createReadOnly(norms3);
		}
		return n;
	}

	public static DataList correctNormals4(DataList n) {
		if (n != null && n.toDoubleArrayArray().item(0).size() == 3) {
			double[][] norms = n.toDoubleArrayArray(null);
			double[][] norms4 = new double[norms.length][4];
			for (int i = 0; i < norms.length; ++i) {
				System.arraycopy(norms[i], 0, norms4[i], 0, 3);
				norms4[i][3] = 10E-8; // assume euclidean normal vectors but
										// avoid orientation problems with
										// setting 0.0 here
			}
			return StorageModel.DOUBLE_ARRAY.array(4).createReadOnly(norms4);
		}
		return n;
	}

	private static void renderFaces(IndexedFaceSet sg, double alpha, GL2 gl,
			boolean pickMode, int colorBind, int normalBind, int colorLength,
			DataList vertices, DataList vertexNormals, DataList faceNormals,
			DataList vertexColors, DataList faceColors, DataList texCoords0,
			DataList texCoords1, DataList texCoords2, DataList lightMapCoords,
			int vertexLength, boolean smooth, boolean doNormals4) {
		// System.err.println("rendering with vertex arrays");
		boolean faceN = normalBind == PER_FACE;

		boolean faceC = colorBind == PER_FACE;

		boolean faceT = false;
		Attribute TANGENTS = Attribute.attributeForName("TANGENTS");

		DataList tanCoords = null;
		// System.err.println("face color = "+faceC);
		if (doNormals4) {
			if (faceN) {
				tanCoords = correctNormals4(faceNormals);
			} else
				tanCoords = correctNormals4(vertexNormals);
			faceT = faceN;
			faceNormals = vertexNormals = null;
		} else {
			if (faceN)
				faceNormals = correctNormals(faceNormals);
			else
				vertexNormals = correctNormals(vertexNormals);
		}
		if (tanCoords == null) {
			tanCoords = sg.getVertexAttributes(TANGENTS);
		}

		boolean renderInlined = (normalBind == PER_VERTEX || faceN)
				&& (colorBind == PER_VERTEX || colorBind == PER_PART || faceC);

		if (renderInlined) {

			gl = new DebugGL2(gl);

			// count indices
			int triagCnt = 0;
			IntArrayArray faces = sg.getFaceAttributes(Attribute.INDICES)
					.toIntArrayArray();
			int numFaces = faces.getLength();
			for (int i = 0; i < numFaces; i++) {
				triagCnt += (faces.getLengthAt(i) - 2);
			}

			boolean hasColors = vertexColors != null || faceColors != null;

			IntBuffer indexBuffer = null;
			boolean inlineI = false; // = normalBind == PER_FACE || hasColors;
			if (inlineI) {
				indexBuffer = BufferCache.index(sg, triagCnt);
			}
			DoubleBuffer vertexBuffer = null;
			double[] tmpV = new double[vertexLength];
			boolean inlineV = true;
			if (inlineV) {
				vertexBuffer = BufferCache.vertex(sg, triagCnt, vertexLength);
			}
			// handle 0th texture coordinates
			DoubleArrayArray tc0 = null;
			int texLength0 = 0;
			double[] tmpTex0 = null;
			DoubleBuffer texBuffer0 = null;
			boolean inlineTex0 = texCoords0 != null;
			if (inlineTex0) {
				tc0 = texCoords0.toDoubleArrayArray();
				texLength0 = tc0.getLengthAt(0);
				tmpTex0 = new double[texLength0];
				texBuffer0 = BufferCache.texCoord0(sg, triagCnt, texLength0);
			}
			// handle 1st texture coordinates
			DoubleArrayArray tc1 = null;
			int texLength1 = 1;
			double[] tmpTex1 = null;
			DoubleBuffer texBuffer1 = null;
			boolean inlineTex1 = texCoords1 != null;
			if (inlineTex1) {
				tc1 = texCoords1.toDoubleArrayArray();
				texLength1 = tc1.getLengthAt(0);
				tmpTex1 = new double[texLength1];
				texBuffer1 = BufferCache.texCoord1(sg, triagCnt, texLength1);
			}
			// handle 2th texture coordinates
			DoubleArrayArray tc2 = null;
			int texLength2 = 2;
			double[] tmpTex2 = null;
			DoubleBuffer texBuffer2 = null;
			boolean inlineTex2 = texCoords2 != null;
			if (inlineTex2) {
				tc2 = texCoords2.toDoubleArrayArray();
				texLength2 = tc2.getLengthAt(0);
				tmpTex2 = new double[texLength2];
				texBuffer2 = BufferCache.texCoord2(sg, triagCnt, texLength2);
			}
			double[] tmpTan = new double[4];
			// this can be currently either tangents or normal4 field
			// tangents are store in texture coords1. if these are set already,
			// ignore tangents
			boolean inlineTan = tanCoords != null && !inlineTex1;
			DoubleBuffer tanBuffer = null;
			if (inlineTan) {
				tanBuffer = BufferCache.tangent(sg, triagCnt, 4);
			}

			double[] tmpN = new double[3];
			boolean inlineN = !doNormals4;
			DoubleBuffer normalBuffer = null;
			if (inlineN) {
				normalBuffer = BufferCache.normal(sg, triagCnt);
			}

			double[] tmpC = new double[colorLength];
			boolean inlineC = hasColors;
			DoubleBuffer colorBuffer = null;
			if (inlineC) {
				colorBuffer = BufferCache.color(sg, triagCnt, colorLength);
			}

			if (!upToDate(sg, smooth)) {

				DoubleArray da;

				DoubleArrayArray verts = vertices.toDoubleArrayArray();
				tc0 = inlineTex0 ? tc0 : null;
				DoubleArrayArray t = inlineTan ? tanCoords.toDoubleArrayArray()
						: null;
				DoubleArrayArray norms = null;
				if (!doNormals4)
					norms = faceN ? faceNormals.toDoubleArrayArray()
							: vertexNormals.toDoubleArrayArray();
				DoubleArrayArray cols = inlineC ? (faceC ? faceColors
						.toDoubleArrayArray() : vertexColors
						.toDoubleArrayArray()) : null;

				for (int i = 0; i < numFaces; i++) {
					IntArray face = faces.getValueAt(i);
					for (int j = 0; j < face.getLength() - 2; j++) {
						final int i1 = face.getValueAt(0);
						final int i2 = face.getValueAt(j + 1);
						final int i3 = face.getValueAt(j + 2);
						if (inlineI) {
							indexBuffer.put(i1);
							indexBuffer.put(i2);
							indexBuffer.put(i3);
						}
						if (inlineV) {
							da = verts.getValueAt(i1);
							da.toDoubleArray(tmpV);
							try {
								vertexBuffer.put(tmpV);
							} catch (Exception e) {
								System.out.println(vertexBuffer);
								System.out.println("triags=" + triagCnt);
							}
							da = verts.getValueAt(i2);
							da.toDoubleArray(tmpV);
							vertexBuffer.put(tmpV);
							da = verts.getValueAt(i3);
							da.toDoubleArray(tmpV);
							vertexBuffer.put(tmpV);
						}
						if (inlineTex0) {
							da = tc0.getValueAt(i1);
							da.toDoubleArray(tmpTex0);
							texBuffer0.put(tmpTex0);
							da = tc0.getValueAt(i2);
							da.toDoubleArray(tmpTex0);
							texBuffer0.put(tmpTex0);
							da = tc0.getValueAt(i3);
							da.toDoubleArray(tmpTex0);
							texBuffer0.put(tmpTex0);
						}
						if (inlineTex1) {
							da = tc1.getValueAt(i1);
							da.toDoubleArray(tmpTex1);
							texBuffer1.put(tmpTex1);
							da = tc1.getValueAt(i2);
							da.toDoubleArray(tmpTex1);
							texBuffer1.put(tmpTex1);
							da = tc1.getValueAt(i3);
							da.toDoubleArray(tmpTex1);
							texBuffer1.put(tmpTex1);
						}
						if (inlineTex2) {
							da = tc2.getValueAt(i1);
							da.toDoubleArray(tmpTex2);
							texBuffer2.put(tmpTex2);
							da = tc2.getValueAt(i2);
							da.toDoubleArray(tmpTex2);
							texBuffer2.put(tmpTex2);
							da = tc2.getValueAt(i3);
							da.toDoubleArray(tmpTex2);
							texBuffer2.put(tmpTex2);
						}
						if (inlineTan) {
							da = t.getValueAt(faceT ? i : i1);
							da.toDoubleArray(tmpTan);
							tanBuffer.put(tmpTan);
							if (!faceT) {
								da = t.getValueAt(i2);
								da.toDoubleArray(tmpTan);
							}
							tanBuffer.put(tmpTan);
							if (!faceT) {
								da = t.getValueAt(i3);
								da.toDoubleArray(tmpTan);
							}
							tanBuffer.put(tmpTan);
						}
						if (inlineN) {
							da = norms.getValueAt(faceN ? i : i1);
							da.toDoubleArray(tmpN);
							normalBuffer.put(tmpN);
							if (!faceN) {
								da = norms.getValueAt(i2);
								da.toDoubleArray(tmpN);
							}
							normalBuffer.put(tmpN);
							if (!faceN) {
								da = norms.getValueAt(i3);
								da.toDoubleArray(tmpN);
							}
							normalBuffer.put(tmpN);
						}
						if (inlineC) {
							da = cols.getValueAt(faceC ? i : i1);
							da.toDoubleArray(tmpC);
							colorBuffer.put(tmpC);
							if (!faceC) {
								da = cols.getValueAt(i2);
								da.toDoubleArray(tmpC);
							}
							colorBuffer.put(tmpC);
							if (!faceC) {
								da = cols.getValueAt(i3);
								da.toDoubleArray(tmpC);
							}
							colorBuffer.put(tmpC);
						}
					}
				}
			}
			vertexBuffer.rewind();
			if (!doNormals4)
				normalBuffer.rewind();

			gl.glEnableClientState(GL2.GL_VERTEX_ARRAY);
			if (!doNormals4)
				gl.glEnableClientState(GL2.GL_NORMAL_ARRAY);

			gl.glVertexPointer(vertexLength, GL2.GL_DOUBLE, 0, vertexBuffer);
			if (!doNormals4)
				gl.glNormalPointer(GL2.GL_DOUBLE, 0, normalBuffer);
			if (hasColors) {
				gl.glEnableClientState(GL2.GL_COLOR_ARRAY);
				colorBuffer.rewind();
				gl.glColorPointer(colorLength, GL2.GL_DOUBLE, 0, colorBuffer);
			}
			if (texCoords0 != null) {
				gl.glClientActiveTexture(GL.GL_TEXTURE0);
				gl.glEnableClientState(GL2.GL_TEXTURE_COORD_ARRAY);
				texBuffer0.rewind();
				gl.glTexCoordPointer(texLength0, GL2.GL_DOUBLE, 0, texBuffer0);
			}
			if (texCoords1 != null) {
				gl.glClientActiveTexture(GL.GL_TEXTURE1);
				gl.glEnableClientState(GL2.GL_TEXTURE_COORD_ARRAY);
				texBuffer1.rewind();
				gl.glTexCoordPointer(texLength1, GL2.GL_DOUBLE, 0, texBuffer1);
			}
			if (texCoords2 != null) {
				gl.glClientActiveTexture(GL.GL_TEXTURE2);
				gl.glEnableClientState(GL2.GL_TEXTURE_COORD_ARRAY);
				texBuffer2.rewind();
				gl.glTexCoordPointer(texLength2, GL2.GL_DOUBLE, 0, texBuffer2);
			}
			// int TANGENT_ID=9;
			if (tanCoords != null) {
				tanBuffer.rewind();
				// gl.glVertexAttribPointer(TANGENT_ID, 4, GL.GL_DOUBLE, true,
				// 0, tanBuffer);
				// gl.glEnableVertexAttribArray(TANGENT_ID);
				gl.glClientActiveTexture(GL.GL_TEXTURE1);
				gl.glEnableClientState(GL2.GL_TEXTURE_COORD_ARRAY);
				gl.glTexCoordPointer(4, GL2.GL_DOUBLE, 0, tanBuffer);
			}
			if (inlineI) {
				indexBuffer.rewind();
				gl.glDrawElements(GL.GL_TRIANGLES, indexBuffer.remaining(),
						GL.GL_UNSIGNED_INT, indexBuffer);
			} else
				gl.glDrawArrays(GL.GL_TRIANGLES, 0, triagCnt * 3);

			gl.glDisableClientState(GL2.GL_VERTEX_ARRAY);
			if (!doNormals4)
				gl.glDisableClientState(GL2.GL_NORMAL_ARRAY);
			if (texCoords0 != null) {
				gl.glClientActiveTexture(GL.GL_TEXTURE0);
				gl.glDisableClientState(GL2.GL_TEXTURE_COORD_ARRAY);
			}
			if (texCoords1 != null) {
				gl.glClientActiveTexture(GL.GL_TEXTURE1);
				gl.glDisableClientState(GL2.GL_TEXTURE_COORD_ARRAY);
			}
			if (texCoords2 != null) {
				gl.glClientActiveTexture(GL.GL_TEXTURE2);
				gl.glDisableClientState(GL2.GL_TEXTURE_COORD_ARRAY);
			}
			if (hasColors) {
				gl.glDisableClientState(GL2.GL_COLOR_ARRAY);
			}
			if (tanCoords != null) {
				gl.glClientActiveTexture(GL.GL_TEXTURE1);
				gl.glDisableClientState(GL2.GL_TEXTURE_COORD_ARRAY);
				// gl.glDisableVertexAttribArray(TANGENT_ID);
			}
		} else {
			System.out.println("GlslPolygonShader inlined: ??");
		}
	}

	private static WeakHashMap<IndexedFaceSet, Boolean> upToDateIFS = new WeakHashMap<IndexedFaceSet, Boolean>();

	private static boolean upToDate(final IndexedFaceSet sg, boolean smooth) {
		if (upToDateIFS.get(sg) == Boolean.valueOf(smooth))
			return true;
		else {
			upToDateIFS.put(sg, Boolean.valueOf(smooth));
			sg.addGeometryListener(new GeometryListener() {
				public void geometryChanged(GeometryEvent ev) {
					if (!ev.getChangedVertexAttributes().isEmpty()
							|| !ev.getChangedFaceAttributes().isEmpty()) {
						upToDateIFS.remove(sg);
						sg.removeGeometryListener(this);
					}
				}
			});
			return false;
		}
	}

	private static final class BufferCache {

		static WeakHashMap<IndexedFaceSet, ByteBuffer> vertexBuffers = new WeakHashMap<IndexedFaceSet, ByteBuffer>();
		static WeakHashMap<IndexedFaceSet, ByteBuffer> texCoord0Buffers = new WeakHashMap<IndexedFaceSet, ByteBuffer>();
		static WeakHashMap<IndexedFaceSet, ByteBuffer> texCoord1Buffers = new WeakHashMap<IndexedFaceSet, ByteBuffer>();
		static WeakHashMap<IndexedFaceSet, ByteBuffer> texCoord2Buffers = new WeakHashMap<IndexedFaceSet, ByteBuffer>();
		static WeakHashMap<IndexedFaceSet, ByteBuffer> tangentBuffers = new WeakHashMap<IndexedFaceSet, ByteBuffer>();
		static WeakHashMap<IndexedFaceSet, ByteBuffer> normalBuffers = new WeakHashMap<IndexedFaceSet, ByteBuffer>();
		static WeakHashMap<IndexedFaceSet, ByteBuffer> colorBuffers = new WeakHashMap<IndexedFaceSet, ByteBuffer>();
		static WeakHashMap<IndexedFaceSet, ByteBuffer> indexBuffers = new WeakHashMap<IndexedFaceSet, ByteBuffer>();

		private BufferCache() {
		}

		static DoubleBuffer vertex(IndexedFaceSet ifs, int numTris,
				int vertexLen) {
			return get(ifs, vertexBuffers, numTris * 3 * vertexLen * 8)
					.asDoubleBuffer();
		}

		static DoubleBuffer texCoord0(IndexedFaceSet ifs, int numTris,
				int texcoordlength) {
			return get(ifs, texCoord0Buffers, numTris * 3 * texcoordlength * 8)
					.asDoubleBuffer();
		}

		static DoubleBuffer texCoord1(IndexedFaceSet ifs, int numTris,
				int texcoordlength) {
			return get(ifs, texCoord1Buffers, numTris * 3 * texcoordlength * 8)
					.asDoubleBuffer();
		}

		static DoubleBuffer texCoord2(IndexedFaceSet ifs, int numTris,
				int texcoordlength) {
			return get(ifs, texCoord2Buffers, numTris * 3 * texcoordlength * 8)
					.asDoubleBuffer();
		}

		static DoubleBuffer tangent(IndexedFaceSet ifs, int numTris,
				int tangentLen) {
			return get(ifs, tangentBuffers, numTris * 3 * tangentLen * 8)
					.asDoubleBuffer();
		}

		static DoubleBuffer normal(IndexedFaceSet ifs, int numTris) {
			return get(ifs, normalBuffers, numTris * 3 * 3 * 8)
					.asDoubleBuffer();
		}

		static DoubleBuffer color(IndexedFaceSet ifs, int numTris, int colorLen) {
			return get(ifs, colorBuffers, numTris * 3 * colorLen * 8)
					.asDoubleBuffer();
		}

		static IntBuffer index(IndexedFaceSet ifs, int numTris) {
			return get(ifs, indexBuffers, numTris * 3 * 4).asIntBuffer();
		}

		private static ByteBuffer get(IndexedFaceSet ifs,
				WeakHashMap<IndexedFaceSet, ByteBuffer> cache, int capacity) {
			ByteBuffer bb = cache.get(ifs);
			if (bb == null || bb.capacity() < capacity) {
				bb = ByteBuffer.allocateDirect(capacity).order(
						ByteOrder.nativeOrder());
				cache.put(ifs, bb);
			}
			bb.position(0).limit(capacity);
			return bb;
		}

	};

}
