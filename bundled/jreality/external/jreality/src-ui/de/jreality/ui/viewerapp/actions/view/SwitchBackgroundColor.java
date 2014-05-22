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


package de.jreality.ui.viewerapp.actions.view;

import java.awt.Color;
import java.awt.event.ActionEvent;

import de.jreality.scene.Appearance;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.Viewer;
import de.jreality.shader.ShaderUtility;
import de.jreality.ui.viewerapp.ViewerApp;
import de.jreality.ui.viewerapp.actions.AbstractJrAction;


/**
 * Switches the background color of a scene graph's root, 
 * which is used as the background color of the displaying viewer.  
 * 
 * @author msommer
 */
public class SwitchBackgroundColor extends AbstractJrAction {

	/**
	 * @deprecated use {@link ViewerApp#defaultBackgroundColor} instead
	 */
	public static Color[] defaultColor = ViewerApp.defaultBackgroundColor;

	private Color[] colors;
	private SceneGraphComponent sceneRoot;


	/**
	 * Sets the scene root's background color.
	 * @param name name of the action
	 * @param sceneRoot the root of the scene graph
	 * @param colors list of colors with length = 1 or 4
	 */
	public SwitchBackgroundColor(String name, SceneGraphComponent sceneRoot, Color... colors) {
		super(name);

		if (colors == null || (colors.length!=1 && colors.length!=4)) 
			throw new IllegalArgumentException("illegal length of colors[]");
		if (sceneRoot == null) 
			throw new IllegalArgumentException("no scene root");

		this.colors = colors;
		this.sceneRoot = sceneRoot;

		setShortDescription("Set the viewer's background color");
	}

	/** @see SwitchBackgroundColor#SwitchBackgroundColor(String, SceneGraphComponent, Color[]) */
	public SwitchBackgroundColor(String name, ViewerApp viewerApp, Color... colors) {
		this(name, viewerApp.getViewerSwitch().getSceneRoot(), colors);
	}

	/** @see SwitchBackgroundColor#SwitchBackgroundColor(String, SceneGraphComponent, Color[]) */
	public SwitchBackgroundColor(String name, Viewer viewer, Color... colors) {
		this(name, viewer.getSceneRoot(), colors);
	}


	@Override
	public void actionPerformed(ActionEvent e) {
		Appearance app = sceneRoot.getAppearance();
		if (app == null) {
			app = new Appearance("root appearance");
			ShaderUtility.createRootAppearance(app);
			sceneRoot.setAppearance(app);
		}

		//trim colors[] if it contains the same 4 colors
		if (colors.length == 4) {
			boolean equal = true;
			for (int i = 1; i < colors.length; i++)
				if (colors[i] != colors[0]) equal = false;
			if (equal) colors = new Color[]{ colors[0] };
		}

		app.setAttribute("backgroundColor", (colors.length==1)? colors[0] : Appearance.INHERITED);
		app.setAttribute("backgroundColors", (colors.length==4)? colors : Appearance.INHERITED);
	}

}