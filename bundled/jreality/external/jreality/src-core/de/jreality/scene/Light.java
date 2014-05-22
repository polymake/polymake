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


package de.jreality.scene;

import java.awt.Color;

import de.jreality.scene.event.LightEvent;
import de.jreality.scene.event.LightEventMulticaster;
import de.jreality.scene.event.LightListener;

/**
 * Light is the abstract super class to all lights in the scene.
 * It carries a color and an intensity as the only common properties of all lights.
 * <p>
 * Warning: currently, all lights are considered to be global ({@link #setGlobal(boolean)})
 * in all backends.
 * @author <a href="mailto:hoffmann@math.tu-berlin.de">Tim Hoffmann</a>
 * 
 * TODO: make lights immutable - make a LightFactory
 * 
 */
public abstract class Light extends SceneGraphNode {
  
  private transient LightEventMulticaster lightListener = new LightEventMulticaster();
  private transient boolean lightChanged;
  
  private Color color= new Color(1f, 1f, 1f);
  private double intensity = 0.75;
  private boolean global =true;
  private boolean ambientFake;

	public Light(String name) {
		super(name);
	}
	
	public Color getColor() {
    startReader();
    try {
      return color;
    } finally {
      finishReader();
    }
	}
	public void setColor(Color color) {
    startWriter();
    if (this.color != color) fireLightChanged();
		this.color = color;
    finishWriter();
	}
  
  /**
   * get the current color of this light as a triple of floats, premultiplied by the intensity factor.
   * @return float[]
   */
  public float[] getScaledColorAsFloat() {
    float[] cc = getColor().getRGBColorComponents(null);
    for (int i = 0; i<3; ++i)  cc[i] *= intensity;
    return cc;
  }

    /**
     * Get this light's intensity
     * @return double the intensity
     */
    public double getIntensity() {
      startReader();
      try {
        return intensity;
      } finally {
        finishReader();
      }
    }

    /**
     * Sets the intensity.
     * @param intensity the intensity
     */
    public void setIntensity(double intensity) {
      startWriter();
      if (this.intensity != intensity) fireLightChanged();
      this.intensity = intensity;
      finishWriter();
    }

    /**
     * @return Returns wether the light is global for the scene.
     */
    public boolean isGlobal() {
      startReader();
      try {
        return global;
      } finally {
        finishReader();
      }
    }

    /**
     * @param global: setting wether the light is global for the scene.
     */
    public void setGlobal(boolean global) {
      startWriter();
      if (this.global != global) fireLightChanged();
      this.global = global;
      finishWriter();
    }
    
    /**
     * @return Returns whether the light should be considered as a fake for
     * ambient light (so sophisticated renderers might ignore it).
     */
    public boolean isAmbientFake() {
      startReader();
      try {
        return ambientFake;
      } finally {
        finishReader();
      }
    }
    
    /**
     * @param global: setting whether the light should be considered as a fake for
     * ambient light (so sophisticated renderers might ignore it).
     */
    public void setAmbientFake(boolean b) {
      startWriter();
      if (ambientFake != b) fireLightChanged();
      ambientFake = b;
      finishWriter();
    }
 
    public void accept(SceneGraphVisitor v) {
      startReader();
      try {
        v.visit(this);
      } finally {
        finishReader();
      }
    }
  
    static void superAccept(Light l, SceneGraphVisitor v) {
      l.superAccept(v);
    }
    private void superAccept(SceneGraphVisitor v) {
      super.accept(v);
    }

    public void addLightListener(LightListener listener) {
      startReader();
      lightListener.add( listener);
      finishReader();
    }

    public void removeLightListener(LightListener listener) {
      startReader();
      lightListener.remove(listener);
      finishReader();
    }
    
    protected void fireLightChanged() {
      lightChanged=true;
    }
    
    protected void fireLightChangedImpl() {
      if (lightListener != null) lightListener.lightChanged(new LightEvent(this));
    }

    protected void writingFinished() {
      if (lightChanged) fireLightChangedImpl();
      lightChanged=false;
    }

}
