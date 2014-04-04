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


/**
 * A point light class.  Situated at the origin (0,0,0,1); 
 * use scene graph transformations to
 * position it where you want.
 * <p>
 * The attenuation of the point light is proportional to
 * <i>1/(A0+A1*d+A2*d*d)</i> where <i>d</i> is the distant of the 
 * illuminated point from the light.
 * Set these attenuation factors using {@link #setFalloff(double, double, double)}.
 * <p>
 * TODO: Somebody needs to document what the shadow map settings are for.
 * @author Charles Gunn
 *
 */
public class PointLight extends Light {

	// Where are these fields defined?  [10.10.05 gunn]
	// I'm trying to debug my SpotLight but since I don't know what these values are intended to
	// represent, I'm having to guess.
	private double falloffA0 = 0.5;
	private double falloffA1 = 0.5;
	private double falloffA2 = 0;
	private boolean useShadowMap = false;
	private String shadowMap = "";
	private int shadowMapX = 512;
	private int shadowMapY = 512;

	private static int UNNAMED_ID;
	
	public PointLight() {
		super("point-light "+(UNNAMED_ID++));
	}
	
	public PointLight(String name) {
		super(name);
	}
	
  public double getFalloffA0() {
    startReader();
    try {
      return falloffA0;
    } finally {
      finishReader();
    }
	}

	public double getFalloffA1() {
    startReader();
    try {
      return falloffA1;
    } finally {
      finishReader();
    }
	}

	public double getFalloffA2() {
    startReader();
    try {
      return falloffA2;
    } finally {
      finishReader();
    }
	}

	/**
	 * Sets the falloffA0.
	 * @param falloffA0 The falloffA0 to set
	 */
	public void setFalloffA0(double falloffA0) {
    startWriter();
    if (this.falloffA0 != falloffA0) fireLightChanged();
    this.falloffA0 = falloffA0;
    finishWriter();
	}

	/**
	 * Sets the falloffA1.
	 * @param falloffA1 The falloffA1 to set
	 */
	public void setFalloffA1(double falloffA1) {
    startWriter();
    if (this.falloffA1 != falloffA1) fireLightChanged();
    this.falloffA1 = falloffA1;
    finishWriter();
	}

	/**
	 * Sets the falloffA2.
	 * @param falloffA2 The falloffA2 to set
	 */
	public void setFalloffA2(double falloffA2) {
    startWriter();
    if (this.falloffA2 != falloffA2) fireLightChanged();
    this.falloffA2 = falloffA2;
    finishWriter();
	}

	public void setFalloff(double a0, double a1, double a2)	{
    startWriter();
    if (falloffA0 != a0 ||
        falloffA1 != a1 ||
        falloffA2 != a2) fireLightChanged();
      falloffA0 = a0;
      falloffA1 = a1;
      falloffA2 = a2;
    finishWriter();
	}
  
	/**
	 * @param atten
   * @deprecated do we need that method?
	 */
	public void setFalloff(double[] atten) {
		if (atten.length <= 2) 
			// TODO signal error
			return;
		falloffA0 = atten[0];
		falloffA1 = atten[1];
		falloffA2 = atten[2];
    // TODO: compare with old value?
    fireLightChanged();
	}

	public String getShadowMap() {
    startReader();
    try {
      return shadowMap;
    } finally {
      finishReader();
    }
	}

	public boolean isUseShadowMap() {
    startReader();
    try {
      return useShadowMap;
    } finally {
      finishReader();
    }
	}

	/**
	 * Sets the shadowMap.
	 * @param shadowMap The shadowMap to set
	 */
	public void setShadowMap(String shadowMap) {
    startWriter();
    if (this.shadowMap != shadowMap) fireLightChanged();
    this.shadowMap = shadowMap;
    finishWriter();
}

	/**
	 * Sets the useShadowMap.
	 * @param useShadowMap The useShadowMap to set
	 */
	public void setUseShadowMap(boolean useShadowMap) {
    startWriter();
    if (this.useShadowMap != useShadowMap) fireLightChanged();
    this.useShadowMap = useShadowMap;
    finishWriter();
  }

	public int getShadowMapX() {
    startReader();
    try {
      return shadowMapX;
    } finally {
      finishReader();
    }
	}

	public int getShadowMapY() {
    startReader();
    try {
      return shadowMapY;
    } finally {
      finishReader();
    }
	}

	/**
	 * Sets the shadowMapX.
	 * @param shadowMapX The shadowMapX to set
	 */
	public void setShadowMapX(int shadowMapX) {
    startWriter();
    if (this.shadowMapX != shadowMapX) fireLightChanged();
    this.shadowMapX = shadowMapX;
    finishWriter();
	}

	/**
	 * Sets the shadowMapY.
	 * @param shadowMapY The shadowMapY to set
	 */
	public void setShadowMapY(int shadowMapY) {
    startWriter();
    if (this.shadowMapY != shadowMapY) fireLightChanged();
    this.shadowMapY = shadowMapY;
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
    
    static void superAccept(PointLight l, SceneGraphVisitor v) {
        l.superAccept(v);
      }
    private void superAccept(SceneGraphVisitor v) {
        super.accept(v);
      }
}
