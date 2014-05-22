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

import de.jreality.scene.data.AttributeEntity;

public interface RootAppearance extends AttributeEntity {

  public final static Color  BACKGROUND_COLORS_DEFAULT =  null;
  public final static Color  BACKGROUND_COLOR_DEFAULT = new java.awt.Color(225, 225, 225);
  public final static boolean  FOG_ENABLED_DEFAULT = false;
  public final static Color  FOG_COLOR_DEFAULT = new java.awt.Color(225, 225, 225);
  public final static double FOG_DENSITY_DEFAULT = 0.1;
  public final static boolean RENDER_S3_DEFAULT = false;
  public final static boolean USE_GLSL_DEFAULT = false;


  Color getBackgroundColor();
  void setBackgroundColor(Color color);
  
  Color[] getBackgroundColors();
  void setBackgroundColors(Color[] color);
  
  Boolean getFogEnabled();
  void setFogEnabled(Boolean fog);
  
  Color getFogColor();
  void setFogColor(Color fogColor);
  
  Double getFogDensity();
  void setFogDensity(Double density);
  
  Boolean getRenderS3();
  void setRenderS3(Boolean b);
  
  Boolean getUseGLSL();
  void setUseGLSL(Boolean b);
  
  Texture2D createBackgroundTexture2D();
  Texture2D getBackgroundTexture2D();
}
