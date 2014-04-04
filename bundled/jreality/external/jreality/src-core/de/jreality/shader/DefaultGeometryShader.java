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

import de.jreality.scene.data.AttributeCollection;

/**
 * This interface represents the default geometry shader in jReality.
 * It consists of three sub-shaders, for polygons, lines, and points,
 * and three corresponding boolean values to control whether these three elements
 * are to be drawn.
 * @author Charles Gunn
 *
 */public interface DefaultGeometryShader extends AttributeCollection {

  static final Class DEFAULT_ENTITY = DefaultGeometryShader.class;
  
  public static final boolean SHOW_FACES_DEFAULT=true;
  public static final boolean SHOW_LINES_DEFAULT=true;
  public static final boolean SHOW_POINTS_DEFAULT=true;
 
  Boolean getShowFaces();
  void setShowFaces(Boolean faceDraw);

  Boolean getShowLines();
  void setShowLines(Boolean edgeDraw);
  
  Boolean getShowPoints();
  void setShowPoints(Boolean vertexDraw);
  
  PointShader getPointShader();
  PointShader createPointShader(String name);
  
  LineShader getLineShader();
  LineShader createLineShader(String name);

  PolygonShader getPolygonShader();
  PolygonShader createPolygonShader(String name);
 
//  RenderingHintsShader getRenderingHintsShader();
//  RenderingHintsShader createRenderingHintsShader(String name);
  
//  TextShader getTextShader();
//  TextShader createTextShader(String name);
}
