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


package de.jreality.sunflow;

import java.awt.Component;
import java.awt.Dimension;

import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.SceneGraphPath;
import de.jreality.scene.Viewer;

/**
 * 
 * @version 1.0
 * @author <a href="mailto:pinkall@math.tu-berlin.de">Ulrich Pinkall</a>
 *
 */
public class SunflowViewer implements Viewer {
	private SceneGraphPath cameraPath;
	private SceneGraphComponent sceneRoot;
	private CancelableImagePanel display = new CancelableImagePanel();
	private int width;
	private int height;
	private RenderOptions options;

	public CancelableImagePanel getViewingComponent() {
		return display;
	}

	public void setSceneRoot(SceneGraphComponent c) {
		sceneRoot =c;
	}

	public SceneGraphComponent getSceneRoot() {
		return sceneRoot;
	}
	
	public void render() {
		SunflowRenderer sv =new SunflowRenderer();
		sv.setOptions(options);
		sv.render(sceneRoot,cameraPath,display,width,height);
	}

	public SceneGraphPath getCameraPath() {
		return cameraPath;
	}

	public void setCameraPath(SceneGraphPath p) {
		cameraPath = p;        
	}

	public boolean hasViewingComponent() {
		return false;
	}

	public void initializeFrom(Viewer v) {
		cameraPath = v.getCameraPath();
		sceneRoot = v.getSceneRoot();
		if (v.hasViewingComponent()){
			Component c = (Component)v.getViewingComponent();
			setWidth(c.getWidth());
			setHeight(c.getHeight());
		}
		display.setPreferredSize(new Dimension(width,height));
	}

	public int getMetric() {
		return 0;
	}

	public void setMetric(int sig) {
	}

	public void setAuxiliaryRoot(SceneGraphComponent ar) {
		throw new UnsupportedOperationException("not implemented");
	}

	public SceneGraphComponent getAuxiliaryRoot() {
		throw new UnsupportedOperationException("not implemented");
	}

	public int getHeight() {
		return width;
	}
	
	public void setHeight(int h) {
		height = h;
		display.setPreferredSize(new Dimension(width,height));
	}

	public int getWidth() {
		return height;
	}
	
	public void setWidth(int w) {
		width = w;
		display.setPreferredSize(new Dimension(width,height));
	}

	public Dimension getViewingComponentSize() {
		return getViewingComponent().getSize();
	}

	public boolean canRenderAsync() {
		return false;
	}

	public void renderAsync() {
		throw new UnsupportedOperationException();
	}

	public RenderOptions getOptions() {
		return options;
	}

	public void setOptions(RenderOptions options) {
		this.options = options;
	}
	
	public void cancel() {
		display.cancel();
	}
}
