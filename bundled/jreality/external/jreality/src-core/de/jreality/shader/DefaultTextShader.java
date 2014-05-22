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
import java.awt.Font;

import javax.swing.SwingConstants;

public interface DefaultTextShader extends TextShader {

	Object CREATE_DEFAULT = new Object();
  
	final static Color DIFFUSE_COLOR_DEFAULT = Color.BLACK;
	final static double SCALE_DEFAULT = 0.0125;
	final static Font FONT_DEFAULT = new Font("Sans Serif",Font.PLAIN,48);
	final static double[] OFFSET_DEFAULT = new double[]{0,0,1,1};
	final static Boolean SHOW_LABELS_DEFAULT = Boolean.TRUE;
	final static int ALIGNMENT_DEFAULT = SwingConstants.NORTH_EAST;
	
  
	Color getDiffuseColor();
	void setDiffuseColor(Color c);
	  
	Double getScale();
	void setScale(Double s);
	
	Font getFont();
	void setFont(Font f);
  
	double[] getOffset();
	void setOffset(double[] o);

	Integer getAlignment();
	void setAlignment(Integer a);
	
	// until there is an alternative for making labels invisible
	// without completely deleting them, this stays undeprecated. -gunn
	Boolean getShowLabels();
 	void setShowLabels(Boolean b);
		
}
