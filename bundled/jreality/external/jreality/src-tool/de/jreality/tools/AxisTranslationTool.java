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
import de.jreality.math.MatrixBuilder;
import de.jreality.math.Pn;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.Transformation;
import de.jreality.scene.tool.AbstractTool;
import de.jreality.scene.tool.InputSlot;
import de.jreality.scene.tool.ToolContext;
import de.jreality.shader.EffectiveAppearance;

public class AxisTranslationTool extends AbstractTool {

	private boolean moveChildren;
	private boolean snapToGrid = true;
	private double snapToGridInterval = 1;

	transient boolean directionDetermined;
	transient int direction;

	transient EffectiveAppearance eap;
	transient private int metric;
	transient Matrix startMatrix = new Matrix();
	transient double dx, dy, dz;
	transient Matrix local2world = new Matrix();
	transient Matrix dragFrame;
	transient Matrix pointer = new Matrix();
	transient protected SceneGraphComponent comp;


	transient private boolean dragInViewDirection;

	static InputSlot activationSlot = InputSlot.getDevice("DragActivation");
	static InputSlot alongPointerSlot = InputSlot.getDevice("DragAlongViewDirection");
	static InputSlot evolutionSlot = InputSlot.getDevice("PointerEvolution");

	public AxisTranslationTool() {
		super(activationSlot);
		addCurrentSlot(evolutionSlot);
		addCurrentSlot(alongPointerSlot);
	}

	public void activate(ToolContext tc) {
		comp = (moveChildren ? tc.getRootToLocal() : tc.getRootToToolComponent()).getLastComponent();
		if (comp.getTransformation() == null) comp.setTransformation(new Transformation());
		try {
			if (tc.getAxisState(alongPointerSlot).isPressed()) {
				dragInViewDirection = true;
			}
			else {
				dragInViewDirection = false;
			}
		} catch (Exception me) {
			// no drag in zaxis
			dragInViewDirection = false;
		}
		if (eap == null || !EffectiveAppearance.matches(eap, tc.getRootToToolComponent())) {
			eap = EffectiveAppearance.create(tc.getRootToToolComponent());
		}
		metric = eap.getAttribute("metric", Pn.EUCLIDEAN);
		comp.getTransformation().getMatrix(startMatrix.getArray());
		dx = 0;
		dy = 0;
		dz = 0;
		directionDetermined = false;
	}

	public void perform(ToolContext tc) {
		if (tc.getSource() == alongPointerSlot) {
			if (tc.getAxisState(alongPointerSlot).isPressed()) {
				dragInViewDirection = true;
			}
			else {
				dragInViewDirection = false;
			}
			return;
		}

		Matrix evolution = new Matrix(tc.getTransformationMatrix(evolutionSlot));
		// need to convert from euclidean to possibly non-euclidean translation
		if (metric != Pn.EUCLIDEAN)
			MatrixBuilder.init(null, metric).translate(evolution.getColumn(3)).assignTo(evolution);

		(moveChildren ? tc.getRootToLocal():tc.getRootToToolComponent()).getMatrix(local2world.getArray());



//		if (dragInViewDirection) {
//		tc.getTransformationMatrix(InputSlot.getDevice("CameraToWorld")).toDoubleArray(pointer.getArray());
//		// TODO non-euclideanize this (once you understand it!)
//		double dz = evolution.getEntry(0,3)+evolution.getEntry(1,3);
//		evolution.assignIdentity();
//		evolution.setColumn(3, Rn.times(null, dz, pointer.getColumn(2)));
//		evolution.setEntry(3,3,1);
//		}

		double[] t = evolution.getColumn(3);
		dx += t[0];
		dy += t[1];
		dz += t[2];
		double tx=dx, ty=dy, tz=dz;
		if (snapToGrid) {
			tx = snapToGridInterval * Math.round(dx/snapToGridInterval);
			ty = snapToGridInterval * Math.round(dy/snapToGridInterval);
			tz = snapToGridInterval * Math.round(dz/snapToGridInterval);
		}
		double ax = Math.abs(tx);
		double ay = Math.abs(ty);
		double az = Math.abs(tz);
		if (ax>0 || ay >0 || az>0) {
			if (!dragInViewDirection ) {
				if (!directionDetermined) {
					direction = 0;
					if (ay > ax) direction = 1;
					if (az > ay) direction = 2;
					directionDetermined = true;
				}
				if (direction != 0) tx = 0;
				if (direction != 1) ty = 0;
				if (direction != 2) tz = 0;
			}
			Matrix result = new Matrix(startMatrix);
			result.multiplyOnRight(local2world.getInverse());
			result.multiplyOnRight(MatrixBuilder.euclidean().translate(tx, ty, tz).getMatrix());
			result.multiplyOnRight(local2world);
			comp.getTransformation().setMatrix(result.getArray());
		}
	}

	public boolean getMoveChildren() {
		return moveChildren;
	}
	public void setMoveChildren(boolean moveChildren) {
		this.moveChildren = moveChildren;
	}

	public boolean isSnapToGrid() {
		return snapToGrid;
	}

	public void setSnapToGrid(boolean snapToGrid) {
		this.snapToGrid = snapToGrid;
	}

	public double getSnapToGridInterval() {
		return snapToGridInterval;
	}

	public void setSnapToGridInterval(double snapToGridInterval) {
		this.snapToGridInterval = snapToGridInterval;
	}
}
