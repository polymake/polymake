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

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

import javax.media.opengl.GL;
import javax.media.opengl.GL2;
import javax.media.opengl.GLAutoDrawable;
import javax.media.opengl.glu.GLU;

import de.jreality.jogl.shader.GlslLoader;

public class ClothCalculation extends AbstractCalculation {
  
  private static int NUM_ROWS;
  private static int NUM_COLS;
  
  private FloatBuffer positions;
  private int dataTextureSize;
  
  private int[] texIDsPositions;
  private int[] texIDsVelocities;
  
  private boolean hasData;
  private boolean dataChanged;
  
  private int pingPong;
  private int pongPing=1;

  private double[] gravity=new double[]{0,0,-1};
  private double damping=0.01;
  private double factor=1;
  
  public ClothCalculation(int rows, int columns) {
      NUM_ROWS = rows;
      NUM_COLS = columns*columns;
      dataTextureSize=columns;
      positions=ByteBuffer.allocateDirect(NUM_COLS*4*4).order(ByteOrder.nativeOrder()).asFloatBuffer();
    valueBuffer = ByteBuffer.allocateDirect(NUM_COLS*NUM_ROWS*4*4).order(ByteOrder.nativeOrder()).asFloatBuffer();
    texIDsPositions = new int[NUM_ROWS*2];
    texIDsVelocities = new int[NUM_ROWS*2];
  }
  
  protected String initSource() {
    return "" +
    "uniform bool point; \n" +
    "uniform vec3 gravity; \n" +
    "uniform float damping; \n" +
    "uniform float factor; \n" +
    " \n" +
    "uniform samplerRect upper; \n" +
    "uniform samplerRect prev; \n" +
    "uniform samplerRect velocity; \n" +
    " \n" +
    "void main(void) { \n" +
    "  vec2 pos = gl_TexCoord[0].st; \n" + 
    "  vec3 prevPos = textureRect(prev, pos).xyz; \n" + 
    "  vec3 upperPos = textureRect(upper, pos).xyz; \n" + 
    "  vec3 vel = textureRect(velocity, pos).xyz; \n" + 
    " \n" +
    " vec3 dir = prevPos - upperPos + factor*(vel+gravity); \n" +
    " dir = 0.1*normalize(dir);\n" +
    "  if (point) {\n" +
    " \n" +
    " gl_FragColor =  vec4(upperPos +dir, 1.);\n" +
    " \n" +
    "  } else {\n" +
    " \n" +
    " gl_FragColor = vec4(damping*(vel- dot(vel,dir)*dir),1.); \n" +
    "  }\n" +
    "} \n";
  }
  
  protected void initViewport(GL2 gl, GLU glu) {
    gl.glMatrixMode(GL2.GL_PROJECTION);
    gl.glLoadIdentity();
    glu.gluOrtho2D(0, dataTextureSize, 0, dataTextureSize);
    gl.glMatrixMode(GL2.GL_MODELVIEW);
    gl.glLoadIdentity();
    gl.glViewport(0, 0, dataTextureSize, dataTextureSize);
  }

  protected void renderQuad(GL2 gl) {
    gl.glColor3f(1,0,0);
    gl.glPolygonMode(GL.GL_FRONT, GL2.GL_FILL);
    gl.glBegin(GL2.GL_QUADS);
      gl.glTexCoord2d(0.0, 0.0);
      gl.glVertex2d(0.0,0.0);
      gl.glTexCoord2d(isTex2D() ? 1 : dataTextureSize, 0.0);
      gl.glVertex2d(dataTextureSize, 0.0);
      gl.glTexCoord2d(isTex2D() ? 1 : dataTextureSize, isTex2D() ? 1 : dataTextureSize);
      gl.glVertex2d(dataTextureSize, dataTextureSize);
      gl.glTexCoord2d(0.0, isTex2D() ? 1 : dataTextureSize);
      gl.glVertex2d(0.0, dataTextureSize);
    gl.glEnd();
  }

  public void display( GLAutoDrawable drawable) {
    //GL gl = new DebugGL(drawable.getGL());
    GL2 gl = drawable.getGL().getGL2();
    GLU glu = new GLU(); //drawable.getGLU();
    if (hasData && doIntegrate) {
      
      initPrograms(gl);
      initFBO(gl);
      initViewport(gl, glu);

      initDataTextures(gl);


      valueBuffer.clear();
      valueBuffer.put(positions);
      positions.clear();
      // values fixed for this step
      program.setUniform("gravity", gravity);
      program.setUniform("damping", damping);
      program.setUniform("factor", factor);

      gl.glEnable(TEX_TARGET);
      
      for(int i = 0; i < NUM_ROWS-1; i++) {
      
        gl.glFramebufferTexture2D(GL2.GL_FRAMEBUFFER,
            GL2.GL_COLOR_ATTACHMENT0, TEX_TARGET, texIDsPositions[pingPong*NUM_ROWS+i+1], 0);
        gl.glFramebufferTexture2D(GL.GL_FRAMEBUFFER,
            GL2.GL_COLOR_ATTACHMENT1, TEX_TARGET, texIDsVelocities[pingPong*NUM_ROWS+i+1], 0);
    
        GpgpuUtility.checkBuf(gl);
        
        gl.glDrawBuffer(GL2.GL_COLOR_ATTACHMENT0);
        
        // set all values
        // ping pong - current values
        gl.glActiveTexture(GL.GL_TEXTURE0);
        gl.glBindTexture(TEX_TARGET, texIDsPositions[pingPong*NUM_ROWS+i]);
        program.setUniform("upper", 0);
  
        gl.glActiveTexture(GL.GL_TEXTURE1);
        gl.glBindTexture(TEX_TARGET, texIDsPositions[pongPing*NUM_ROWS+i+1]);
        program.setUniform("prev", 1);
        
        gl.glActiveTexture(GL.GL_TEXTURE2);
        gl.glBindTexture(TEX_TARGET, texIDsVelocities[pongPing*NUM_ROWS+i+1]);
        program.setUniform("velocity", 2);

        program.setUniform("point", true);
        GlslLoader.render(program, gl);
        renderQuad(gl);
        //expensive;
        gl.glFinish();
        valueBuffer.position((i+1)*NUM_COLS*4).limit((i+2)*NUM_COLS*4);
        //System.out.println(valueBuffer);
        gl.glReadBuffer(GL2.GL_COLOR_ATTACHMENT0);
        //expensive;
        gl.glReadPixels(0, 0, dataTextureSize, dataTextureSize, TEX_FORMAT, GL.GL_FLOAT, valueBuffer.slice());
        
        gl.glDrawBuffer(GL2.GL_COLOR_ATTACHMENT1);
        program.setUniform("point", false);
        GlslLoader.render(program, gl);
        renderQuad(gl);
        //expensive;
        //gl.glFinish();
        gl.glDrawBuffer(0);
      }
      
      // do swap
      int tmp = pingPong;
      pingPong = pongPing;
      pongPing = tmp;
    
      GlslLoader.postRender(program, gl); // any postRender just resets the shader pipeline
    
      // switch back to old buffer
      gl.glBindFramebuffer(GL2.GL_FRAMEBUFFER, 0);
    
      calculationFinished();
      gl.glDisable(TEX_TARGET);
    }
  }
  
  protected void initDataTextures(GL gl) {
    if (texIDsPositions[0] == 0) {
      gl.glGenTextures(texIDsPositions.length, texIDsPositions, 0);
      gl.glGenTextures(texIDsVelocities.length, texIDsVelocities, 0);
      for (int i = 0; i < texIDsPositions.length; i++) {
        setupTexture(gl, texIDsPositions[i], dataTextureSize);
        setupTexture(gl, texIDsVelocities[i], dataTextureSize);
      }
    }
    if (dataChanged) {
      transferToTexture(gl, positions, texIDsPositions[0], dataTextureSize);
      transferToTexture(gl, positions, texIDsPositions[NUM_ROWS], dataTextureSize);
      dataChanged = false;
    }
  }
  
  public void setPositions(double[] data) {
    positions.clear();
    assert(data.length*4 == positions.capacity()*3);
    for (int i = 0; i < data.length; i++) {
    positions.put((float) data[i]);    
    if(i%3==2) positions.put(1f);
    }
    positions.clear();
    hasData = true;
    dataChanged = true;
  }

  public void setPositions(float[] data) {
    positions.clear();
    assert(data.length == positions.capacity());
    positions.put(data);
    positions.clear();
    hasData = true;
    dataChanged = true;
  }

  public void triggerCalculation() {
    if (hasData) super.triggerCalculation();
  }

  public double getDamping() {
    return damping;
  }

  public void setDamping(double damping) {
    this.damping = damping;
  }

  public double getFactor() {
    return factor;
  }

  public void setFactor(double factor) {
    this.factor = factor;
  }

  public double[] getGravity() {
    return gravity;
  }

  public void setGravity(double[] gravity) {
    this.gravity = gravity;
  }

  protected void calculationFinished() {
//    FloatBuffer fb = getCurrentValues();
//    fb.clear();
//    GpgpuUtility.dumpData(fb);
//    System.out.println(fb);
//    fb.clear();
//    fb.limit(12);
//    fb.position(0);
//    GpgpuUtility.dumpSelectedData(fb);
//    
//    fb.limit(fb.capacity());
//    fb.position(fb.capacity()-12);
//    GpgpuUtility.dumpSelectedData(fb);
//    
    measure();
//    triggerCalculation();
  }
  
  public static void main(String[] args) {
    ClothCalculation cc = new ClothCalculation(40, 64);
    float[] f = GpgpuUtility.makeGradient(8);
    GpgpuUtility.dumpData(f);
    cc.setPositions(f);
    cc.setDisplayTexture(false);
    cc.triggerCalculation();
    GpgpuUtility.run(cc);
  }

public void dispose(GLAutoDrawable drawable) {
	// TODO Auto-generated method stub
	
}
}
