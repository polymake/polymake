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

import de.jreality.math.Pn;
import de.jreality.scene.data.AttributeEntity;

public interface RenderingHintsShader extends AttributeEntity {

	  static final Class DEFAULT_ENTITY=RenderingHintsShader.class;
	  Object CREATE_DEFAULT=new Object();
	  
	  final static int METRIC_DEFAULT = Pn.EUCLIDEAN;			// do lighting or not
	  final static boolean LIGHTING_ENABLED_DEFAULT = true;			// do lighting or not
      final static boolean TRANSPARENCY_ENABLED_DEFAULT = false;        // do transparency or not
      final static boolean ADDITIVE_BLENDING_ENABLED_DEFAULT = false;        // do transparency or not
	  final static boolean Z_BUFFER_ENABLED_DEFAULT = false;			// can help to switch this in transparency mode
	  final static boolean IGNORE_ALPHA0_DEFAULT = true;				// pseudo-transparency available in OpenGL
	  final static boolean ANY_DISPLAY_LISTS_DEFAULT = true;			// use display lists at all?
	  final static boolean BACK_FACE_CULLING_DEFAULT = false;	
	  final static boolean OPAQUE_TUBES_AND_SPHERES_DEFAULT = true;
	  final static boolean LOCAL_LIGHT_MODEL_DEFAULT = true;		
	  final static boolean SEPARATE_SPECULAR_DEFAULT = false;
	  final static boolean FLIP_NORMALS_DEFAULT = false;	
	  final static double LEVEL_OF_DETAIL_DEFAULT = 			0.0;
	  final static double DEPTH_FUDGE_FACTOR_DEFAULT = 1.0;
	  
	  Integer getMetric();
	  void setMetric(Integer m);
	  
	  Boolean getLightingEnabled();
	  void setLightingEnabled(Boolean b);

      Boolean getTransparencyEnabled();
      void setTransparencyEnabled(Boolean d);

      Boolean getAdditiveBlendingEnabled();
      void setAdditiveBlendingEnabled(Boolean d);

	  Boolean getZBufferEnabled();
	  void setZBufferEnabled(Boolean d);

	  Boolean getIgnoreAlpha0();
	  void setIgnoreAlpha0(Boolean d);

	  Boolean getLocalLightModel();
	  void setLocalLightModel(Boolean d);

	  Boolean getSeparateSpecularColor();
	  void setSeparateSpecularColor(Boolean b);

	  Boolean getAnyDisplayLists();
	  void setAnyDisplayLists(Boolean d);

	  Boolean getBackFaceCulling();
	  void setBackFaceCulling(Boolean d);

	  Boolean getFlipNormals();
	  void setFlipNormals(Boolean d);

	  Boolean getOpaqueTubesAndSpheres();
	  void setOpaqueTubesAndSpheres(Boolean d);

	  Double getLevelOfDetail();
	  void setLevelOfDetail(Double d);

	  Double getDepthFudgeFactor();
	  void setDepthFudgeFactor(Double d);

}
