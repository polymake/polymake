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


package de.jreality.examples;

import javax.media.opengl.GL;
import javax.media.opengl.GLAutoDrawable;

import de.jreality.jogl.AbstractCalculation;
import de.jreality.jogl.GpgpuUtility;
import de.jreality.jogl.IntegratorFactory;
import de.jreality.math.Matrix;
import de.jreality.shader.GlslProgram;

public class PathCurves extends AbstractCalculation {
  
  protected String initSource() {
    IntegratorFactory rk = IntegratorFactory.rk4();
    rk.addUniform("matrix", "mat4");
    rk.srcAll(
      "  vec4 ret = matrix*point;\n" +
      "  //ret.w = 0.;\n" +
      "  return ret;\n"
    );
    return rk.toString();
  }

  protected void setUniformValues(GL gl, GlslProgram prog) {
    super.setUniformValues(gl, prog);
    // some matrix in gl(3)
    double a=3, b=2, c=-.1;
    Matrix m = new Matrix(0,-a,-b,0, a,0,-c,0, b,c,0,0, 0,0,0,0);
    //Matrix m = MatrixBuilder.euclidean().rotate(Math.PI/2, 0,0,1).getMatrix();
    prog.setUniformMatrix("matrix", m.getArray());
    prog.setUniform("h", 0.005);
    prog.setUniform("r3", true);
  }
  
  protected void calculationFinished() {
//    if (numValues < 64) 
//      GpgpuUtility.dumpData(getCurrentValues());
//    else {
//      FloatBuffer fb = getCurrentValues();
//      fb.position(0).limit(3*4);
//      GpgpuUtility.dumpSelectedData(fb);
//    }
    triggerCalculation();
  }
  
  public static void main(String[] args) {
    PathCurves ev = new PathCurves();
    ev.setDisplayTexture(true);
    ev.setReadData(true);
    int sl = 128;
    float[] f = GpgpuUtility.makeGradient(sl, 254);
    ev.setValues(f);
    ev.triggerCalculation();
    GpgpuUtility.run(ev);
  }

public void dispose(GLAutoDrawable drawable) {
	// TODO Auto-generated method stub
	
}

}
