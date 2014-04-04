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

import java.util.EventObject;

import de.jreality.scene.IndexedFaceSet;
import de.jreality.scene.data.Attribute;

public class FaceDragEvent extends EventObject {
    
    private static final long serialVersionUID = 19823L;

    private final int index;
    private final double[] translation;
    private final double[] position;
    private final IndexedFaceSet faceSet;
    private int[] faceIndices;
    private double[][] faceVertices;
  
    public FaceDragEvent(IndexedFaceSet faceSet, int index, double[] translation, double[] position) {
        super(faceSet);
    this.faceSet=faceSet;
    this.index=index;
    this.translation = (double[])translation.clone();
    this.position = (double[])position.clone();
    this.faceIndices = faceSet.getFaceAttributes(Attribute.INDICES).toIntArrayArray().getValueAt(index).toIntArray(null);
    this.faceVertices=new double[faceIndices.length][];
    for(int i=0;i<faceIndices.length;i++)
        faceVertices[i]=faceSet.getVertexAttributes(Attribute.COORDINATES).toDoubleArrayArray().getValueAt(faceIndices[i]).toDoubleArray(null);
    }
    
    /** The x-coordinate of this event's translation. */
    public double getX() {
        return translation[0];
    }
    
    /** The y-coordinate of this event's translation. */
    public double getY() {
        return translation[1];
    }
    
    /** The z-coordinate of this event's translation. */
    public double getZ() {
        return translation[2];
    }

  public double[] getTranslation() {
      return (double[]) translation.clone();
  }
  
  public double[] getPosition() {
      return (double[]) position.clone();
    }
  
  public int getIndex() {
      return index;
  }
  public int[] getFaceIndices() {
      return faceIndices;
  }  
  public double[][] getFaceVertices(){
      return faceVertices;
  }  
  public IndexedFaceSet getIndexedFaceSet() {
    return faceSet;
  }
}


