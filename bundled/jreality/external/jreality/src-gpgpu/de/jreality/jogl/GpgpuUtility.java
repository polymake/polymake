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

import java.awt.BorderLayout;
import java.nio.FloatBuffer;
import java.nio.IntBuffer;
import java.util.Random;

import javax.media.opengl.GL;
import javax.media.opengl.GL2;
import javax.media.opengl.GLCapabilities;
import javax.media.opengl.GLEventListener;
import javax.media.opengl.GLProfile;
import javax.media.opengl.awt.GLCanvas;
import javax.swing.JFrame;

import com.jogamp.opengl.util.Animator;

import de.jreality.math.Rn;


public class GpgpuUtility {
  private GpgpuUtility() {}
  
  static int texSize(int i) {
    double fl = Math.floor(Math.sqrt(i));
    return (int) ((fl*fl < i) ? fl+1 : fl);
  }

  public static void dumpData(float[] data) {
    for (int i = 0; i < data.length; i++)
      System.out.print(data[i] + ", ");
    System.out.println();
  }

  public static void dumpData(IntBuffer data) {
    data.clear();
  for (int i = 0; i < data.capacity(); i++)
    System.out.print(data.get(i) + ", ");
  System.out.println();
}

  public static void dumpData(FloatBuffer data) {
    data.clear();
  for (int i = 0; i < data.capacity(); i++)
    System.out.print(data.get(i) + ", ");
  System.out.println();
}

  public static void dumpSelectedData(FloatBuffer data) {
    while (data.hasRemaining())
      System.out.print(data.get() + ", ");
    System.out.println();
  }
  
  static void checkBuf(GL2 gl) {
    int status = gl.glCheckFramebufferStatus(GL.GL_FRAMEBUFFER);
    String ret;
    switch (status) {
    case GL2.GL_FRAMEBUFFER_COMPLETE:
    	return;
    // unfortunately the name of the constant changed between the latest jogl version...
    case GL.GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
      ret = "FrameBuffer incomplete attachments";
    case GL.GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
      ret = "FrameBuffer incomplete missing attachment";
    //Ben has commented this, as GL2 in jogamp appears to miss this variable
    //idea for fix: find numerical value of this variable and replace
    //case GL2.GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT:
    //  ret = "FrameBuffer incomplete duplicate";
    case GL2.GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
      ret = "FrameBuffer incomplete dimensions";
    case GL2.GL_FRAMEBUFFER_INCOMPLETE_FORMATS:
      ret = "FrameBuffer incomplete formats";
    case GL2.GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
      ret = "FrameBuffer incomplete draw buffer";
    case GL2.GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
      ret = "FrameBuffer incomplete read buffer";
    case GL2.GL_FRAMEBUFFER_UNSUPPORTED:
      ret = "FrameBuffer unsupported";
    default:
      ret = "FrameBuffer unrecognized error";
    }
    throw new IllegalStateException(ret);
  }

  public static void run(GLEventListener listener) {
    JFrame f=new JFrame("gpgpu runner");
    f.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
    // get a GLCanvas
    GLCapabilities capabilities = new GLCapabilities(GLProfile.get("GL2"));

    GLCanvas canvas =new GLCanvas(capabilities);	
          //GLDrawableFactory.getFactory().createGLCanvas(capabilities);
    
    Animator animator = new Animator(canvas);
    canvas.setSize(256, 256);
      // add a GLEventListener, which will get called when the
      // canvas is resized or needs a repaint
      canvas.addGLEventListener(listener);
      // now add the canvas to the Frame.  Note we use BorderLayout.CENTER
      // to make the canvas stretch to fill the container (ie, the frame)
      f.getContentPane().add(canvas, BorderLayout.CENTER);
      f.pack();
      f.show();
      animator.start();
  }
  
  /**
   * swaps the R and B values
   * @param fb
   */
  static void atiHack(FloatBuffer fb) {
    for (int i = 0, n = fb.remaining()/4; i < n; i++) {
      float tmp = fb.get(4*i);
      fb.put(4*i, fb.get(4*i+2));
      fb.put(4*i+2, tmp);
    }
  }

  public static float[] makeGradient(int sl) {
    float[] f = new float[sl*sl*4];
    for (int i = 0; i < sl; i++) {
      for (int j = 0; j < sl; j++) {
        f[4*(sl*i+j)+0]=((float)i)/sl;
        f[4*(sl*i+j)+1]=((float)j)/sl;
        f[4*(sl*i+j)+2]=0;
        f[4*(sl*i+j)+3]=1;
      }
    }
    return f;
  }

  public static float[] makeGradient(int sl, int dismissCnt) {
    float[] f = new float[(sl*sl-dismissCnt)*4];
    int remaining = sl*sl;
    for (int i = 0; i < sl; i++) {
      for (int j = 0; j < sl; j++) {
        remaining--;
        if (remaining<dismissCnt) return f;
        f[4*(sl*i+j)+0]=((float)i)/sl;
        f[4*(sl*i+j)+1]=((float)j)/sl;
        f[4*(sl*i+j)+2]=0;
        f[4*(sl*i+j)+3]=1;
      }
    }
    return f;
  }

  public static float[] makeSphere(int numPoints, double[] origin, double radius1, double radius2) {
    float[] points = new float[numPoints*4];
    Random r = new Random(System.currentTimeMillis());
    double[] tmp = new double[3];
    for (int i = 0; i < numPoints; i++) {
      Rn.setToValue(tmp, -.5+r.nextDouble(), -.5+r.nextDouble(), -.5+r.nextDouble());
      Rn.normalize(tmp, tmp);
      Rn.times(tmp, radius1+r.nextDouble()*(radius2-radius1), tmp);
      if (origin != null) Rn.add(tmp, origin, tmp);
      points[4*i+0]=(float) tmp[0];
      points[4*i+1]=(float) tmp[1];
      points[4*i+2]=(float) tmp[2];
      points[4*i+3]=1;
    }
    return points;
  }

}
