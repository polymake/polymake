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


package de.jreality.tools;

import de.jreality.math.Matrix;
import de.jreality.scene.Transformation;
import de.jreality.scene.tool.ToolContext;

/**
 * For testing Schönebeck data.
 * Only drags in local x and y direction.
 * 
 * @author msommer
 */
public class GFZDraggingTool extends DraggingTool {

	private boolean moveChildren;
	transient private boolean dragInViewDirection;


	@Override
	public void activate(ToolContext tc) {
		comp = (moveChildren ? tc.getRootToLocal() : tc.getRootToToolComponent()).getLastComponent();
		if (comp.getTransformation() == null) comp.setTransformation(new Transformation());

		try {
			if (tc.getAxisState(alongPointerSlot).isPressed())
				dragInViewDirection = true;
			else dragInViewDirection = false;
		} catch (Exception me) {
			dragInViewDirection = false;
		}
	}


	@Override
	public void perform(ToolContext tc) {

		if (tc.getSource() == alongPointerSlot) {
			if (tc.getAxisState(alongPointerSlot).isPressed())
				dragInViewDirection = true;
			else dragInViewDirection = false;
			return;
		}

		Matrix evolution = new Matrix(tc.getTransformationMatrix(evolutionSlot));
		comp.getTransformation().getMatrix(result.getArray());

		final double d = evolution.getEntry(0,3);
		evolution.assignIdentity();
		evolution.setEntry(dragInViewDirection? 1 : 0, 3, d);

		result.multiplyOnRight(evolution);
		comp.getTransformation().setMatrix(result.getArray());
	}
}