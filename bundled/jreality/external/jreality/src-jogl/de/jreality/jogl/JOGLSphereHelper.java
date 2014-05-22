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

import java.util.WeakHashMap;
import java.util.logging.Level;

import javax.media.opengl.GL;
import javax.media.opengl.GL2;

import de.jreality.geometry.SphereUtility;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.util.CameraUtility;

/**
 * @author gunn
 * 
 */
public class JOGLSphereHelper extends SphereUtility {

	// we read this value once -- it needs to stay the same throughout execution
	static boolean sharedDisplayLists;
	static WeakHashMap<GL, int[]> sphereDListsTable = new WeakHashMap<GL, int[]>();
	static int[] globalSharedSphereDisplayLists = null;

	public static void setupSphereDLists(JOGLRenderer jr) {
		// we read this once -- had better be set to correct value when we do
		// so!
		sharedDisplayLists = JOGLConfiguration.sharedContexts;
		int[] dlists = null;
		GL2 gl = jr.globalGL;
		int n = SphereUtility.tessellatedCubes.length;
		dlists = null;
		dlists = new int[n];
		// JOGLConfiguration.theLog.log(Level.INFO,"Setting up sphere display lists for context "+gl);
		for (int i = 0; i < n; ++i) {
			SceneGraphComponent tcs = tessellatedCubeSphere(i, false);
			dlists[i] = gl.glGenLists(1);
			// LoggingSystem.getLogger(JOGLCylinderUtility.class).fine("Allocating new dlist "+dlists[i]);
			gl.glNewList(dlists[i], GL2.GL_COMPILE);
			IndexedFaceSet qms = (IndexedFaceSet) tcs.getChildComponent(0)
					.getGeometry();
			for (int j = 0; j < tcs.getChildComponentCount(); ++j) {
				gl.glPushMatrix();
				gl.glMultTransposeMatrixd(tcs.getChildComponent(j)
						.getTransformation().getMatrix(), 0);
				JOGLRendererHelper.drawFaces(jr, qms, true, 1.0);
				gl.glPopMatrix();
			}
			gl.glEndList();
		}
		if (!sharedDisplayLists)
			sphereDListsTable.put(jr.globalGL, dlists);
		else
			globalSharedSphereDisplayLists = dlists;
	}

	/**
	 * @param i
	 * @return
	 */
	public static int getSphereDLists(int i, JOGLRenderer jr) {
		int[] dlists = getSphereDLists(jr);
		if (dlists == null) {
			JOGLConfiguration.getLogger().log(Level.WARNING,
					"Invalid sphere display lists");
			return 0;
		}
		return dlists[dlists.length <= i ? dlists.length - 1 : i];
	}

	/**
	 * @param i
	 * @return
	 */
	public static int[] getSphereDLists(JOGLRenderer jr) {
		int dlists[];
		if (!sharedDisplayLists)
			dlists = (int[]) sphereDListsTable.get(jr.globalGL);
		else
			dlists = globalSharedSphereDisplayLists;
		if (dlists == null) {
			setupSphereDLists(jr);
			if (!sharedDisplayLists)
				dlists = (int[]) sphereDListsTable.get(jr.globalGL);
			else
				dlists = globalSharedSphereDisplayLists;
		}
		if (dlists == null) {
			throw new IllegalStateException(
					"Can't make sphere display lists successfully");
		}
		return dlists;
	}

	public static void disposeSphereDLists(JOGLRenderer jr) {
		if (!sharedDisplayLists)
			sphereDListsTable.remove(jr);

	}

	static double[] lodLevels = { .02, .08, .16, .32, .64 };

	public static int getResolutionLevel(double[] o2ndc, double lod) {
		double d = lod * CameraUtility.getNDCExtent(o2ndc);
		// JOGLConfiguration.theLog.log(Level.FINE,"Distance is "+d);
		int i = 0;
		for (i = 0; i < 5; ++i) {
			if (d < lodLevels[i])
				break;
		}
		return i;
	}

}
