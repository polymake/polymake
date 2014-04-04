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


package de.jreality.soft;

import java.util.logging.Level;
import java.util.logging.Logger;

import de.jreality.shader.EffectiveAppearance;
import de.jreality.shader.ShaderUtility;

/**
 * Utility class encapsulating the shader lookup algorithm.
 * Currently it will look for shader foo of type bar by looking for a class
 * named <code>de.jreality.soft.FooBarShader</code>. There's no caching yet.
 */
public class ShaderLookup
{
  private ShaderLookup(){}

  public static PolygonShader lookupPolygonShader(String name) {
    return (PolygonShader)lookup1(name, "Polygon");
  }
  public static VertexShader lookupVertexShader(String name) {
    try {
      return (VertexShader)lookup1(name, "Vertex");
    } catch (Exception e) {
      return new DefaultVertexShader();
    }
  }
  
  public static LineShader lookupLineShader(String name) {
      return (LineShader)lookup1(name, "Line");
  }
  public static PointShader lookupPointShader(String name) {
      return (PointShader)lookup1(name, "Point");
  }
  
  private static Object lookup1(String shaderName, String type) {
    Object ps;
    try
    {
      final String clName="de.jreality.soft."+Character.toUpperCase(
        shaderName.charAt(0))+shaderName.substring(1)+type+"Shader";
      Logger.getLogger("de.jreality").log(Level.FINEST, "attempt to load {0}", clName);
      final Class cl= Class.forName(clName);
      Logger.getLogger("de.jreality").log(Level.FINEST, "loaded {0}", cl);
      ps=cl.newInstance();
      Logger.getLogger("de.jreality").log(Level.FINEST, "instantiated {0}", cl);
    }
    catch(ClassNotFoundException ex)
    {
      type=Character.toUpperCase(type.charAt(0))+type.substring(1);
      Logger.getLogger("de.jreality").log(Level.WARNING,
        "unsupported {0} shader {1}", new String[] {type, shaderName});
      try {
        ps=Class.forName("de.jreality.soft.Default"+type+"Shader").newInstance();
      } catch (Exception e) {
        throw new Error();
      }
    }
    catch(Exception ex)
    {
      type=Character.toUpperCase(type.charAt(0))+type.substring(1);
      Logger.getLogger("de.jreality").log(Level.WARNING,
        "{0} shader {1} failed {2}", new Object[] {type, shaderName, ex});
      ps=new DefaultPolygonShader();
    }
    return ps;
  }

  public static VertexShader getVertexShaderAttr(
    EffectiveAppearance eAppearance, String base, String attr) {

    String vShader = (String)eAppearance.getAttribute(ShaderUtility.nameSpace(base, attr), "default");
    String vShaderName = (String)eAppearance.getAttribute(ShaderUtility.nameSpace(base, attr+".name"),
      ShaderUtility.nameSpace(base, attr));
//    System.out.println(vShaderName+" <= "+NameSpace.name(base, attr+".name")
//      +": "+NameSpace.name(base, attr));
    VertexShader vShaderImpl= ShaderLookup.lookupVertexShader(vShader);
    vShaderImpl.setup(eAppearance, vShaderName);
    return vShaderImpl;
  }

  public static PolygonShader getPolygonShaderAttr(
    EffectiveAppearance eAppearance, String base, String attr) {

    String vShader = (String)eAppearance.getAttribute(ShaderUtility.nameSpace(base, attr), "default");
    String vShaderName = (String)eAppearance.getAttribute(ShaderUtility.nameSpace(base, attr+".name"),
      ShaderUtility.nameSpace(base, attr));
    PolygonShader vShaderImpl= ShaderLookup.lookupPolygonShader(vShader);
    //System.out.println("base "+base+" attr "+attr+" vShaderName "+vShaderName+" vShader "+vShader);
    vShaderImpl.setup(eAppearance, vShaderName);
    return vShaderImpl;
  }

  public static LineShader getLineShaderAttr(
          EffectiveAppearance eAppearance, String base, String attr) {

      String vShader = (String)eAppearance.getAttribute(ShaderUtility.nameSpace(base, attr), "default");
      String vShaderName = (String)eAppearance.getAttribute(ShaderUtility.nameSpace(base, attr+".name"),
              ShaderUtility.nameSpace(base, attr));
      LineShader vShaderImpl= ShaderLookup.lookupLineShader(vShader);
      vShaderImpl.setup(eAppearance, vShaderName);
      return vShaderImpl;
  }
  public static PointShader getPointShaderAttr(
          EffectiveAppearance eAppearance, String base, String attr) {

      String vShader = (String)eAppearance.getAttribute(ShaderUtility.nameSpace(base, attr), "default");
      String vShaderName = (String)eAppearance.getAttribute(ShaderUtility.nameSpace(base, attr+".name"),
              ShaderUtility.nameSpace(base, attr));
      PointShader vShaderImpl= ShaderLookup.lookupPointShader(vShader);
      vShaderImpl.setup(eAppearance, vShaderName);
      return vShaderImpl;
  }
}
