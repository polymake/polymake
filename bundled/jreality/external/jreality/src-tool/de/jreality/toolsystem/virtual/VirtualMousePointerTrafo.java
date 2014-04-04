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


package de.jreality.toolsystem.virtual;

import de.jreality.math.Rn;
import de.jreality.toolsystem.ToolEvent;
import de.jreality.toolsystem.VirtualDeviceContext;


/**
 *
 * TODO: comment this
 *
 * @author brinkman
 *
 */
public class VirtualMousePointerTrafo extends VirtualRawMousePointerTrafo {

   public ToolEvent process(VirtualDeviceContext context) {
	   ToolEvent te = super.process(context);
	   // this is the normalization code that VirtualRawMousePointerTrafo lacks
     for(int i=0; i<4; i++)
    	if (Math.abs(pointerTrafo[12+i])>Rn.TOLERANCE)
    		scaleColumn(pointerTrafo, i, 1/pointerTrafo[i+12]);
    for(int i = 0; i<3; i++)
    	columnTrafo(pointerTrafo, i, 3, -1);
    
    double nrm = columnNorm(pointerTrafo, 2);
	if (nrm>Rn.TOLERANCE)
	    scaleColumn(pointerTrafo, 2, -1/nrm);
    for(int i = 1; i>=0; i--) {
    	columnTrafo(pointerTrafo, i, i+1, -scalarColumnProduct(pointerTrafo, i, i+1));
 
    }
    columnTrafo(pointerTrafo, 1, 2, -scalarColumnProduct(pointerTrafo, 1, 2));
   	nrm = columnNorm(pointerTrafo, 1);
	if (nrm>Rn.TOLERANCE)
	    scaleColumn(pointerTrafo, 1, 1/nrm);
	columnTrafo(pointerTrafo, 0, 2, -scalarColumnProduct(pointerTrafo, 0, 2));
	columnTrafo(pointerTrafo, 0, 1, -scalarColumnProduct(pointerTrafo, 0, 1));
	nrm = columnNorm(pointerTrafo, 0);
	if (nrm>Rn.TOLERANCE)
	    scaleColumn(pointerTrafo, 0, 1/nrm);
	
  pointerTrafo[12]=pointerTrafo[13]=pointerTrafo[14]=0;
  pointerTrafo[15]=1;

	return te;
  }

  public String getName() {
    return "MousePointerTrafo";
  }

  private void scaleColumn(double[] matrix, int col, double factor) {
  	matrix[col]*=factor;
  	matrix[col+4]*=factor;
  	matrix[col+8]*=factor;
  	matrix[col+12]*=factor;
  }
  
  private void columnTrafo(double[] matrix, int i, int j, double factor) {
  	matrix[i]+=matrix[j]*factor;
  	matrix[i+4]+=matrix[j+4]*factor;
  	matrix[i+8]+=matrix[j+8]*factor;
  	matrix[i+12]+=matrix[j+12]*factor;
  }
  
  private double scalarColumnProduct(double[] matrix, int i, int j) {
  	return matrix[i]*matrix[j]+
		   matrix[i+4]*matrix[j+4]+
		   matrix[i+8]*matrix[j+8]+
		   matrix[i+12]*matrix[j+12]; 
  }
  
  private double columnNorm(double[] matrix, int i) {
  	return Math.sqrt(scalarColumnProduct(matrix, i, i));
  }
}
