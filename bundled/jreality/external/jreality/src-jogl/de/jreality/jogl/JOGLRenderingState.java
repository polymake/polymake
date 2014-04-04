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

import javax.media.opengl.GL;
import javax.media.opengl.GL2;

import de.jreality.math.Pn;
import de.jreality.math.Rn;
import de.jreality.scene.Geometry;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.pick.Graphics3D;
import de.jreality.util.CameraUtility;

/**
 * @author gunn
 * 
 */
public class JOGLRenderingState {

	public static boolean useOldTransparency = false;
	public static boolean useGLSL = false;

	public JOGLRenderer renderer;
	public Geometry currentGeometry = null;

	public boolean smoothShading = true;
	public boolean lighting = true;
	public boolean backFaceCullingEnabled = false;
	public boolean zbufferEnabled = true;
	public boolean flipped = false;
	public boolean transparencyEnabled = false;
	public boolean fogEnabled;
	public boolean localLightModel = false;
	public boolean separateSpecularColor = false;
	public boolean ignoreAlpha0 = true;
	public boolean renderGeometryOnly = false;
	public boolean insideDisplayList = false;
	public boolean componentDisplayLists = false;
	public boolean currentPickMode = false;
	public boolean useDisplayLists = true;
	public boolean clearColorBuffer = true;
	public boolean useVertexColors = false; // for line shaders a question
	public boolean normals4d = false;
	public boolean shadeGeometry = true; // may allow shading to occur at scene
											// graph component
	public boolean oneTexture2DPerImage = false; // the conservative choice:
													// always render texture
													// anew

	public int activeTexture;
	public int frontBack = GL.GL_FRONT_AND_BACK;
	public int numLights = 0;
	public int currentClippingPlane = 0;
	public int currentEye = CameraUtility.MIDDLE_EYE;
	public int clearBufferBits = GL.GL_COLOR_BUFFER_BIT
			| GL.GL_DEPTH_BUFFER_BIT;
	public int colorMask = 0xf;
	public int currentMetric = Pn.EUCLIDEAN;
	public int texUnitCount = 0;
	public int polygonCount = 0;
	public int stereoType = JOGLViewer.CROSS_EYED_STEREO;
	protected int[] sphereDisplayLists = null;
	protected int[] cylinderDisplayLists = null;

	public double pointSize = 1.0, lineWidth = 1.0;
	public double levelOfDetail;
	public double depthFudgeFactor;
	public double currentAlpha = 1.0;
	public double globalAntiAliasingFactor = 1.0; // for offscreen rendering,
													// when image will be
													// anti-aliased
	public double[] cameraToWorld = Rn.identityMatrix(4), worldToCamera = Rn
			.identityMatrix(4), cameraToNDC = Rn.identityMatrix(4);

	public float[][] subWindowTform = { { 1, 0, 0 }, { 0, 1, 0 } };
	public float[] diffuseColor = new float[4];

	public SceneGraphPath currentPath = new SceneGraphPath();
	public Graphics3D context;

	public JOGLRenderingState(JOGLRenderer jr) {
		super();
		this.renderer = jr;
	}

	public static boolean equals(float[] a, float[] b, float tol) {
		int n = a.length;
		for (int i = 0; i < n; ++i)
			if (Math.abs(a[i] - b[i]) > tol)
				return false;
		return true;
	}

	public void initializeGLState() {
		// TODO clean this up, provide an interface to set
		// "OpenGL Preferences ..."
		// and make sure everything is here.
		// set drawing color and point size
		GL2 gl = renderer.globalGL;
		gl.glDepthMask(true);
		gl.glDisable(GL.GL_BLEND);
		gl.glColor3f(0.0f, 0.0f, 0.0f);
		gl.glEnable(GL.GL_DEPTH_TEST); // Enables Depth Testing
		gl.glDepthFunc(GL.GL_LEQUAL); // The Type Of Depth Testing To Do
		gl.glEnable(GL2.GL_ALPHA_TEST);
		gl.glAlphaFunc(GL.GL_GREATER, 0f); // alpha = 0 gets ignored in fragment
											// shader: cheap transparency
		gl.glClearDepth(1.0f);
		gl.glEnable(GL2.GL_NORMALIZE);
		gl.glEnable(GL.GL_MULTISAMPLE);
		gl.glEnable(GL2.GL_VERTEX_PROGRAM_TWO_SIDE_ARB);
		gl.glLightModeli(GL2.GL_LIGHT_MODEL_TWO_SIDE, GL.GL_TRUE);
		float[] white = { 1f, 1f, 1f, 1f };
		gl.glLightModelfv(GL2.GL_LIGHT_MODEL_AMBIENT, white, 0);
		float[] amb = { 0f, 0f, 0f };
		float[] spec = { .5f, .5f, .5f };
		gl.glMaterialfv(frontBack, GL2.GL_AMBIENT, amb, 0);
		gl.glMaterialfv(frontBack, GL2.GL_DIFFUSE, new float[] { 1, 0, 0 }, 0);
		gl.glMaterialfv(frontBack, GL2.GL_SPECULAR, spec, 0);
		gl.glMaterialf(frontBack, GL2.GL_SHININESS, 60f);
		gl.glEnable(GL2.GL_COLOR_MATERIAL);
		gl.glColorMaterial(frontBack, GL2.GL_DIFFUSE);

		if (smoothShading)
			gl.glShadeModel(GL2.GL_SMOOTH);
		else
			gl.glShadeModel(GL2.GL_FLAT);

		if (flipped)
			gl.glFrontFace(GL.GL_CW);
		else
			gl.glFrontFace(GL.GL_CCW);
	}

	public int getCylinderDisplayLists(int i) {
		if (cylinderDisplayLists == null)
			cylinderDisplayLists = JOGLCylinderUtility
					.getCylinderDLists(renderer);
		return cylinderDisplayLists[i];
	}

	public int getSphereDisplayLists(int i) {
		if (sphereDisplayLists == null)
			sphereDisplayLists = JOGLSphereHelper.getSphereDLists(renderer);
		return sphereDisplayLists[i];
	}

	public boolean isClearColorBuffer() {
		return clearColorBuffer;
	}

	public void setClearColorBuffer(boolean clearColorBuffer) {
		this.clearColorBuffer = clearColorBuffer;
	}
}
