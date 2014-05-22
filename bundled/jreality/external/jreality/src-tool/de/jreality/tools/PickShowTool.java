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

import java.awt.Color;

import de.jreality.geometry.Primitives;
import de.jreality.math.Matrix;
import de.jreality.math.MatrixBuilder;
import de.jreality.math.Pn;
import de.jreality.math.Rn;
import de.jreality.scene.Appearance;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.pick.PickResult;
import de.jreality.scene.tool.AbstractTool;
import de.jreality.scene.tool.InputSlot;
import de.jreality.scene.tool.ToolContext;
import de.jreality.shader.CommonAttributes;

public class PickShowTool extends AbstractTool {

  private transient SceneGraphComponent c = new SceneGraphComponent();
  private transient Appearance a = new Appearance();
  
  private transient boolean attached;
  private double radius=0.005;
  
  public PickShowTool(String activationAxis, double radius) {
	    super(activationAxis == null ? null : InputSlot.getDevice(activationAxis));
	    this.radius=radius;
	    init();
	  }

private void init() {
	addCurrentSlot(InputSlot.getDevice("PointerTransformation"));
	c.addChild(Primitives.sphere(1, 0, 0, 0));
	c.setName("pick display");
	c.setAppearance(a);
	c.setPickable( false);
	a.setAttribute(CommonAttributes.FACE_DRAW, true);
	a.setAttribute(CommonAttributes.PICKABLE, false);
}

  public PickShowTool() {
    super();
    init();
  }

  public void activate(ToolContext tc) {
    perform(tc);
  }
  
  public void perform(ToolContext tc) {
    PickResult pr = tc.getCurrentPick();
    if (pr == null) {
      assureDetached(tc);
      return;
    }
    assureAttached(tc);
    switch (pr.getPickType()) {
    case PickResult.PICK_TYPE_FACE:
      c.getAppearance().setAttribute("diffuseColor", Color.yellow);
      break;
    case PickResult.PICK_TYPE_LINE:
      c.getAppearance().setAttribute("diffuseColor", Color.green);
      break;
    case PickResult.PICK_TYPE_POINT:
      c.getAppearance().setAttribute("diffuseColor", Color.magenta);
      break;
    case PickResult.PICK_TYPE_OBJECT:
      c.getAppearance().setAttribute("diffuseColor", Color.red);
      break;
    default:
      c.getAppearance().setAttribute("diffuseColor", Color.black);
    }
    // TODO non-euclideanize this
    double[] worldCoordinates = pr.getWorldCoordinates();
    if (!Pn.isValidCoordinate(worldCoordinates, 3, Pn.EUCLIDEAN)) return;
    double[] cp = new Matrix(tc.getViewer().getCameraPath().getMatrix(null)).getColumn(3);
    double scale=Rn.euclideanDistance(cp, worldCoordinates);
    MatrixBuilder.euclidean().translate(worldCoordinates).assignTo(c);
    MatrixBuilder.euclidean().scale(scale*radius).assignTo(c.getChildComponent(0));
   }

  public void deactivate(ToolContext tc) {
    assureDetached(tc);
  }
  
  private void assureAttached(ToolContext tc) {
    if (!attached) tc.getViewer().getSceneRoot().addChild(c);
    attached = true;
    c.setVisible(true);
  }
  private void assureDetached(ToolContext tc) {
    c.setVisible(false);
  }

public double getRadius() {
	return radius;
}

public void setRadius(double radius) {
	this.radius = radius;
}

}
