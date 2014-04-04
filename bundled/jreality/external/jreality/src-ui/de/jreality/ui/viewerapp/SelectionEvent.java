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


package de.jreality.ui.viewerapp;

import de.jreality.scene.Appearance;
import de.jreality.scene.Geometry;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.Transformation;
import de.jreality.scene.data.AttributeEntity;
import de.jreality.scene.tool.Tool;


/**
 * Event for selections in the scene graph.
 * 
 * @author msommer
 */
public class SelectionEvent extends java.util.EventObject {

	private final Selection selection;

	/**
	 * @param source the event's source
	 * @param selection the current Selection object
	 */
	public SelectionEvent(Object source, Selection selection) {
		super(source);
		this.selection = selection;
	}

	/** Get the current Selection object */
	public Selection getSelection() {
		return selection;
	}

	/** Returns true iff a {@link Tool} was selected */
	public boolean toolSelected() {
		return selection.isTool();
	}

	/** Returns true iff an {@link AttributeEntity} was selected */
	public boolean entitySelected() {
		return selection.isEntity();
	}

	/** Returns true iff the scene graph's root was selected */
	public boolean rootSelected() {
		return (selection.getLength()==1);  //&& componentSelected()
	}

	/** Returns true iff a {@link de.jreality.scene.SceneGraphNode} was selected */
	public boolean nodeSelected() {
		return selection.isNode();
	}

	/** Returns true iff a {@link SceneGraphComponent} was selected */
	public boolean componentSelected() {
		return selection.isComponent();
	}

	/** Returns true iff a {@link Geometry} was selected */
	public boolean geometrySelected() {
		return (nodeSelected() && 
				selection.getLastElement() instanceof Geometry);
	}

	/** Returns true iff an {@link Appearance} was selected */
	public boolean appearanceSelected() {
		return (nodeSelected() && 
				selection.getLastElement() instanceof Appearance);
	}

	/** Returns true iff a {@link Transformation} was selected */
	public boolean transformationSelected() {
		return (nodeSelected() && 
				selection.getLastElement() instanceof Transformation);
	}

}