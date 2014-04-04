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

import de.jreality.scene.IndexedFaceSet;

/**
 * This interface represents the default polygon shader used in jReality.
 * This interface can be used in conjunction with the {@link de.jreality.scene.data.AttributeEntity} 
 * class to set the actual {@link Appearance} attributes which the various
 * backends use when rendering the scene.  This avoids having to use the keys for these attributes 
 * (see {@link de.jreality.shader.CommonAttributes}). Backend writers can
 * use a similar strategy to get the attributes, too.
 * <p>
 * This shader implements a standard plastic-like surface shader where the
 * shaded color at a point P on the surface with normal vector N, eye vector
 * I, and light vector L 
 * is given by: <code><p>
 * Cs = Ka*ambient + Kd*(N.L)*diffuse + Ks*pow((L.R),Kexp)*specular
 * </code>
 * where R is the reflected eye vector.  Here, ambient, diffuse, and specular are
 * colors which are set using the methods {@link #setAmbientColor(Color)}), etc. --
 * but see below for exceptions related to the diffuse term.
 * <p>
 * The transparency value determines how much light the surface lets through. A value of 0
 * is opaque.  (See {@link de.jreality.shader.RenderingHintsShader#getTransparencyEnabled()}).
 * <p>
 * There is also a flag to determine whether facets are smooth-shaded or not ({@link #setSmoothShading(Boolean)}).
 * If <code>true</code>, then the shader uses vertex colors and vertex normals (assuming they are present)
 * as the <code>diffuse</code> color and the <code>N</code> vector in the above formula; 
 * if <code>false</code>, then the shader uses face colors and face normals instead (assuming they are present). 
 * See {@link IndexedFaceSet} for how to set these vertex and face attributes.
 * This means that the <code>diffuse</code> color can be gotten from the current diffuse color in the shader,
 * from the vertex colors attached to the geometry, or from  the face colors.
 * <p>
 * @author Charles Gunn
 * @see de.jreality.jogl.shader.DefaultGeometryShader
 *
 */
public interface DefaultPolygonShader extends PolygonShader {

	final static double AMBIENT_COEFFICIENT_DEFAULT = .0;

	final static Color AMBIENT_COLOR_DEFAULT = Color.WHITE;
	final static double DIFFUSE_COEFFICIENT_DEFAULT = 1.0;
	final static Color DIFFUSE_COLOR_DEFAULT = Color.BLUE;
	final static boolean SMOOTH_SHADING_DEFAULT = true;
	final static double SPECULAR_COEFFICIENT_DEFAULT = .7;
	final static Color SPECULAR_COLOR_DEFAULT = Color.WHITE;
	final static double SPECULAR_EXPONENT_DEFAULT = 60.;
	final static double TRANSPARENCY_DEFAULT = 0.0;
	Object CREATE_DEFAULT=new Object();

	TextShader createTextShader(String name);
	Texture2D createTexture2d();
	CubeMap createReflectionMap();
	Double getAmbientCoefficient();
	Color getAmbientColor();

	Double getDiffuseCoefficient();
	Color getDiffuseColor();

	Boolean getSmoothShading();
	Double getSpecularCoefficient();

	Color getSpecularColor();
	Double getSpecularExponent();

	TextShader getTextShader();
    Texture2D getTexture2d();
    //Texture2D getLightMap();
    CubeMap getReflectionMap();

	Double getTransparency();
	void setAmbientCoefficient(Double d);

	void setAmbientColor(Color c);
	void setDiffuseCoefficient(Double d);

	void setDiffuseColor(Color c);
	void setSmoothShading(Boolean b);

	void setSpecularCoefficient(Double d);
	void setSpecularColor(Color c);

	void setSpecularExponent(Double d);
	void setTransparency(Double d);
}