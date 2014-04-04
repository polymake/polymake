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

import de.jreality.geometry.Primitives;
import de.jreality.scene.IndexedFaceSet;
import de.jreality.util.CameraUtility;
import de.jreality.util.LoggingSystem;

/**
 * @author gunn
 * 
 */
public class JOGLCylinderUtility {

	private JOGLCylinderUtility() {
	}

	private static int numCyls = 6;
	private static IndexedFaceSet[] cylinderList = new IndexedFaceSet[numCyls];

	private static IndexedFaceSet getCylinder(int i) {
		if (cylinderList[i] == null)
			cylinderList[i] = Primitives.cylinder(2 ^ i);
		return cylinderList[i];
	}

	static boolean sharedDisplayLists = JOGLConfiguration.sharedContexts;
	static WeakHashMap<GL, int[]> cylinderDListsTable = new WeakHashMap<GL, int[]>();
	static int[] globalSharedCylinderDisplayLists = null;

	// TODO This can't be static; the display lists so created are invalid if
	// the renderer parameter
	// no longer exists. So ... these display lists have to be tied to a
	// specific context.
	public static void setupCylinderDLists(JOGLRenderer jr) {
		int[] dlists = null; // getCylinderDLists(jr);
		// if (dlists != null) {
		// JOGLConfiguration.theLog.log(Level.WARNING,"Already have cylinder display lists for this renderer "+jr);
		// }
		GL2 gl = jr.globalGL;
		int n = 6;
		dlists = null;
		// if (!sharedDisplayLists) dlists = (int[] )
		// cylinderDListsTable.get(gl);
		// else
		dlists = new int[n];
		JOGLConfiguration.theLog.log(Level.INFO,
				"Setting up cylinder display lists for context " + gl);
		int nv = 4;
		for (int i = 0; i < n; ++i) {
			dlists[i] = gl.glGenLists(1);
			LoggingSystem.getLogger(JOGLCylinderUtility.class).fine(
					"Allocating new dlist " + dlists[i]);
			gl.glNewList(dlists[i], GL2.GL_COMPILE);
			// gl.glDisable(GL.GL_SMOOTH);
			IndexedFaceSet cyl = Primitives.cylinder(nv);
			nv *= 2;
			JOGLRendererHelper.drawFaces(jr, cyl, true, 1.0);
			gl.glEndList();
		}
		if (!sharedDisplayLists)
			cylinderDListsTable.put(jr.globalGL, dlists);
		else
			globalSharedCylinderDisplayLists = dlists;
	}

	/**
	 * @param i
	 * @return
	 */
	public static int getCylinderDLists(int i, JOGLRenderer jr) {
		int[] dlists = getCylinderDLists(jr);
		if (dlists == null) {
			JOGLConfiguration.getLogger().log(Level.WARNING,
					"Invalid cylinder display lists");
			return 0;
		}
		return dlists[i];
	}

	/**
	 * @param i
	 * @return
	 */
	public static int[] getCylinderDLists(JOGLRenderer jr) {
		int dlists[];
		if (!sharedDisplayLists)
			dlists = (int[]) cylinderDListsTable.get(jr.globalGL);
		else
			dlists = globalSharedCylinderDisplayLists;
		if (dlists == null) {
			setupCylinderDLists(jr);
			if (!sharedDisplayLists)
				dlists = (int[]) cylinderDListsTable.get(jr.globalGL);
			else
				dlists = globalSharedCylinderDisplayLists;
		}
		if (dlists == null) {
			throw new IllegalStateException(
					"Can't make cylinder display lists successfully");
		}
		return dlists;
	}

	public static void disposeCylinderDLists(JOGLRenderer jr) {
		if (!sharedDisplayLists)
			cylinderDListsTable.remove(jr);
	}

	static double[] lodLevels = { .02, .08, .16, .32, .64 };

	public static int getResolutionLevel(double[] o2ndc, double lod) {
		double d = lod * CameraUtility.getNDCExtent(o2ndc);
		int i = 0;
		for (i = 0; i < 5; ++i) {
			if (d < lodLevels[i])
				break;
		}
		return i;
	}

}
