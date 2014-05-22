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

import de.jreality.math.FactoredMatrix;
import de.jreality.math.Matrix;
import de.jreality.math.MatrixBuilder;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.tool.AbstractTool;
import de.jreality.scene.tool.InputSlot;
import de.jreality.scene.tool.ToolContext;

/**
 * @author weissman
 *  
 */
public class HeadTransformationTool extends AbstractTool {

	private transient InputSlot rotateActivation = InputSlot.getDevice("ShipRotateActivation");
	private transient final InputSlot verticalRotation = InputSlot.getDevice("VerticalHeadRotationAngleEvolution");

	private double maxAngle = Math.PI*0.35;
	private double minAngle = -Math.PI*0.35;

	private boolean invert;

	private transient double[] headTranslation;

	private transient double currentAngle;
	private transient boolean rotate;

	private final transient Matrix m=new Matrix();

	public HeadTransformationTool() {
		addCurrentSlot(verticalRotation);
		addCurrentSlot(rotateActivation);
	}

	public void perform(ToolContext tc) {
		if (rotate) {
			if (!tc.getAxisState(rotateActivation).isPressed()) {
				removeCurrentSlot(verticalRotation);
				rotate = false;
			}
		} else {
			if (tc.getAxisState(rotateActivation).isPressed()) {
				addCurrentSlot(verticalRotation);
				rotate = true;
			}

			if (tc.getSource() == rotateActivation || !rotate) return;
		}	
		double deltaAngle = tc.getAxisState(verticalRotation).doubleValue();
		if (tc.getRootToToolComponent().getLastComponent().getTransformation() != null) {
			tc.getRootToToolComponent().getLastComponent().getTransformation().getMatrix(m.getArray());
			if (headTranslation == null) {
				FactoredMatrix fm = new FactoredMatrix(m.getArray());
				currentAngle=fm.getRotationAngle();
				if (currentAngle > Math.PI) currentAngle-=2*Math.PI;
			}
			headTranslation=m.getColumn(3);
		} else {
			headTranslation=new double[]{0,1.7,0};
		}
		double dAngle = (invert ? -1 : 1) * deltaAngle;
		//System.out.println(NumberFormat.getNumberInstance().format(dAngle));
		if (currentAngle + dAngle > maxAngle || currentAngle + dAngle < minAngle) {
			return;
		}
		currentAngle+=dAngle;
		SceneGraphComponent myComponent = tc.getRootToToolComponent().getLastComponent();
		MatrixBuilder.euclidean().translate(headTranslation).rotateX(currentAngle).assignTo(myComponent);
	}

	public double getMaxAngle() {
		return maxAngle;
	}

	public void setMaxAngle(double maxAngle) {
		this.maxAngle = maxAngle;
	}

	public double getMinAngle() {
		return minAngle;
	}

	public void setMinAngle(double minAngle) {
		this.minAngle = minAngle;
	}

	public boolean isInvert() {
		return invert;
	}

	public void setInvert(boolean invert) {
		this.invert = invert;
	}
	public void currentAngle(double ang){
		currentAngle=ang;
	}
}