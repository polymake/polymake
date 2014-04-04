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

import de.jreality.jogl.shader.JOGLTexture2D;
import de.jreality.jogl.shader.Texture2DLoaderJOGL;
import de.jreality.math.P3;
import de.jreality.math.Pn;
import de.jreality.scene.Appearance;
import de.jreality.scene.Camera;
import de.jreality.scene.data.AttributeEntityUtility;
import de.jreality.shader.CubeMap;
import de.jreality.shader.ImageData;
import de.jreality.shader.Texture2D;
import de.jreality.shader.TextureUtility;

/**
 * @author Charles Gunn
 * 
 */
class JOGLSkyBox {
	static boolean mirrored = false;
	// TODO straighten out nomenclature on faces
	static private double[][][] cubeVerts3 = {
			{ { 1, 1, 1 }, { 1, 1, -1 }, { 1, -1, -1 }, { 1, -1, 1 } }, // right
			{ { -1, 1, -1 }, { -1, 1, 1 }, { -1, -1, 1 }, { -1, -1, -1 } }, // left
			{ { -1, 1, -1 }, { 1, 1, -1 }, { 1, 1, 1 }, { -1, 1, 1 } }, // up
			{ { -1, -1, 1 }, { 1, -1, 1 }, { 1, -1, -1 }, { -1, -1, -1 } }, // down
			{ { -1, 1, 1 }, { 1, 1, 1 }, { 1, -1, 1 }, { -1, -1, 1 } }, // back
			{ { 1, 1, -1 }, { -1, 1, -1 }, { -1, -1, -1 }, { 1, -1, -1 } } // front
	};

	// TODO figure out texture coordinates
	static private double[][] texCoords = { { 0, 0 }, { 1, 0 }, { 1, 1 },
			{ 0, 1 } };

	static Appearance a = new Appearance();
	static Texture2D tex = (Texture2D) AttributeEntityUtility
			.createAttributeEntity(Texture2D.class, "", a, true);
	static JOGLTexture2D jogltex;
	static {
		tex.setRepeatS(de.jreality.shader.Texture2D.GL_CLAMP_TO_EDGE);
		tex.setRepeatT(de.jreality.shader.Texture2D.GL_CLAMP_TO_EDGE);
		jogltex = new JOGLTexture2D(tex);
	}

	static void render(GL2 gl, double[] w2c, CubeMap cm, Camera cam) {
		ImageData[] imgs = TextureUtility.getCubeMapImages(cm);
		jogltex.setBlendColor(cm.getBlendColor());
		gl.glPushAttrib(GL2.GL_ENABLE_BIT);
		gl.glDisable(GL.GL_BLEND);
		gl.glDisable(GL.GL_DEPTH_TEST);
		gl.glDisable(GL2.GL_LIGHTING);
		gl.glActiveTexture(GL.GL_TEXTURE0);
		gl.glEnable(GL.GL_TEXTURE_2D);
		float[] white = { 1f, 1f, 1f, 1f };
		// gl.glTexEnvfv(GL.GL_TEXTURE_ENV, GL.GL_TEXTURE_ENV_COLOR, white);
		gl.glColor4fv(white, 0);
		gl.glPushMatrix();

		gl.glLoadTransposeMatrixd(P3.extractOrientationMatrix(null, w2c,
				P3.originP3, Pn.EUCLIDEAN), 0);
		double scale = (cam.getNear() + cam.getFar()) / 2;
		// Here's where you can control whether the images appear mirrored or
		// not
		gl.glMultTransposeMatrixd(
				P3.makeStretchMatrix(null, new double[] {
						(mirrored ? -1 : 1) * scale, scale, scale }), 0);
		for (int i = 0; i < 6; ++i) {
			jogltex.setImage(imgs[i]);
			Texture2DLoaderJOGL.render(gl, jogltex);
			gl.glBegin(GL2.GL_POLYGON);
			for (int j = 0; j < 4; ++j) {
				gl.glTexCoord2dv(texCoords[j], 0);
				gl.glVertex3dv(cubeVerts3[i][j], 0);
			}
			gl.glEnd();
		}
		gl.glPopMatrix();
		gl.glPopAttrib();
	}

}
