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


package de.jreality.jogl;

import javax.media.opengl.GL2;
import javax.media.opengl.GLAutoDrawable;


public class GpgpuViewer extends JOGLViewer {

  AbstractCalculation calculation;
  private boolean calculationInited;
  
  public void setCalculation(AbstractCalculation calculation) {
    this.calculation = calculation;
    calculationInited=false;
  }
  
  public void init(GLAutoDrawable arg0) {
    if (calculation != null) {
      calculationInited=true;
      calculation.init(arg0);
    }
    super.init(arg0);
  }
  
  public void display(GLAutoDrawable arg0) {
    if (calculation != null) {
      if (!calculationInited) {
        calculationInited=true;
        calculation.init(arg0);
      }
      arg0.getGL().getGL2().glPushAttrib(GL2.GL_ALL_ATTRIB_BITS);
      calculation.display(arg0);
      arg0.getGL().getGL2().glPopAttrib();
    }
    arg0.getGL().getGL2().glPushAttrib(GL2.GL_ALL_ATTRIB_BITS);
    super.renderer.lightsChanged=true;
    super.display(arg0);
    arg0.getGL().getGL2().glPopAttrib();
  }
  
  public void reshape(GLAutoDrawable arg0, int arg1, int arg2, int arg3, int arg4) {
    if (calculation != null) calculation.reshape(arg0, arg1, arg2, arg3, arg4);
    super.reshape(arg0, arg1, arg2, arg3, arg4);
  }
  
  public AbstractCalculation getCalculation() {
    return calculation;
  }

}
