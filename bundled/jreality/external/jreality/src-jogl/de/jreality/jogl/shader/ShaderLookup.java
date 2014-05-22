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

package de.jreality.jogl.shader;

import java.util.HashMap;
import java.util.logging.Level;

import de.jreality.shader.EffectiveAppearance;
import de.jreality.shader.ShaderUtility;
import de.jreality.util.LoggingSystem;

/**
 * Utility class encapsulating the shader lookup algorithm. Currently it will
 * look for shader foo of type bar by looking for a class named
 * <code>de.jreality.jogl.FooBarShader</code>. There's no caching yet.
 */
public class ShaderLookup {

	private ShaderLookup() {
	}

	private static Object lookup2(String shaderName, String type) {
		Object ps;
		HashMap unknowns = new HashMap();
		try {
			final String clName = "de.jreality.jogl.shader."
					+ Character.toUpperCase(shaderName.charAt(0))
					+ shaderName.substring(1)
					+ Character.toUpperCase(type.charAt(0)) + type.substring(1);
			LoggingSystem.getLogger(ShaderLookup.class).log(Level.FINEST,
					"attempt to load {0}", clName);
			final Class cl = Class.forName(clName);
			LoggingSystem.getLogger(ShaderLookup.class).log(Level.FINEST,
					"loaded {0}", cl);
			ps = cl.newInstance();
			LoggingSystem.getLogger(ShaderLookup.class).log(Level.FINEST,
					"instantiated {0}", cl);
		} catch (ClassNotFoundException ex) {
			type = Character.toUpperCase(type.charAt(0)) + type.substring(1);
			if (unknowns.get(shaderName) != null) {
				unknowns.put(shaderName, shaderName);
				LoggingSystem.getLogger(ShaderLookup.class).warning(
						"unsupported shader " + shaderName);
			}
			ps = new DefaultPolygonShader();
		} catch (Exception ex) {
			type = Character.toUpperCase(type.charAt(0)) + type.substring(1);
			LoggingSystem.getLogger(ShaderLookup.class).warning(
					"shader " + shaderName + " failed");
			ps = new DefaultPolygonShader();
		}
		return ps;
	}

	/**
	 * 
	 * @deprecated
	 */
	public static Shader getShaderAttr(EffectiveAppearance eAppearance,
			String base, String type) {
		return getShaderAttr(eAppearance, base, type, type);
	}

	public static Shader getShaderAttr(EffectiveAppearance eAppearance,
			String base, String type, String attr) {
		// This returns the value of the string base+attr in the current
		// effective appearance, or "default" if not set
		String vShader = (String) eAppearance.getAttribute(
				ShaderUtility.nameSpace(base, attr), "default");
		if (vShader.equals("default"))
			vShader = (String) eAppearance.getAttribute(
					ShaderUtility.nameSpace(base, attr) + "name", "default");
		if (vShader.equals("default"))
			vShader = (String) eAppearance.getAttribute(
					ShaderUtility.nameSpace(base, attr) + ".name", "default");
		Shader vShaderImpl = (Shader) ShaderLookup.lookup2(vShader, type);
		String foo = ShaderUtility.nameSpace(base, attr);
		// Returns the value of base+attr+name, if it's set, or if not, gives
		// base+attr back.
		// String vShaderName =
		// (String)eAppearance.getAttribute(foo+"name",foo);
		// initialize the shader with the prefix stem vShaderName
		// System.err.println("Vshader name is "+vShaderName);
		vShaderImpl.setFromEffectiveAppearance(eAppearance, foo);
		return vShaderImpl;
	}
}
