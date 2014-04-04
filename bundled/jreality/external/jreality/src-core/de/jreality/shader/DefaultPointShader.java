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

/**
 * The default point shader for jReality. 
 * @author Charles Gunn
 * @see DefaultPolygonShader  for general remarks on these shader interfaces.
 *
 */public interface DefaultPointShader extends PointShader {

  Object CREATE_DEFAULT=new Object();

	public static final Color DIFFUSE_COLOR_DEFAULT = Color.RED;
  public final static boolean SPHERES_DRAW_DEFAULT = true;
//  public final static boolean NORMALS_DRAW_DEFAULT = false;
  public final static double POINT_RADIUS_DEFAULT = 0.025;
  public final static double POINT_SIZE_DEFAULT = 3.0;
  public final static boolean ATTENUATE_POINT_SIZE_DEFAULT = true;
  
  public static final boolean RADII_WORLD_COORDINATES_DEFAULT = CommonAttributes.RADII_WORLD_COORDINATES_DEFAULT;

	public abstract Color getDiffuseColor();
	public abstract void setDiffuseColor(Color c);
  
  void setRadiiWorldCoordinates(Boolean b);
  Boolean getRadiiWorldCoordinates();

  Boolean getSpheresDraw();
  void setSpheresDraw(Boolean value);
  
//  Boolean getNormalsDraw();
//  void setNormalsDraw(Boolean value);
  
  Double getPointRadius();
  void setPointRadius(Double radius);
  
  Double getPointSize();
  void setPointSize(Double size);
  
  Boolean getAttenuatePointSize();
  void setAttenuatePointSize(Boolean b);
  
  PolygonShader getPolygonShader();
  PolygonShader createPolygonShader(String shaderName);
  
  TextShader getTextShader();
  TextShader createTextShader(String name);
}