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

import java.io.File;
import java.util.Map;

import de.jreality.math.Matrix;
import de.jreality.math.Pn;
import de.jreality.math.Rn;
import de.jreality.renderman.RIBHelper;
import de.jreality.renderman.RIBVisitor;
import de.jreality.scene.Appearance;
import de.jreality.shader.CommonAttributes;
import de.jreality.shader.CubeMap;
import de.jreality.shader.EffectiveAppearance;
import de.jreality.shader.ShaderUtility;
import de.jreality.shader.Texture2D;

/**
 * @author Charles Gunn
 *
 */
public class DefaultPolygonShader extends AbstractRendermanShader {
  
  CubeMap reflectionMap;  
  
  static int count = 0;
  de.jreality.shader.DefaultPolygonShader attent;
  
  public DefaultPolygonShader(de.jreality.shader.DefaultPolygonShader attent)	{
	  super();
	  this.attent = attent;
  }
  
  public Map getAttributes() {
    return map;
  }
  public void setFromEffectiveAppearance(RIBVisitor ribv, EffectiveAppearance eap, String name) {
	  setFromEffectiveAppearance(ribv, eap, name, "");
  }
  
  /**
   * To avoid having to keep all this code up-to-date for two sided shader, we allow an extra string
   * here
   * @param ribv
   * @param eap
   * @param name
   * @param side
   */
  public void setFromEffectiveAppearance(RIBVisitor ribv, EffectiveAppearance eap, String name, String side) {
    map.clear();
    
    int metric = eap.getAttribute(CommonAttributes.METRIC, Pn.EUCLIDEAN);
    boolean lighting = (boolean) eap.getAttribute(CommonAttributes.LIGHTING_ENABLED, true);
    map.put("float roughness"+side,new Float(1/attent.getSpecularExponent().floatValue()));
    map.put("float Ks"+side,new Float(attent.getSpecularCoefficient()));
    map.put("float Kd"+side,new Float(attent.getDiffuseCoefficient()));
    map.put("float Ka"+side,new Float(attent.getAmbientCoefficient()));
    map.put("color specularcolor"+side,attent.getSpecularColor());
    map.put("float lighting"+side, new Float( lighting ? 1 : 0));
//    System.err.println("name = "+name+" lighting = "+lighting);
    
    boolean transp = (boolean) eap.getAttribute(CommonAttributes.TRANSPARENCY_ENABLED,false);
    map.put("float transparencyenabled"+side,new Float(transp ? 1 : 0));    
    
    if (metric != Pn.EUCLIDEAN) {
      map.put("float signature", metric);
      map.put("float[16] objectToCamera", RIBHelper.fTranspose(ribv.getCurrentObjectToCamera()));
      shaderName = "noneuclideanpolygonshader";
    }
    else shaderName ="defaultpolygonshader" ;
    boolean ignoreTexture2d = eap.getAttribute(ShaderUtility.nameSpace(name,"ignoreTexture2d"), false);	
    if (!ignoreTexture2d && attent.getTexture2d() != null) { //AttributeEntityUtility.hasAttributeEntity(Texture2D.class, name+".texture2d", eap)) {
      Texture2D tex = attent.getTexture2d(); //AttributeEntityUtility.createAttributeEntity(Texture2D.class, ShaderUtility.nameSpace(name,"texture2d"), eap);
      
      String fname = null;
      fname = (String) eap.getAttribute(CommonAttributes.RMAN_TEXTURE_FILE,"");
      if (fname == "")	{
        fname = null;
      } 
      if (fname == null) {
        fname = new File(ribv.writeTexture(tex)).getName();
      }
      // removed texfile path stripping -> is just the filename without path now. 
      map.put("string texturename"+side,fname);
      Matrix textureMatrix = tex.getTextureMatrix();
      double[] mat = textureMatrix.getArray();
      if(mat != null && !Rn.isIdentityMatrix(mat, 10E-8)) {
        map.put("float[16] tm"+side, RIBHelper.fTranspose(mat));
      }
    }
    
    
    if (attent.getReflectionMap() != null) { //AttributeEntityUtility.hasAttributeEntity(CubeMap.class, ShaderUtility.nameSpace(name,"reflectionMap"), eap))
      reflectionMap = attent.getReflectionMap(); //TextureUtility.readReflectionMap(eap, ShaderUtility.nameSpace(name,"reflectionMap"));
      if((boolean) eap.getAttribute(CommonAttributes.RMAN_RAY_TRACING_REFLECTIONS,false))
        map.put("float raytracedreflections", new Float(1));
      else{
        map.put("float raytracedreflections", new Float(0));
        String fname = (String) eap.getAttribute(CommonAttributes.RMAN_REFLECTIONMAP_FILE,"");
        if (fname == "") {
          fname = null;
        }
        if (fname == null) {
          fname = new File(ribv.writeCubeMap(reflectionMap)).getName();
        }
        map.put("string reflectionmap"+side, fname);
      }
      map.put("float reflectionBlend"+side, new Float(reflectionMap.getBlendColor().getAlpha()/255.0));
    }    
    
    //volume shaders
    Object obj1 = eap.getAttribute(CommonAttributes.RMAN_VOLUME_INTERIOR_SHADER, Appearance.INHERITED,SLShader.class);
    Object obj2 = eap.getAttribute(CommonAttributes.RMAN_VOLUME_EXTERIOR_SHADER, Appearance.INHERITED,SLShader.class);     
    if((obj1 != Appearance.INHERITED && (boolean) eap.getAttribute(CommonAttributes.RMAN_RAY_TRACING_VOLUMES,false))||obj2 != Appearance.INHERITED)
      map.put("float raytracedvolumes", new Float(1));    
  }
  
  public String getType() {
    return "Surface";
  }
}