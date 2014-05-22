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

import java.awt.Color;

import de.jreality.scene.Appearance;
import de.jreality.scene.data.AttributeEntityUtility;

/**
 * @author gunn
 * 
 */
public class ShaderUtility {

  private ShaderUtility() {
  }

  public static Color combineDiffuseColorWithTransparency(Color diffuseColor, double transparency, boolean useOldTransparency) {
    // LoggingSystem.getLogger().log(Level.FINE,"Input: c3, transparency:
    // "+diffuseColor.getAlpha()/255.0f+" "+transparency);
	  Color ret;
	  double alpha = 1.0-transparency;
	  if (useOldTransparency)	{
		   double alpha2 = diffuseColor.getAlpha() / 255.0f;
		   alpha = alpha * alpha2;
		   if (alpha < 0.0)
		      alpha = 0.0;
		   if (alpha > 1.0)
		      alpha = 1.0;
	  }
	  float[] f = diffuseColor.getRGBComponents(null);
	  // LoggingSystem.getLogger().log(Level.FINE,"Alpha is "+alpha);		  
	  ret = new Color(f[0], f[1], f[2], (float) alpha);
    // LoggingSystem.getLogger().log(Level.FINE,"f[3] is "+f[3]);
    // LoggingSystem.getLogger().log(Level.FINE,"Output: c3, alpha:
    // "+ret.getAlpha()/255.0f+" "+alpha);
    return ret;
  }

  public static String nameSpace(String s1, String s2) {
    return s1.length() == 0 ? s2 : s1 + '.' + s2;
  }

  public static DefaultGeometryShader createDefaultGeometryShader(Appearance a, boolean readDefaults) {
    return (DefaultGeometryShader) AttributeEntityUtility.getAttributeEntity(DefaultGeometryShader.class, "", a, readDefaults);
  }

  public static DefaultGeometryShader createDefaultGeometryShader(EffectiveAppearance ea) {
      return (DefaultGeometryShader) AttributeEntityUtility.createAttributeEntity(DefaultGeometryShader.class, "", ea);
    }

  public static RenderingHintsShader createDefaultRenderingHintsShader(Appearance a, boolean readDefaults) {
	    return (RenderingHintsShader) AttributeEntityUtility.getAttributeEntity(RenderingHintsShader.class, "", a, readDefaults);
	  }

  public static RenderingHintsShader createRenderingHintsShader(EffectiveAppearance ea) {
      return (RenderingHintsShader) AttributeEntityUtility.createAttributeEntity(RenderingHintsShader.class, "", ea);
    }

 public static RootAppearance createRootAppearance(Appearance a) {
      return (RootAppearance) AttributeEntityUtility.createAttributeEntity(RootAppearance.class, "", a, true);
    }
  
  public static Class resolveEntity(Class type, String name) {
    if (PointShader.class.isAssignableFrom(type)) {
      if (name.equals("default")) return DefaultPointShader.class;
      throw new IllegalArgumentException(" no such point shader ["+name+"]");
    }
    if (LineShader.class.isAssignableFrom(type)) {
      if (name.equals("default")) return DefaultLineShader.class;
      throw new IllegalArgumentException(" no such line shader ["+name+"]");
    }
    if (PolygonShader.class.isAssignableFrom(type)) {
      if (name.equals("twoSide")) return TwoSidePolygonShader.class;
      if (name.equals("default")) return DefaultPolygonShader.class;
      if (name.equals("hatch"))
		try {
			return Class.forName("de.jreality.shader.HatchPolygonShader");
		} catch (ClassNotFoundException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
     if (name.equals("etch"))
        try {
                return Class.forName("de.jreality.shader.EtchPolygonShader");
            } catch (ClassNotFoundException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
    try {
        return Class.forName("de.jreality.shader."+name.substring(0,1).toUpperCase()+name.substring(1)+"PolygonShader");
    } catch (ClassNotFoundException e) {
        // TODO Auto-generated catch block
        e.printStackTrace();
    }
      throw new IllegalArgumentException(" no such polygon shader ["+name+"]");
    }
    if (TextShader.class.isAssignableFrom(type)) {
        if (name.equals("default")) return DefaultTextShader.class;
        throw new IllegalArgumentException(" no such text shader ["+name+"]");
      }
   throw new IllegalArgumentException("unhandled entity class "+type);
  }

public static HapticShader createHapticShader(Appearance node) {
	return (HapticShader) AttributeEntityUtility.createAttributeEntity(HapticShader.class, CommonAttributes.HAPTIC_SHADER, node, true);
}
}
