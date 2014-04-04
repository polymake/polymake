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

import de.jreality.renderman.RIBVisitor;
import de.jreality.shader.CubeMap;
import de.jreality.shader.DefaultPolygonShader;
import de.jreality.shader.EffectiveAppearance;

/**
 * @author bleicher
 *
 */
public class TwoSidePolygonShader extends AbstractRendermanShader {
  
  CubeMap reflectionMap;  
  
  static int count = 0;
  de.jreality.shader.TwoSidePolygonShader tsps;
  DefaultPolygonShader front, back;
  de.jreality.renderman.shader.DefaultPolygonShader rfront, rback;
  
  public TwoSidePolygonShader(de.jreality.shader.TwoSidePolygonShader xxx)	{
	  super();	  
	  shaderName="twosidepolygonshader";
	  tsps = xxx;
	  front = (DefaultPolygonShader) tsps.getFront();
	  back = (DefaultPolygonShader) tsps.getBack();
	  rfront = new de.jreality.renderman.shader.DefaultPolygonShader(front);
	  rback = new de.jreality.renderman.shader.DefaultPolygonShader(back);
  }  

  public void setFromEffectiveAppearance(RIBVisitor ribv, EffectiveAppearance eap, String name) {    
    map.clear();
    rfront.setFromEffectiveAppearance(ribv, eap, name, "front");
    rback.setFromEffectiveAppearance(ribv, eap, name, "back");
    map.putAll(rfront.getAttributes());
    map.putAll(rback.getAttributes());
    map.put("color diffusecolorfront", front.getDiffuseColor());
    map.put("color diffusecolorback", back.getDiffuseColor());
  }
  
  public String getType() {
    return "Surface";
  }
}