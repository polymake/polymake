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
import de.jreality.scene.tool.AbstractTool;
import de.jreality.scene.tool.InputSlot;
import de.jreality.scene.tool.ToolContext;
import de.jreality.shader.EffectiveAppearance;

/**
 * @author weissman
 *  
 */
public class LookAtTool extends AbstractTool {

  private InputSlot myActivation = InputSlot.getDevice("ShipRotateActivation");
  private final InputSlot verticalRotation = InputSlot.getDevice("VerticalHeadRotationAngleEvolution");
  private final InputSlot horizontalRotation = InputSlot.getDevice("HorizontalShipRotationAngleEvolution");

  private double currentAngleH;
  private double currentAngleV;

  private double minHorizontalAngle=-Double.MAX_VALUE;
  private double maxHorizontalAngle=Double.MAX_VALUE;
  private double minVerticalAngle=-0.2;
  private double maxVerticalAngle=-Math.PI/2-0.2;
  
  private double rodLength=5;
  
  private boolean rotate;
  
  public LookAtTool() {
    addCurrentSlot(myActivation);
    currentAngleV=(maxVerticalAngle+minVerticalAngle)/2;
  }

  public void perform(ToolContext tc) {
    if (rotate) {
      if (!tc.getAxisState(myActivation).isPressed()) {
        removeCurrentSlot(verticalRotation);
        removeCurrentSlot(horizontalRotation);
        rotate = false;
      }
    } else {
      if (tc.getAxisState(myActivation).isPressed()) {
        addCurrentSlot(verticalRotation);
        addCurrentSlot(horizontalRotation);
        rotate = true;
      }
    }
    if (tc.getSource() == myActivation) return;

    double dAngle = tc.getAxisState(verticalRotation).doubleValue();
    double hAngle = tc.getAxisState(horizontalRotation).doubleValue();

    if (currentAngleH-hAngle<=maxHorizontalAngle && currentAngleH-hAngle>=minHorizontalAngle)
      currentAngleH-=hAngle;
    if (currentAngleV+dAngle<=Math.max(minVerticalAngle, maxVerticalAngle) && currentAngleV+dAngle>=Math.min(minVerticalAngle, maxVerticalAngle)) {
      currentAngleV+=dAngle;
    }
    EffectiveAppearance eap = EffectiveAppearance.create(tc.getRootToToolComponent());
    int sig = eap.getAttribute("metric", Pn.EUCLIDEAN);
    
    Matrix m = MatrixBuilder.init(null, sig).rotateY(currentAngleH).rotateX(currentAngleV).translate(0,0,rodLength).getMatrix();

    m.assignTo(tc.getRootToLocal().getLastComponent());
  }

  public double getMaxHorizontalAngle() {
    return maxHorizontalAngle;
  }

  public void setMaxHorizontalAngle(double maxHorizontalAngle) {
    this.maxHorizontalAngle = maxHorizontalAngle;
  }

  public double getMaxVerticalAngle() {
    return maxVerticalAngle;
  }

  public void setMaxVerticalAngle(double maxVerticalAngle) {
    this.maxVerticalAngle = maxVerticalAngle;
  }

  public double getMinHorizontalAngle() {
    return minHorizontalAngle;
  }

  public void setMinHorizontalAngle(double minHorizontalAngle) {
    this.minHorizontalAngle = minHorizontalAngle;
  }

  public double getMinVerticalAngle() {
    return minVerticalAngle;
  }

  public void setMinVerticalAngle(double minVerticalAngle) {
    this.minVerticalAngle = minVerticalAngle;
  }

  public double getRodLength() {
    return rodLength;
  }

  public void setRodLength(double rodLength) {
    this.rodLength = rodLength;
  }

}