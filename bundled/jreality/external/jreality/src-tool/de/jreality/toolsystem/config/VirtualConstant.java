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


package de.jreality.toolsystem.config;

import java.beans.DefaultPersistenceDelegate;
import java.beans.PersistenceDelegate;

import de.jreality.scene.data.DoubleArray;
import de.jreality.scene.tool.AxisState;
import de.jreality.scene.tool.InputSlot;


/**
 *
 * TODO: comment this
 *
 * @author weissman
 *
 */
public class VirtualConstant {
  
  public static final PersistenceDelegate DELEGATE = new DefaultPersistenceDelegate(new String[]{
    "slot", "value"
  });

  private InputSlot slot;
  private double[] trafo;
  private Double axis;
  private boolean isTrafo;

  public VirtualConstant(InputSlot slot, Object value) {
    this.slot=slot;
    if (value instanceof Double) {
      axis = (Double)value;
    } else {
      trafo = (double[]) value;
      if (trafo.length != 16) throw new IllegalArgumentException("no 4x4 matrix");
      isTrafo=true;
    }
  }
  
  public InputSlot getSlot() {
    return slot;
  }
  public boolean isTrafo() {
    return isTrafo;
  }
  public AxisState getAxisState() {
    if (isTrafo) return null;
    return new AxisState(axis.doubleValue());
  }
  public DoubleArray getTransformationMatrix() {
    if (!isTrafo) return null;
    return new DoubleArray(trafo);
  }
  public String toString() {
    return "VirtualConstant: "+slot+"->"+(isTrafo ? new DoubleArray(trafo).toString() : new AxisState(axis.doubleValue()).toString());
  }

}
