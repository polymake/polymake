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


package de.jreality.softviewer.shader;

import java.awt.Color;

import de.jreality.shader.CommonAttributes;
import de.jreality.shader.EffectiveAppearance;
import de.jreality.shader.ShaderUtility;
import de.jreality.softviewer.Environment;
import de.jreality.softviewer.Polygon;

/**
 * 
 * @version 1.0
 * @author <a href="mailto:hoffmann@math.tu-berlin.de">Tim Hoffmann</a>
 *
 */
public class ConstantVertexShader extends VertexShader {

	private double red;
	private double green;
	private double blue;
	private double transparency = 0;

	public ConstantVertexShader(double r, double g, double b) {
		super();
		red = r;
		green = g;
		blue = b;
	}
	
	public ConstantVertexShader(Color c, double transparency) {
		super();
        float[] rgb=c.getComponents(null);
        red=rgb[0];
        green=rgb[1];
        blue=rgb[2];
		this.transparency = transparency;
	}

	public void shadeVertex(
		double[] vertex,
		Environment environment, boolean vertexColors) {
        vertexColors |= this.vertexColors;
        if(!vertexColors) {
			vertex[Polygon.R] = red;
			vertex[Polygon.G] = green;
			vertex[Polygon.B] = blue;
        }
	}


	public double getTransparency() {

		return transparency;
	}

	public double getBlue() {
		return blue;
	}

	public double getGreen() {
		return green;
	}

	public double getRed() {
		return red;
	}

	public void setBlue(double blue) {
		this.blue = blue;
	}

	public void setGreen(double green) {
		this.green = green;
	}

	public void setRed(double red) {
		this.red = red;
	}

    public void setTransparency(double transparency) {
        this.transparency = transparency;
    }

    public void setup(EffectiveAppearance eAppearance, String name) {
      transparency = eAppearance.getAttribute(ShaderUtility.nameSpace(name, "transparency"), transparency);
      Color c = (Color)eAppearance.getAttribute(ShaderUtility.nameSpace(name, "color"), Color.BLACK);
      c = (Color)eAppearance.getAttribute(ShaderUtility.nameSpace(name, CommonAttributes.DIFFUSE_COLOR), c);
      float[] rgb=c.getComponents(null);
     
          red=rgb[0];
          green=rgb[1];
          blue=rgb[2];
     
    }

    @Override
    public void setColor(double r, double g, double b) {
        red = r;
        green = g;
        blue = b;
        
    }
}
