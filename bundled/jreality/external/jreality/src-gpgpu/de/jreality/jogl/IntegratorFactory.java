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


package de.jreality.jogl;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Map;

import de.jreality.util.LoggingSystem;

/**
 * factory for easy creation of glsl code for runge kutta integration on the gpu
 * @author weissman
 *
 */
public class IntegratorFactory {

	enum IntegrationScheme {
	EULER(
    "void main(void) {" +
    "  vec2 pos = gl_TexCoord[0].st;\n" + 
    "  vec4 pt = textureRect(values, pos);\n" + 
    "  vec4 res = pt + h*evaluateT0(pt);\n" + 
    "  if (r3) res.w = 1.;\n" + 
    "  gl_FragColor = res;\n" +
    "}\n"),

	MIDPOINT(
    "void main(void) {" +
    "  vec2 pos = gl_TexCoord[0].st;\n" + 
    "  vec4 pt = textureRect(values, pos);\n" + 
    "  vec4 res = pt + h*evaluateT0_H2(pt);\n" + 
    "  if (r3) res.w = 1.;\n" + 
    "  gl_FragColor = res;\n" +
    "}\n"),

    RK2(
    "void main(void) {" +
    "  vec2 pos = gl_TexCoord[0].st;\n" + 
    "  vec4 pt = textureRect(values, pos);\n" + 
    "  vec4 k1 = h * evaluateT0(pt);\n" + 
    "  vec4 k2 = h * evaluateT0_H2(pt+k1/2.);\n" + 
    "  vec4 res = pt + k2;\n" + 
    "  if (r3) res.w = 1.;\n" + 
    "  gl_FragColor = res;\n" +
    "}\n"),

  RK4(
    "void main(void) {" +
    "  vec2 pos = gl_TexCoord[0].st;\n" +
    "  vec4 pt = textureRect(values, pos);\n" +
    "  vec4 k1 = h * evaluateT0(pt);\n" +
    "  vec4 k2 = h * evaluateT0_H2(pt+k1/2.);\n" +
    "  vec4 k3 = h * evaluateT0_H2(pt+k2/2.);\n" +
    "  vec4 k4 = h * evaluateT0_H(pt+k3);\n" +
    "  vec4 res = pt + (k1 + 2.*(k2 + k3) + k4)/6.;\n" +
    "  if (r3) res.w = 1.;\n" + 
    "  gl_FragColor = res;\n" +
    "}\n");

  	String src;
	
  	IntegrationScheme(String src) {
		this.src=src;
	}

	}
  
  private HashSet uniforms=new HashSet();
  private HashSet signatures=new HashSet();
  private HashMap<String, String> methods=new HashMap<String, String>();

  private HashSet constants=new HashSet();
  
  private IntegrationScheme scheme;

  private IntegratorFactory(IntegrationScheme scheme) {
	this.scheme=scheme;
    addUniform("h", "float");
    addUniform("r3", "bool");
    addUniform("values", "samplerRect");
  }
  

  /**
   * create a factory for runge kutta integration of order 2
   * @return the factory
   */
  public static IntegratorFactory euler() {
    return new IntegratorFactory(IntegrationScheme.EULER);
  }

  /**
   * create a factory for midpoint integration
   * @return the factory
   */
  public static IntegratorFactory midpoint() {
    return new IntegratorFactory(IntegrationScheme.MIDPOINT);
  }
  
  /**
   * create a factory for runge kutta integration of order 2
   * @return the factory
   */
  public static IntegratorFactory rk2() {
    return new IntegratorFactory(IntegrationScheme.RK2);
  }
  
  /**
   * create a factory for runge kutta integration of order 4
   * @return the factory
   */
  public static IntegratorFactory rk4() {
    return new IntegratorFactory(IntegrationScheme.RK4);
  }

  /**
   * add a uniform parameter to the src code
   * @param name the name of the uniform param
   * @param type the type of the param
   */
  public void addUniform(String name, String type) {
    uniforms.add("uniform "+type+" "+name+";");
  }
  
  /**
   * add a constantto the src code
   * @param definition the source line for the constant, i.e. <code> const float PI = 3.141592653589793; </code>
   */
  public void addConstant(String definition) {
    constants.add(definition);
  }
  
  public void addMethod(String name, String retType, String params, String implementation) {
    String signature = retType+" "+name+"("+params+")";
    String impl = "{\n"+implementation+"}\n";
    signatures.add(signature);
    String prevImpl = methods.put(signature, impl);
    if (prevImpl != null) {
    	LoggingSystem.getLogger(IntegratorFactory.class).info("Overwriting glsl method: "+signature);
    }
  }
  
  public void srcT0(String impl) {
	if (scheme == IntegrationScheme.MIDPOINT) throw new IllegalStateException("no such method for midpoint rule");
    addMethod("evaluateT0", "vec4", "const vec4 point", impl);
  }

  public void srcT0_H2(String impl) {
    if (scheme == IntegrationScheme.EULER) throw new IllegalStateException("no such method for euler"); 
    addMethod("evaluateT0_H2", "vec4", "const vec4 point", impl);
  }

  public void srcT0_H(String impl) {
    if (scheme != IntegrationScheme.RK4) throw new IllegalStateException("no such method for RK4"); 
    addMethod("evaluateT0_H", "vec4", "const vec4 point", impl);
  }

  public void srcAll(String impl) {
    if (scheme != IntegrationScheme.MIDPOINT) srcT0(impl);
    if (scheme != IntegrationScheme.EULER) srcT0_H2(impl);
    if (scheme == IntegrationScheme.RK4) srcT0_H(impl);
  }
  
  String overwrittenMain=null;
  public void overwriteMain(String src) {
	  overwrittenMain=src;
  }
  
  public String toString() {
    StringBuffer sb = new StringBuffer();
    for(Iterator i = constants.iterator(); i.hasNext(); )
      sb.append(i.next()).append('\n');
    for (Iterator i = uniforms.iterator(); i.hasNext(); )
      sb.append(i.next()).append('\n');
    sb.append('\n');
    for (Iterator i = signatures.iterator(); i.hasNext(); )
      sb.append(i.next()).append(';').append('\n');
    sb.append('\n');
    if (overwrittenMain == null) sb.append(scheme.src);
    else sb.append("void main() {\n").append(overwrittenMain).append('\n').append('}').append('\n');
    for (Map.Entry<String, String> m : methods.entrySet() )
      sb.append(m.getKey() + m.getValue()).append('\n');
    sb.append('\n');
    return sb.toString();
  }

}
