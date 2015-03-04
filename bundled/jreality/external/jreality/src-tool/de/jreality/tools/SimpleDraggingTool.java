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
import de.jreality.math.P3;
import de.jreality.math.Pn;
import de.jreality.math.Rn;
import de.jreality.scene.SceneGraphComponent;
import de.jreality.scene.Transformation;
import de.jreality.scene.tool.AbstractTool;
import de.jreality.scene.tool.InputSlot;
import de.jreality.scene.tool.ToolContext;
import de.jreality.shader.EffectiveAppearance;


/**
 *
 * TODO: document this
 *
 * @author brinkman
 *
 */
public class SimpleDraggingTool extends AbstractTool {

    private boolean moveChildren;
    
    static InputSlot activationSlot = InputSlot.getDevice("SimpleDragActivation");
    static InputSlot evolutionSlot = InputSlot.getDevice("SimpleTranslateTransformation");
    
    public SimpleDraggingTool(InputSlot ...activationSlots )	{
    	super(activationSlots);
    }
    
    public SimpleDraggingTool() {
        this(activationSlot);
        addCurrentSlot(evolutionSlot);
    }
    
    transient protected SceneGraphComponent comp;
    
    public void activate(ToolContext tc) {
      comp = (moveChildren ? tc.getRootToLocal() : tc.getRootToToolComponent()).getLastComponent();
      if (comp.getTransformation() == null) comp.setTransformation(new Transformation());
      if (eap == null || !EffectiveAppearance.matches(eap, tc.getRootToToolComponent())) {
          eap = EffectiveAppearance.create(tc.getRootToToolComponent());
        }
        metric = eap.getAttribute("metric", Pn.EUCLIDEAN);
    }

    transient EffectiveAppearance eap;
    transient protected int metric;
    transient protected Matrix result = new Matrix();
    transient Matrix local2world = new Matrix();
    transient Matrix dragFrame;
    transient Matrix pointer = new Matrix();
    
    public void perform(ToolContext tc) {
      Matrix evolution = new Matrix(tc.getTransformationMatrix(evolutionSlot));
      // need to convert from euclidean to possibly non-euclidean translation
	  if (metric != Pn.EUCLIDEAN)
		  MatrixBuilder.init(null, metric).translate(evolution.getColumn(3)).assignTo(evolution);
    
      (moveChildren ? tc.getRootToLocal():tc.getRootToToolComponent()).getMatrix(local2world.getArray());
      if (Rn.isNan(local2world.getArray())) return;
      
      comp.getTransformation().getMatrix(result.getArray());
      
     evolution.conjugateBy(local2world.getInverse());
     if (metric != Pn.EUCLIDEAN) P3.orthonormalizeMatrix(evolution.getArray(), evolution.getArray(), 10E-8, metric);
     result.multiplyOnRight(evolution);
     if (Rn.isNan(result.getArray())) return;
      comp.getTransformation().setMatrix(result.getArray());
    }

    public boolean getMoveChildren() {
      return moveChildren;
    }
    public void setMoveChildren(boolean moveChildren) {
      this.moveChildren = moveChildren;
    }

}
