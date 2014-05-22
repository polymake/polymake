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


package de.jreality.shader;

import java.io.IOException;
import java.nio.FloatBuffer;
import java.util.WeakHashMap;

import de.jreality.scene.Appearance;
import de.jreality.scene.event.AppearanceEvent;
import de.jreality.scene.event.AppearanceListener;
import de.jreality.shader.GlslSource.UniformParameter;
import de.jreality.util.Input;

public class GlslProgram {
  
  private static final Object EMPTY=new Object();

  private final GlslSource source;
  private final Appearance app;
  private final EffectiveAppearance eApp;
  private final WeakHashMap<String, Object> uniforms = new WeakHashMap<String, Object>();
  
  private final String pre;
  
  public static boolean hasGlslProgram(EffectiveAppearance eap, String prefix) {
	    Object prog = eap.getAttribute(prefix+"::glsl-source", EMPTY, Object.class);
	    return (prog != EMPTY);
	  }
	  
  public static boolean hasGlslProgram(Appearance eap, String prefix) {
	    Object prog = eap.getAttribute(prefix+"::glsl-source");
	    return (prog instanceof GlslSource);
	  }
	  
  public GlslProgram(Appearance app, String prefix, Input vertexProgram, Input fragmentProgram) throws IOException {
		 this(app, prefix,   new GlslSource(vertexProgram, fragmentProgram));
  }

  public GlslProgram(Appearance app, String prefix, GlslSource s) {
	  	source = s;
	    this.app = app;
	    this.eApp = EffectiveAppearance.create().create(app);
	    pre = prefix+"::glsl-";
	    app.setAttribute(pre+"source", source);
	    addAppListener(app);
  }
 
  public GlslProgram(Appearance app, String prefix, String vertexProgram, String fragmentProgram) {
	    source = new GlslSource(vertexProgram, fragmentProgram);
	    this.app = app;
	    this.eApp = EffectiveAppearance.create().create(app);
	    pre = prefix+"::glsl-";
	    app.setAttribute(pre+"source", source);
	    addAppListener(app);
  }

  public GlslProgram(Appearance app, String prefix, String[] vertexProgram, String[] fragmentProgram) {
    source = new GlslSource(vertexProgram, fragmentProgram);
    this.app = app;
    this.eApp = EffectiveAppearance.create().create(app);
    pre = prefix+"::glsl-";
    app.setAttribute(pre+"source", source);
    addAppListener(app);
  }
  
  public GlslProgram(EffectiveAppearance eap, String prefix) {
    this.app = null;
    this.eApp = eap;
    pre = prefix+"::"+"glsl-";
    if (!hasGlslProgram(eap, prefix)) throw new IllegalStateException("no program!");
    source = (GlslSource) eap.getAttribute(pre+"source", EMPTY, Object.class);
  }
  
  public GlslProgram(Appearance app, String prefix) {
	  	this.app = app;
	    this.eApp = EffectiveAppearance.create().create(app);
	    pre = prefix+"::"+"glsl-";
	    if (!hasGlslProgram(app, prefix)) throw new IllegalStateException("no program!");
	    source = (GlslSource) app.getAttribute(pre+"source");
	    addAppListener(app);
  }

  private void addAppListener(Appearance ap)	{
	  ap.addAppearanceListener(new AppearanceListener() {

		public void appearanceChanged(AppearanceEvent ev) {
			uniforms.clear();
		}
		  
	  });
  }
  /**
   * this makes only sense if app is the last appearance pushed on the EffectiveAppearance stack!
   */
  public GlslProgram(Appearance app, EffectiveAppearance eap, String prefix) {
	    this.eApp = eap;
	  	this.app = app;
	    pre = prefix+"::"+"glsl-";
	    if (!hasGlslProgram(eap, prefix)) throw new IllegalStateException("no program!");
	    source = (GlslSource) eap.getAttribute(pre+"source", EMPTY, Object.class);
	    addAppListener(app);
  }
	  
  private void checkWrite() {
    if (app == null) throw new IllegalStateException("not writable!");
  }
  
  private void checkParam(String name, String type, boolean array, boolean matrix) {
    UniformParameter param = source.getUniformParameter(name);
    if (param == null) throw new IllegalStateException("no such parameter: "+name);
    if (type != null && !param.getType().equals(type)) throw new IllegalArgumentException("wrong type");
  }
  
  public void setUniform(String name, boolean value) {
    checkWrite();
    checkParam(name, "bool", false, false);
    assign(name, new int[]{value ? 1 : 0});
  }

  public void setUniform(String name, float value) {
    checkWrite();
    checkParam(name, "float", false, false);
    assign(name, new float[]{value});
  }

  public void setUniform(String name, int value) {
    checkWrite();
    checkParam(name, null, false, false);
    assign(name, new int[]{value});
  }

  public void setUniform(String name, float[] values) {
    checkWrite();
    checkParam(name, null, true, false);
    assign(name, values.clone());
  }
  
  public void setUniform(String name, int[] values) {
    checkWrite();
    checkParam(name, null, true, false);
    assign(name, values.clone());
  }

  private void assign(String name, Object values) {
    app.setAttribute(pre+name, values);
  }
  
  public void setUniform(String name, double value) {
    setUniform(name, (float)value);
  }
  
  public void setUniform(String name, double[] values) {
    float[] floats = new float[values.length];
    for (int i = 0; i < values.length; i++) floats[i] = (float) values[i];
    checkWrite();
    checkParam(name, null, true, false);
    assign(name, floats);
  }

  public void setUniform(String name, FloatBuffer data) {
    float[] floats = new float[data.remaining()];
    data.get(floats);
    checkWrite();
    checkParam(name, null, true, false);
    assign(name, floats);
  }

  public void setUniformMatrix(String name, float[] matrix) {
    checkWrite();
    checkParam(name, null, true, true);
    assign(name, matrix);
  }
  
  public void setUniformMatrix(String name, double[] matrix) {
    float[] floats = new float[matrix.length];
    for (int i = 0; i < matrix.length; i++) floats[i] = (float) matrix[i];
    setUniformMatrix(name, floats);
  }
  
  	public Object getUniform(String name) {
  		UniformParameter param = source.getUniformParameter(name);
  		if (param == null) throw new IllegalArgumentException("no such uniform param");
  		Object val = uniforms.get(name);
  		if (val == null)	{
		    val = eApp.getAttribute(pre+name, EMPTY, Object.class);
	  		if (val == EMPTY) return null;
		    uniforms.put(name, val);
  		}
  		return val;
  	}
  
  public GlslSource getSource() {
    return source;
  }
  
  public static class UniformValue {
    private final UniformParameter param;
    private final Object value;
    private UniformValue(UniformParameter param, Object value) {
      this.param=param;
      this.value=value;
    }
    public UniformParameter getParameter() {
      return param;
    }
    public Object getValue() {
      return value;
    }
  }

}
