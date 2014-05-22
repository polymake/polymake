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


package de.jreality.renderman.shader;

import java.awt.Color;

import de.jreality.math.Rn;
import de.jreality.renderman.RIBVisitor;
import de.jreality.shader.DefaultPolygonShader;
import de.jreality.shader.EffectiveAppearance;

public class RopePolygonShader extends de.jreality.renderman.shader.DefaultPolygonShader {
	public RopePolygonShader(DefaultPolygonShader attent) {
		super(attent);
		// TODO Auto-generated constructor stub
	}

	Color band1color = new Color(1f,1f,1f), 
		band2color = new Color(.8f, .8f, .8f), 
		shadowcolor = new Color(0f,0f,0f), 
		gapcolor = new Color(0f,0f,0f,0f),
		blendcolor = new Color(0f,0f,0f,0f);
;
	double bandwidth, shadowwidth, blendfactor, gapalpha;
	double[] textureMatrix = Rn.identityMatrix(4);
	
	public void setFromEffectiveAppearance(RIBVisitor ribv, EffectiveAppearance eap, String name) {
		super.setFromEffectiveAppearance(ribv, eap, name);
		shaderName = "ropeshader";			// name of RIB shader
        bandwidth = eap.getAttribute(name+"."+"bandwidth", .7);
        shadowwidth = eap.getAttribute(name+"."+"shadowwidth", .12);
        blendfactor = eap.getAttribute(name+"."+"blendfactor", 1.0);
        gapalpha = eap.getAttribute(name+"."+"gapalpha", 0.0);
        band1color =(Color) eap.getAttribute(name+"."+"band1color", band1color);
        band2color =(Color) eap.getAttribute(name+"."+"band2color", band2color);
        shadowcolor =(Color) eap.getAttribute(name+"."+"shadowcolor", shadowcolor);
        gapcolor =(Color) eap.getAttribute(name+"."+"gapcolor", gapcolor);
        blendcolor =(Color) eap.getAttribute(name+"."+"blendcolor", blendcolor);
        textureMatrix =(double[]) eap.getAttribute(name+"."+"textureMatrix", textureMatrix);
        map.put("bandwidth",new Float((float) bandwidth));
        map.put("shadowwidth",new Float((float) shadowwidth));
        map.put("blendfactor",new Float((float) blendfactor));
        map.put("gapalpha",new Float((float) gapalpha));
        map.put("band1color", band1color);
        map.put("band2color", band2color);
        map.put("shadowcolor", shadowcolor);
        map.put("gapcolor", gapcolor);
        map.put("blendcolor", blendcolor);
        map.put("textureMatrix", textureMatrix);
        map.remove("string texturename");
	}

}
