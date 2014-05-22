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
 * This is a spot light. The light direction is the z-axis.
 * Other directions may be obtained by changing the transformation. 
 * <p>
 * The cone angle is specified in radians. The <i>distribution</i> is
 * an exponent specifying the distribution of the light directions
 * around the axis.  A value of <i>0</i> implies the light is evenly
 * distributed around the axis.  
 * <p>
 * @author <a href="mailto:hoffmann@math.tu-berlin.de">Tim Hoffmann</a>
 *
 */
public class SpotLight extends PointLight {

	private static int UNNAMED_ID;
	
	public SpotLight() {
		super("spot-light "+(UNNAMED_ID++));
	}
	
	public SpotLight(String name) {
		super(name);
	}

    private double coneAngle = Math.PI / 6;
    private double coneDeltaAngle = coneAngle / 3.;
    private double distribution = 2;

    public double getConeAngle() {
      startReader();
      try {
        return coneAngle;
      } finally {
        finishReader();
      }
    }

    /**
     * Sets the coneAngle. This is the maximal illuminated cone.
     * @param coneAngle The coneAngle to set
     */
    public void setConeAngle(double coneAngle) {
      startWriter();
      if (this.coneAngle != coneAngle) fireLightChanged();
      this.coneAngle = coneAngle;
      finishWriter();
    }

    public double getConeDeltaAngle() {
      startReader();
      try {
        return coneDeltaAngle;
      } finally {
        finishReader();
      }
    }

    public double getDistribution() {
      startReader();
      try {
        return distribution;
      } finally {
        finishReader();
      }
    }

    /**
     * Sets the coneDeltaAngle. This angle gives the width of the smooth falloff of the light's intensity towards the 
     * edge of the coneAngle.
     * @param coneDeltaAngle The coneDeltaAngle to set
     */
    public void setConeDeltaAngle(double coneDeltaAngle) {
      startWriter();
      if (this.coneDeltaAngle != coneDeltaAngle) fireLightChanged();
      this.coneDeltaAngle = coneDeltaAngle;
      finishWriter();
    }

    /**
     * Sets the distribution. This is the regular falloff of the lights intensity towards the edge of the cone
     * it is an exponent. 
     * @param distribution The distribution to set
     */
    public void setDistribution(double distribution) {
      startWriter();
      if (this.distribution != distribution) fireLightChanged();
      this.distribution = distribution;
      finishWriter();
    }

    public void accept(SceneGraphVisitor v) {
      v.visit(this);
    }
    static void superAccept(SpotLight l, SceneGraphVisitor v) {
      l.superAccept(v);
    }
    private void superAccept(SceneGraphVisitor v) {
      super.accept(v);
    }

}
