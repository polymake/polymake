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

//


//
package de.jreality.openhaptics;

import java.util.ArrayList;
import java.util.List;
import java.util.logging.Level;

import javax.media.opengl.GLAutoDrawable;

import de.jreality.jogl.JOGLConfiguration;
import de.jreality.jogl.JOGLViewer;


public class OHViewer extends Viewer {
	List<OHRawDevice> rawDevices = new ArrayList<OHRawDevice>();
	private double[] p0 = { -5, -5, -5};
	private double[] p1 = { 5, 5, 5};
	
	public double[] getP0() {
		return p0;
	}

	public double[] getP1() {
		return p1;
	}

	public void setBox(double[] p0, double[] p1) {
		if(p0!= null) this.p0 = p0;
		if(p1!= null) this.p1 = p1;
	}
	
	public List<OHRawDevice> getRawDevices() {
		return rawDevices;
	}
	
	
	@Override
	public void init(GLAutoDrawable arg0) {
		JOGLConfiguration.theLog.log(Level.INFO,"OpenHaptics Context initialization, creating new renderer");
		renderer = new OHRenderer(this);
		renderer.init(arg0);
		renderer.setStereoType(stereoType);
	}
	
	public OHRenderer getRenderer () {
		return (OHRenderer)renderer;
	}
}
	